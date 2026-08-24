/// @file par_validation_test.cpp
/// @brief Regression tests for double dummy table validation in the par API.
///
/// The par entry points derive contract levels directly from
/// DdTableResults::res_table and format them into fixed-size character
/// buffers. Before par_table_checks() was added, an out-of-range table
/// overflowed those buffers: a table full of 2000000000 produced a
/// stack-buffer-overflow in Par() (strcat into `char temp[8]`, par.cpp:121)
/// and a silent 26-character write into the `char[10]` field
/// ParResultsDealer::contracts[0] via SidesPar(), both while still returning
/// RETURN_NO_FAULT.
///
/// These tests are most meaningful under --config=asan, where an unguarded
/// regression aborts rather than merely returning the wrong code.

#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include <dds/dds.h>

namespace {

/// A legal table: trick counts per strain summing to 13 across the two sides.
auto legal_table() -> DdTableResults
{
  DdTableResults tab;
  std::memset(&tab, 0, sizeof(tab));
  for (int d = 0; d < DDS_STRAINS; d++)
  {
    tab.res_table[d][0] = 7;  // North
    tab.res_table[d][1] = 6;  // East
    tab.res_table[d][2] = 7;  // South
    tab.res_table[d][3] = 6;  // West
  }
  return tab;
}

/// A table with every entry set to `v`.
auto uniform_table(int v) -> DdTableResults
{
  DdTableResults tab;
  std::memset(&tab, 0, sizeof(tab));
  for (int d = 0; d < DDS_STRAINS; d++)
    for (int h = 0; h < DDS_HANDS; h++)
      tab.res_table[d][h] = v;
  return tab;
}

// ---------------------------------------------------------------------------
// The original overflow trigger.
// ---------------------------------------------------------------------------

TEST(ParValidation, ParRejectsOverflowingTable)
{
  DdTableResults const tab = uniform_table(2000000000);
  ParResults resp;
  std::memset(&resp, 0, sizeof(resp));

  EXPECT_EQ(Par(&tab, &resp, 0), RETURN_PAR_TABLE_FAULT);
}

TEST(ParValidation, SidesParRejectsOverflowingTable)
{
  DdTableResults const tab = uniform_table(2000000000);
  ParResultsDealer sides[2];
  std::memset(sides, 0, sizeof(sides));

  EXPECT_EQ(SidesPar(&tab, sides, 0), RETURN_PAR_TABLE_FAULT);
  // The pre-fix failure wrote 26 characters into this 10-byte field.
  EXPECT_LT(std::strlen(sides[0].contracts[0]), sizeof(sides[0].contracts[0]));
}

TEST(ParValidation, SidesParBinRejectsOverflowingTable)
{
  DdTableResults const tab = uniform_table(2000000000);
  ParResultsMaster sides[2];
  std::memset(sides, 0, sizeof(sides));

  EXPECT_EQ(SidesParBin(&tab, sides, 0), RETURN_PAR_TABLE_FAULT);
}

TEST(ParValidation, DealerParRejectsOverflowingTable)
{
  DdTableResults const tab = uniform_table(2000000000);
  ParResultsDealer resp;
  std::memset(&resp, 0, sizeof(resp));

  EXPECT_EQ(DealerPar(&tab, &resp, 0, 0), RETURN_PAR_TABLE_FAULT);
}

TEST(ParValidation, DealerParBinRejectsOverflowingTable)
{
  DdTableResults const tab = uniform_table(2000000000);
  ParResultsMaster resp;
  std::memset(&resp, 0, sizeof(resp));

  EXPECT_EQ(DealerParBin(&tab, &resp, 0, 0), RETURN_PAR_TABLE_FAULT);
}

// ---------------------------------------------------------------------------
// Range boundaries.
// ---------------------------------------------------------------------------

TEST(ParValidation, ThirteenTricksIsAccepted)
{
  // 13 is the largest legal trick count and must not be rejected.
  DdTableResults tab = legal_table();
  tab.res_table[0][0] = 13;
  tab.res_table[0][2] = 13;
  tab.res_table[0][1] = 0;
  tab.res_table[0][3] = 0;

  ParResults resp;
  std::memset(&resp, 0, sizeof(resp));
  EXPECT_EQ(Par(&tab, &resp, 0), RETURN_NO_FAULT);
}

TEST(ParValidation, FourteenTricksIsRejected)
{
  DdTableResults tab = legal_table();
  tab.res_table[2][1] = 14;

  ParResults resp;
  std::memset(&resp, 0, sizeof(resp));
  EXPECT_EQ(Par(&tab, &resp, 0), RETURN_PAR_TABLE_FAULT);
}

TEST(ParValidation, NegativeTrickCountIsRejected)
{
  DdTableResults tab = legal_table();
  tab.res_table[3][2] = -1;

  ParResults resp;
  std::memset(&resp, 0, sizeof(resp));
  EXPECT_EQ(Par(&tab, &resp, 0), RETURN_PAR_TABLE_FAULT);
}

TEST(ParValidation, SingleBadEntryAnywhereIsRejected)
{
  // Every position is checked, not just the first.
  for (int d = 0; d < DDS_STRAINS; d++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      DdTableResults tab = legal_table();
      tab.res_table[d][h] = 99;

      ParResults resp;
      std::memset(&resp, 0, sizeof(resp));
      EXPECT_EQ(Par(&tab, &resp, 0), RETURN_PAR_TABLE_FAULT)
        << "strain " << d << ", hand " << h;
    }
  }
}

