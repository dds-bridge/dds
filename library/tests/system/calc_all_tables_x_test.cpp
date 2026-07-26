/// @file calc_all_tables_x_test.cpp
/// @brief Tests for unbounded CalcAllTablesX / CalcAllTablesPBNX.
///
/// The unbounded APIs must accept more than MAXNOOFTABLES deals and run all
/// deal×strain boards as a single parallel job (ddss-style), not as repeated
/// MAXNOOFTABLES-sized chunks.

#include <gtest/gtest.h>
#include <cstring>
#include <vector>

#include <api/dll.h>
#include <dds/dds.hpp>
#include <calc_tables.hpp>
#include <system/parallel_boards.hpp>

namespace
{

DdTableDeal make_known_deal()
{
  DdTableDeal deal{};
  deal.cards[0][0] = 0x1800 | 0x0040;
  deal.cards[0][1] = 0x2000 | 0x0060 | 0x0004;
  deal.cards[0][2] = 0x0800 | 0x0100 | 0x0020;
  deal.cards[0][3] = 0x0400 | 0x0200 | 0x0100;
  deal.cards[1][0] = 0x0100 | 0x0080 | 0x0008;
  deal.cards[1][1] = 0x0800 | 0x0200 | 0x0080;
  deal.cards[1][2] = 0x4000 | 0x0400 | 0x0080 | 0x0040 | 0x0010;
  deal.cards[1][3] = 0x1000 | 0x0010;
  deal.cards[2][0] = 0x2000 | 0x0020;
  deal.cards[2][1] = 0x0400 | 0x0100 | 0x0008;
  deal.cards[2][2] = 0x2000 | 0x1000 | 0x0200;
  deal.cards[2][3] = 0x4000 | 0x0080 | 0x0040 | 0x0020 | 0x0004;
  deal.cards[3][0] = 0x4000 | 0x0400 | 0x0200 | 0x0010 | 0x0004;
  deal.cards[3][1] = 0x4000 | 0x1000 | 0x0010;
  deal.cards[3][2] = 0x0008 | 0x0004;
  deal.cards[3][3] = 0x2000 | 0x0800 | 0x0008;
  return deal;
}

void expect_tables_equal(const DdTableResults& a, const DdTableResults& b)
{
  for (int strain = 0; strain < DDS_STRAINS; strain++)
    for (int hand = 0; hand < DDS_HANDS; hand++)
      EXPECT_EQ(a.res_table[strain][hand], b.res_table[strain][hand])
          << "Mismatch at strain=" << strain << " hand=" << hand;
}

}  // namespace

TEST(CalcAllTablesX, ChunkDealsMatchesBoardBudget)
{
  EXPECT_EQ(calc_all_tables_chunk_deals(DDS_STRAINS), MAXNOOFBOARDS / DDS_STRAINS);
  EXPECT_EQ(calc_all_tables_chunk_deals(0), 0);
}

