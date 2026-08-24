/// @file deal_input_validation_test.cpp
/// @brief Regression tests for deal validation on the CalcDDtable* paths and
///        for the safety of the error-reporting path itself.
///
/// Both defects were found by the fuzz harnesses in library/tests/fuzz:
///
///   - CalcDDtable()/CalcDDtablePBN() did not check that the four hands held
///     equal numbers of cards, so a 51-card deal reached the search and read
///     14248 bytes past rel_rank_storage. SolveBoard() rejected the same deal.
///
///   - DumpInput() indexed card_suit[], card_hand[] and card_rank[] with the
///     very values board_range_checks() was rejecting as out of range, so the
///     error path read out of bounds. It is compiled in unless
///     DDS_NO_DUMP_ON_ERROR is defined, which the build does not define.
///
/// Most meaningful under --config=asan, where a regression aborts rather than
/// merely returning an unexpected code.

#include <gtest/gtest.h>
#include <cstring>
#include <memory>
#include <string>
#include <dds/dds.h>
#include <dds/dds.hpp>
#include <api/dds_c_api.h>

namespace {

constexpr char kLegalDeal[] =
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

/// The same deal one card short: north's spades are T8, not T98. Contains
/// only legal PBN characters -- the shape a truncated PBN file takes.
constexpr char kShortOneCard[] =
    "N:QJ6.K652.J85.T8 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

/// The same deal with a card replaced by an invalid rank. convert_from_pbn()
/// silently skips unrecognised characters, so this also arrives one card short.
constexpr char kBadRank[] =
    "N:QJ6.K652.J85.TZ8 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

auto pbn_deal(const char * cards) -> DdTableDealPBN
{
  DdTableDealPBN deal;
  std::memset(&deal, 0, sizeof(deal));
  std::strncpy(deal.cards, cards, sizeof(deal.cards) - 1);
  return deal;
}

/// A legal binary deal: each hand holds one complete suit.
auto one_suit_each() -> DdTableDeal
{
  constexpr unsigned kAllRanks = 0x7FFC;  // ranks 2..A
  DdTableDeal deal;
  std::memset(&deal, 0, sizeof(deal));
  for (int h = 0; h < DDS_HANDS; h++)
    deal.cards[h][h] = kAllRanks;
  return deal;
}

// ---------------------------------------------------------------------------
// Finding 01: unbalanced deals must not reach the search.
// ---------------------------------------------------------------------------

TEST(CalcTableValidation, ShortPbnDealIsRejected)
{
  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(CalcDDtablePBN(pbn_deal(kShortOneCard), &table),
            RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, InvalidRankCharacterIsRejected)
{
  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  // Rejected for the card count, since the invalid rank is skipped by the
  // parser rather than refused outright.
  EXPECT_EQ(CalcDDtablePBN(pbn_deal(kBadRank), &table), RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, UnbalancedBinaryDealIsRejected)
{
  DdTableDeal deal = one_suit_each();
  deal.cards[0][0] &= ~0x4000u;  // remove north's ace

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(CalcDDtable(deal, &table), RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, DuplicateCardIsRejected)
{
  DdTableDeal deal = one_suit_each();
  // Give north a card east already holds, keeping the hand counts equal.
  deal.cards[0][1] |= 0x4000u;
  deal.cards[0][0] &= ~0x4000u;
  deal.cards[1][1] |= 0x4000u;

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(CalcDDtable(deal, &table), RETURN_DUPLICATE_CARDS);
}

TEST(CalcTableValidation, BitsOutsideRankRangeAreRejected)
{
  DdTableDeal deal = one_suit_each();
  deal.cards[2][2] |= 0x8000u;  // above the ace bit

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(CalcDDtable(deal, &table), RETURN_SUIT_OR_RANK);

  DdTableDeal low = one_suit_each();
  low.cards[1][1] |= 0x0001u;  // below the deuce bit
  EXPECT_EQ(CalcDDtable(low, &table), RETURN_SUIT_OR_RANK);
}

TEST(CalcTableValidation, LegalDealStillProducesATable)
{
  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  ASSERT_EQ(CalcDDtablePBN(pbn_deal(kLegalDeal), &table), RETURN_NO_FAULT);

  for (int d = 0; d < DDS_STRAINS; d++)
    for (int h = 0; h < DDS_HANDS; h++)
      EXPECT_GE(table.res_table[d][h], 0) << "strain " << d << " hand " << h;
  for (int d = 0; d < DDS_STRAINS; d++)
    for (int h = 0; h < DDS_HANDS; h++)
      EXPECT_LE(table.res_table[d][h], 13) << "strain " << d << " hand " << h;
}

TEST(CalcTableValidation, LegalBinaryDealStillProducesATable)
{
  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(CalcDDtable(one_suit_each(), &table), RETURN_NO_FAULT);
}

TEST(CalcTableValidation, CalcAllTablesRejectsUnbalancedDeal)
{
  DdTableDeals deals;
  std::memset(&deals, 0, sizeof(deals));
  deals.no_of_tables = 1;
  deals.deals[0] = one_suit_each();
  deals.deals[0].cards[3][3] &= ~0x4000u;  // west one card short

  DdTablesRes res;
  std::memset(&res, 0, sizeof(res));
  AllParResults par;
  std::memset(&par, 0, sizeof(par));
  int const filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(CalcAllTables(&deals, -1, filter, &res, &par), RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, CppOverloadRejectsUnbalancedDeal)
{
  // The C++ calc_dd_table() overloads build Boards directly rather than going
  // through CalcDDtableN(), so they need the same guard.
  DdTableDeal deal = one_suit_each();
  deal.cards[0][0] &= ~0x4000u;

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(calc_dd_table(deal, &table), RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, CppContextOverloadRejectsUnbalancedDeal)
{
  DdTableDeal deal = one_suit_each();
  deal.cards[0][0] &= ~0x4000u;

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  SolverContext ctx;
  EXPECT_EQ(calc_dd_table(ctx, deal, &table), RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, CppPbnOverloadRejectsShortDeal)
{
  DdTableDealPBN const deal = pbn_deal(kShortOneCard);

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));
  EXPECT_EQ(calc_dd_table_pbn(deal, &table), RETURN_CARD_COUNT);
}

TEST(CalcTableValidation, CShimRejectsUnbalancedDeal)
{
  // dds_c_calc_dd_table delegates to the C++ overload guarded above.
  DdTableDeal deal = one_suit_each();
  deal.cards[0][0] &= ~0x4000u;

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));

  DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
  ASSERT_NE(ctx, nullptr);
  EXPECT_EQ(dds_c_calc_dd_table(ctx, &deal, &table), RETURN_CARD_COUNT);
  dds_c_destroy_solvercontext(ctx);
}

TEST(CalcTableValidation, CShimPbnRejectsShortDeal)
{
  DdTableDealPBN const deal = pbn_deal(kShortOneCard);

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));

  DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
  ASSERT_NE(ctx, nullptr);
  EXPECT_EQ(dds_c_calc_dd_table_pbn(ctx, &deal, &table), RETURN_CARD_COUNT);
  dds_c_destroy_solvercontext(ctx);
}

// ---------------------------------------------------------------------------
// Table-count boundary. DdTableDeals/DdTableDealsPBN carry a fixed
// MAXNOOFTABLES * DDS_STRAINS array, and no_of_tables was never checked
// against it -- CalcAllTablesPBNN() copied that many records into a
// fixed-size local before any validation ran.
// ---------------------------------------------------------------------------

TEST(CalcTableValidation, CalcAllTablesRejectsOversizedTableCount)
{
  DdTableDeals deals;
  std::memset(&deals, 0, sizeof(deals));
  deals.no_of_tables = MAXNOOFTABLES * DDS_STRAINS + 1;
  for (int i = 0; i < MAXNOOFTABLES * DDS_STRAINS; i++)
    deals.deals[i] = one_suit_each();

  DdTablesRes res;
  std::memset(&res, 0, sizeof(res));
  AllParResults par;
  std::memset(&par, 0, sizeof(par));
  int const filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(CalcAllTables(&deals, -1, filter, &res, &par),
            RETURN_TOO_MANY_TABLES);
}

TEST(CalcTableValidation, CalcAllTablesRejectsNegativeTableCount)
{
  DdTableDeals deals;
  std::memset(&deals, 0, sizeof(deals));
  deals.no_of_tables = -1;

  DdTablesRes res;
  std::memset(&res, 0, sizeof(res));
  AllParResults par;
  std::memset(&par, 0, sizeof(par));
  int const filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(CalcAllTables(&deals, -1, filter, &res, &par),
            RETURN_TOO_MANY_TABLES);
}

TEST(CalcTableValidation, CalcAllTablesWithZeroDealsSolvesNothing)
{
  // With no deals the board-building loop writes nothing, but the board count
  // was derived from a last-index variable initialised to 0 and so claimed
  // one board -- solving an uninitialised entry of a stack-local Boards.
  // MemorySanitizer reports it; found by the calc_all_tables fuzz harness.
  DdTableDeals deals;
  std::memset(&deals, 0, sizeof(deals));
  deals.no_of_tables = 0;

  DdTablesRes res;
  std::memset(&res, 0, sizeof(res));
  AllParResults par;
  std::memset(&par, 0, sizeof(par));
  int const filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(CalcAllTables(&deals, -1, filter, &res, &par), RETURN_NO_FAULT);
  EXPECT_EQ(res.no_of_boards, 0);
}

TEST(CalcTableValidation, CalcAllTablesPbnRejectsOversizedTableCount)
{
  // Before the guard this copied no_of_tables records into a fixed-size
  // local DdTableDeals, overflowing it on the stack.
  auto deals = std::make_unique<DdTableDealsPBN>();
  std::memset(deals.get(), 0, sizeof(DdTableDealsPBN));
  deals->no_of_tables = MAXNOOFTABLES * DDS_STRAINS + 64;
  for (int i = 0; i < MAXNOOFTABLES * DDS_STRAINS; i++)
    std::strncpy(deals->deals[i].cards, kLegalDeal,
                 sizeof(deals->deals[i].cards) - 1);

  auto res = std::make_unique<DdTablesRes>();
  std::memset(res.get(), 0, sizeof(DdTablesRes));
  auto par = std::make_unique<AllParResults>();
  std::memset(par.get(), 0, sizeof(AllParResults));
  int const filter[DDS_STRAINS] = {0, 0, 0, 0, 0};

  EXPECT_EQ(CalcAllTablesPBN(deals.get(), -1, filter, res.get(), par.get()),
            RETURN_TOO_MANY_TABLES);
}

// ---------------------------------------------------------------------------
// Finding 03: the error path must not index tables with the values it rejects.
// ---------------------------------------------------------------------------

TEST(DumpInputSafety, OutOfRangeTrickSuitAndRankAreReportedNotIndexed)
{
  Deal deal;
  std::memset(&deal, 0, sizeof(deal));
  deal.trump = 0;
  deal.first = 0;
  for (int k = 0; k < 3; k++)
  {
    deal.currentTrickSuit[k] = 7;    // card_suit has 5 entries
    deal.currentTrickRank[k] = 99;   // card_rank has 16 entries
  }

  FutureTricks fut;
  std::memset(&fut, 0, sizeof(fut));
  EXPECT_EQ(SolveBoard(deal, -1, 1, 1, &fut, 0), RETURN_SUIT_OR_RANK);
}

TEST(DumpInputSafety, OutOfRangeTrumpIsReportedNotIndexed)
{
  Deal deal;
  std::memset(&deal, 0, sizeof(deal));
  deal.trump = 99;
  deal.first = 0;

  FutureTricks fut;
  std::memset(&fut, 0, sizeof(fut));
  EXPECT_EQ(SolveBoard(deal, -1, 1, 1, &fut, 0), RETURN_TRUMP_WRONG);
}

TEST(DumpInputSafety, OutOfRangeFirstIsReportedNotIndexed)
{
  Deal deal;
  std::memset(&deal, 0, sizeof(deal));
  deal.trump = 0;
  deal.first = 99;

  FutureTricks fut;
  std::memset(&fut, 0, sizeof(fut));
  EXPECT_EQ(SolveBoard(deal, -1, 1, 1, &fut, 0), RETURN_FIRST_WRONG);
}

TEST(DumpInputSafety, NegativeTrickValuesAreReportedNotIndexed)
{
  Deal deal;
  std::memset(&deal, 0, sizeof(deal));
  deal.trump = 0;
  deal.first = 0;
  deal.currentTrickSuit[0] = -3;
  deal.currentTrickRank[0] = -7;

  FutureTricks fut;
  std::memset(&fut, 0, sizeof(fut));
  EXPECT_EQ(SolveBoard(deal, -1, 1, 1, &fut, 0), RETURN_SUIT_OR_RANK);
}

TEST(DumpInputSafety, UncheckedTrickSuitIsNotUsedAsSubscript)
{
  // board_range_checks() only validates currentTrickSuit[k] when the matching
  // rank is non-zero, but hand_rel_first is derived from the card count, so
  // board_value_checks() could reach an unchecked suit and index remainCards
  // out of bounds. Found by the solve_board fuzz harness.
  Deal deal;
  std::memset(&deal, 0, sizeof(deal));
  deal.trump = 4;
  deal.first = 0;
  deal.currentTrickSuit[1] = 24832;   // never validated: rank below is zero
  deal.remainCards[0][2] = 0x100;
  deal.remainCards[1][1] = 0x40;
  deal.remainCards[2][2] = 0x40;
  deal.remainCards[3][2] = 0x2000;
  deal.remainCards[3][3] = 0x4000;

  FutureTricks fut;
  std::memset(&fut, 0, sizeof(fut));
  EXPECT_EQ(SolveBoard(deal, 0, 3, 0, &fut, 0), RETURN_SUIT_OR_RANK);
}

}  // namespace