TEST(ParValidation, NullTableIsRejected)
{
  ParResults resp;
  std::memset(&resp, 0, sizeof(resp));
  EXPECT_EQ(Par(nullptr, &resp, 0), RETURN_PAR_TABLE_FAULT);
}

// ---------------------------------------------------------------------------
// Legal input still works — the guard must not reject valid tables.
// ---------------------------------------------------------------------------

TEST(ParValidation, LegalTableStillProducesAParResult)
{
  DdTableResults const tab = legal_table();

  ParResults resp;
  std::memset(&resp, 0, sizeof(resp));
  ASSERT_EQ(Par(&tab, &resp, 0), RETURN_NO_FAULT);
  EXPECT_GT(std::strlen(resp.par_score[0]), 0u);
  EXPECT_LT(std::strlen(resp.par_score[0]), sizeof(resp.par_score[0]));
  EXPECT_LT(std::strlen(resp.par_contracts_string[0]),
            sizeof(resp.par_contracts_string[0]));
}

TEST(ParValidation, LegalTableAcceptedByAllEntryPoints)
{
  DdTableResults const tab = legal_table();

  ParResultsDealer sides[2];
  std::memset(sides, 0, sizeof(sides));
  EXPECT_EQ(SidesPar(&tab, sides, 0), RETURN_NO_FAULT);

  ParResultsMaster sidesBin[2];
  std::memset(sidesBin, 0, sizeof(sidesBin));
  EXPECT_EQ(SidesParBin(&tab, sidesBin, 0), RETURN_NO_FAULT);

  ParResultsDealer dealerRes;
  std::memset(&dealerRes, 0, sizeof(dealerRes));
  EXPECT_EQ(DealerPar(&tab, &dealerRes, 0, 0), RETURN_NO_FAULT);

  ParResultsMaster dealerBin;
  std::memset(&dealerBin, 0, sizeof(dealerBin));
  EXPECT_EQ(DealerParBin(&tab, &dealerBin, 0, 0), RETURN_NO_FAULT);
}

TEST(ParValidation, AllLegalVulnerabilitiesAccepted)
{
  DdTableResults const tab = legal_table();
  for (int vul = 0; vul <= 3; vul++)
  {
    ParResultsDealer resp;
    std::memset(&resp, 0, sizeof(resp));
    EXPECT_EQ(DealerPar(&tab, &resp, 0, vul), RETURN_NO_FAULT)
      << "vulnerable = " << vul;
  }
}

// ---------------------------------------------------------------------------
// DealerPar indexes VUL_LOOKUP[4][2] by `vulnerable`, so it must be
// range-checked rather than only compared against, as SidesParBin() does.
// ---------------------------------------------------------------------------