TEST(CalcAllTablesX, NullPointersReturnUnknownFault)
{
  InitializeStaticMemory();
  const DdTableDeal known = make_known_deal();
  DdTableDeal deal = known;
  DdTableResults result{};
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(
    CalcAllTablesX(1, nullptr, -1, filter, &result, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
  EXPECT_EQ(
    CalcAllTablesX(1, &deal, -1, filter, nullptr, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
  EXPECT_EQ(
    CalcAllTablesX(1, &deal, -1, nullptr, &result, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
  // mode in [0, 3] with all strains included requests par output.
  EXPECT_EQ(
    CalcAllTablesX(1, &deal, /*mode=*/0, filter, &result, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
}

TEST(CalcAllTablesPBNX, NullPointersReturnUnknownFault)
{
  InitializeStaticMemory();
  DdTableDealPBN deal{};
  std::strncpy(
    deal.cards,
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
    sizeof(deal.cards));
  DdTableResults result{};
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(
    CalcAllTablesPBNX(1, nullptr, -1, filter, &result, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
  EXPECT_EQ(
    CalcAllTablesPBNX(1, &deal, -1, filter, nullptr, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
  EXPECT_EQ(
    CalcAllTablesPBNX(1, &deal, -1, nullptr, &result, nullptr, 1),
    RETURN_UNKNOWN_FAULT);
}

TEST(CalcAllTablesX, LegacyRejectsMoreThanMaxTables)
{
  InitializeStaticMemory();
  DdTableDeals deals{};
  deals.no_of_tables = MAXNOOFTABLES + 1;
  const DdTableDeal known = make_known_deal();
  // Fill only the in-bounds slots; CalcAllTablesN must reject on count alone.
  for (int i = 0; i < MAXNOOFTABLES; i++)
    deals.deals[i] = known;

  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
  DdTablesRes results{};
  AllParResults par{};
  EXPECT_EQ(
    CalcAllTablesN(&deals, -1, filter, &results, &par, /*maxThreads=*/1),
    RETURN_TOO_MANY_TABLES);
}

TEST(CalcAllTablesX, AcceptsMoreThanMaxTablesAndMatchesLegacy)
{
  InitializeStaticMemory();
  const DdTableDeal known = make_known_deal();
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  DdTableDeals legacy{};
  legacy.no_of_tables = 2;
  legacy.deals[0] = known;
  legacy.deals[1] = known;
  DdTablesRes legacy_results{};
  AllParResults par{};
  ASSERT_EQ(
    CalcAllTablesN(&legacy, -1, filter, &legacy_results, &par, 1),
    RETURN_NO_FAULT);

  constexpr int kNum = MAXNOOFTABLES + 1;
  std::vector<DdTableDeal> deals(static_cast<unsigned>(kNum), known);
  std::vector<DdTableResults> results(static_cast<unsigned>(kNum));
  ASSERT_EQ(
    CalcAllTablesX(
      kNum, deals.data(), -1, filter, results.data(), nullptr, 1),
    RETURN_NO_FAULT);

  expect_tables_equal(legacy_results.results[0], results[0]);
  expect_tables_equal(legacy_results.results[0], results[static_cast<unsigned>(kNum - 1)]);
}

// The performance point of PBNx: a batch larger than MAXNOOFTABLES must be
// one parallel_all_boards_n job covering every deal×strain board, not N
// chunked jobs of MAXNOOFBOARDS each.
TEST(CalcAllTablesX, LargeBatchIsSingleParallelJob)
{
  InitializeStaticMemory();
  const DdTableDeal known = make_known_deal();
  constexpr int kNum = MAXNOOFTABLES + 1;
  const int expected_boards = kNum * DDS_STRAINS;

  std::vector<DdTableDeal> deals(static_cast<unsigned>(kNum), known);
  std::vector<DdTableResults> results(static_cast<unsigned>(kNum));
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  (void)dds::internal::parallel_boards_last_job_board_count();
  ASSERT_EQ(
    CalcAllTablesX(
      kNum, deals.data(), -1, filter, results.data(), nullptr,
      /*maxThreads=*/2),
    RETURN_NO_FAULT);

  EXPECT_EQ(
    dds::internal::parallel_boards_last_job_board_count(),
    expected_boards)
      << "unbounded calc must dispatch all boards in one parallel job";
}

TEST(CalcAllTablesX, PbnVariantMatchesBinary)
{
  InitializeStaticMemory();
  const DdTableDeal known = make_known_deal();
  constexpr int kNum = 3;
  std::vector<DdTableDeal> binary(static_cast<unsigned>(kNum), known);
  std::vector<DdTableResults> binary_results(static_cast<unsigned>(kNum));
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
  ASSERT_EQ(
    CalcAllTablesX(
      kNum, binary.data(), -1, filter, binary_results.data(), nullptr, 1),
    RETURN_NO_FAULT);

  // PBN for the known deal (examples/hands.cpp hand 0).
  const char* pbn =
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";
  std::vector<DdTableDealPBN> pbn_deals(static_cast<unsigned>(kNum));
  for (int i = 0; i < kNum; i++)
    std::strncpy(pbn_deals[static_cast<unsigned>(i)].cards, pbn, sizeof(pbn_deals[0].cards));
  std::vector<DdTableResults> pbn_results(static_cast<unsigned>(kNum));
  ASSERT_EQ(
    CalcAllTablesPBNX(
      kNum, pbn_deals.data(), -1, filter, pbn_results.data(), nullptr, 1),
    RETURN_NO_FAULT);

  for (int i = 0; i < kNum; i++)
    expect_tables_equal(
      binary_results[static_cast<unsigned>(i)],
      pbn_results[static_cast<unsigned>(i)]);
}
