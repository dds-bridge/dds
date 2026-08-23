/// @file calc_dd_table_partial_test.cpp
/// @brief CalcDDtable must report tricks out of remaining cards, not always 13.

#include <cstring>
#include <gtest/gtest.h>

#include <api/calc_dd_table.hpp>
#include <api/dll.h>

namespace
{

// One card each: NS hold ♠AK, EW hold ♠QJ. NS take the single trick as either
// declarer; EW take none.
constexpr const char* kOneTrickSpadesPbn = "N:A... Q... K... J...";

void expect_ns_take_all_remaining(const DdTableResults& table, int tricks)
{
  for (int strain = 0; strain < DDS_STRAINS; strain++)
  {
    EXPECT_EQ(table.res_table[strain][0], tricks) << "strain=" << strain << " North";
    EXPECT_EQ(table.res_table[strain][1], 0) << "strain=" << strain << " East";
    EXPECT_EQ(table.res_table[strain][2], tricks) << "strain=" << strain << " South";
    EXPECT_EQ(table.res_table[strain][3], 0) << "strain=" << strain << " West";
  }
}

}  // namespace

TEST(CalcDdTablePartial, OneCardPerHandUsesRemainingTricksNotThirteen)
{
  InitializeStaticMemory();

  DdTableDealPBN deal{};
  std::strncpy(deal.cards, kOneTrickSpadesPbn, sizeof(deal.cards) - 1);
  deal.cards[sizeof(deal.cards) - 1] = '\0';

  DdTableResults table{};
  ASSERT_EQ(CalcDDtablePBN(deal, &table), RETURN_NO_FAULT);

  // Regression: before the fix, every entry was 13 or 12 (hardcoded 13 - score).
  for (int strain = 0; strain < DDS_STRAINS; strain++)
    for (int hand = 0; hand < DDS_HANDS; hand++)
    {
      EXPECT_GE(table.res_table[strain][hand], 0);
      EXPECT_LE(table.res_table[strain][hand], 1)
          << "strain=" << strain << " hand=" << hand;
    }

  expect_ns_take_all_remaining(table, /*tricks=*/1);
}

TEST(CalcAllTablesPartial, OneCardPerHandUsesRemainingTricksNotThirteen)
{
  InitializeStaticMemory();

  DdTableDealsPBN deals{};
  deals.no_of_tables = 1;
  std::strncpy(deals.deals[0].cards, kOneTrickSpadesPbn, sizeof(deals.deals[0].cards) - 1);
  deals.deals[0].cards[sizeof(deals.deals[0].cards) - 1] = '\0';

  int trump_filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
  DdTablesRes resp{};
  AllParResults par{};
  ASSERT_EQ(
      CalcAllTablesPBN(&deals, /*mode=*/-1, trump_filter, &resp, &par),
      RETURN_NO_FAULT);

  expect_ns_take_all_remaining(resp.results[0], /*tricks=*/1);
}

TEST(CalcAllTablesXPartial, OneCardPerHandUsesRemainingTricksNotThirteen)
{
  InitializeStaticMemory();

  DdTableDealPBN deal{};
  std::strncpy(deal.cards, kOneTrickSpadesPbn, sizeof(deal.cards) - 1);
  deal.cards[sizeof(deal.cards) - 1] = '\0';

  int trump_filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
  DdTableResults result{};
  ASSERT_EQ(
      CalcAllTablesPBNX(1, &deal, /*mode=*/-1, trump_filter, &result, nullptr, 1),
      RETURN_NO_FAULT);

  expect_ns_take_all_remaining(result, /*tricks=*/1);
}

TEST(CalcDdTablePartialCpp, OneCardPerHandUsesRemainingTricksNotThirteen)
{
  InitializeStaticMemory();

  DdTableDealPBN deal_pbn{};
  std::strncpy(deal_pbn.cards, kOneTrickSpadesPbn, sizeof(deal_pbn.cards) - 1);
  deal_pbn.cards[sizeof(deal_pbn.cards) - 1] = '\0';

  DdTableResults table{};
  ASSERT_EQ(calc_dd_table_pbn(deal_pbn, &table), RETURN_NO_FAULT);

  for (int strain = 0; strain < DDS_STRAINS; strain++)
    for (int hand = 0; hand < DDS_HANDS; hand++)
    {
      EXPECT_GE(table.res_table[strain][hand], 0);
      EXPECT_LE(table.res_table[strain][hand], 1)
          << "strain=" << strain << " hand=" << hand;
    }

  expect_ns_take_all_remaining(table, /*tricks=*/1);
}