TEST(ParValidation, DealerParRejectsOutOfRangeVulnerability)
{
  DdTableResults const tab = legal_table();

  for (int vul : {-1, 4, 99})
  {
    ParResultsDealer resp;
    std::memset(&resp, 0, sizeof(resp));
    EXPECT_EQ(DealerPar(&tab, &resp, 0, vul), RETURN_UNKNOWN_FAULT)
      << "vulnerable = " << vul;
  }
}

// ---------------------------------------------------------------------------
// DealerPar() propagates `dealer` into the par tables via pno_list[], where
// sacrifice_as_text() used to subscript with static_cast<unsigned>(pno) --
// turning a negative index into a multi-gigabyte offset. Found by the par
// fuzz harness within 50000 runs, after the `vulnerable` fix above had
// guarded one parameter of the pair and missed the other.
// ---------------------------------------------------------------------------

TEST(ParValidation, DealerParRejectsOutOfRangeDealer)
{
  DdTableResults const tab = legal_table();

  for (int dealer : {-1, 4, 99, -2147483647})
  {
    ParResultsDealer resp;
    std::memset(&resp, 0, sizeof(resp));
    EXPECT_EQ(DealerPar(&tab, &resp, dealer, 0), RETURN_UNKNOWN_FAULT)
      << "dealer = " << dealer;
  }
}

TEST(ParValidation, DealerParBinRejectsOutOfRangeDealer)
{
  DdTableResults const tab = legal_table();

  for (int dealer : {-1, 4})
  {
    ParResultsMaster resp;
    std::memset(&resp, 0, sizeof(resp));
    EXPECT_EQ(DealerParBin(&tab, &resp, dealer, 0), RETURN_UNKNOWN_FAULT)
      << "dealer = " << dealer;
  }
}

TEST(ParValidation, AllLegalDealersAccepted)
{
  DdTableResults const tab = legal_table();

  for (int dealer = 0; dealer <= 3; dealer++)
  {
    ParResultsDealer resp;
    std::memset(&resp, 0, sizeof(resp));
    EXPECT_EQ(DealerPar(&tab, &resp, dealer, 0), RETURN_NO_FAULT)
      << "dealer = " << dealer;
  }
}

TEST(ParValidation, SacrificeContractTextIsWellFormed)
{
  // A table where sacrificing is right, so the text path that used to index
  // NUMBER_TO_PLAYER out of bounds actually runs. No "?" placeholder should
  // appear: that would mean an index escaped DealerPar()'s range checks.
  DdTableResults tab;
  std::memset(&tab, 0, sizeof(tab));
  for (int d = 0; d < DDS_STRAINS; d++)
  {
    tab.res_table[d][0] = 12;
    tab.res_table[d][1] = 1;
    tab.res_table[d][2] = 12;
    tab.res_table[d][3] = 1;
  }

  for (int dealer = 0; dealer <= 3; dealer++)
  {
    ParResultsDealer resp;
    std::memset(&resp, 0, sizeof(resp));
    ASSERT_EQ(DealerPar(&tab, &resp, dealer, 0), RETURN_NO_FAULT);

    for (int k = 0; k < resp.number; k++)
    {
      std::string const contract(resp.contracts[k]);
      EXPECT_EQ(contract.find('?'), std::string::npos)
        << "dealer " << dealer << " contract " << k << ": " << contract;
      EXPECT_LT(contract.size(), sizeof(resp.contracts[k]));
    }
  }
}

// ---------------------------------------------------------------------------
// The new code is wired into the error-message table.
// ---------------------------------------------------------------------------

TEST(ParValidation, ErrorMessageDescribesTableFault)
{
  char line[80];
  std::memset(line, 0, sizeof(line));
  ErrorMessage(RETURN_PAR_TABLE_FAULT, line);

  EXPECT_STREQ(line, TEXT_PAR_TABLE_FAULT);
  EXPECT_GT(std::strlen(line), 0u);
}

}  // namespace
