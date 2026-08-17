/// @file max_threads_equivalence_test.cpp
/// @brief Tests that the *N batch APIs honour maxThreads and stay equivalent to
///        the auto path, plus that the rename/alias both initialize the library.

#include <gtest/gtest.h>
#include <cstring>
#include <thread>

#include <api/dll.h>
#include <dds/dds.hpp>

namespace
{

// Known deal from examples/hands.cpp (hand 0), as used by context_equivalence_test.
// PBN: N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3
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

// CalcDDtableN with maxThreads=1 must match the auto CalcDDtable.
TEST(MaxThreadsEquivalence, CalcDDtableNMatchesAuto)
{
  InitializeStaticMemory();
  DdTableDeal deal = make_known_deal();

  DdTableResults table_auto{};
  ASSERT_EQ(CalcDDtable(deal, &table_auto), RETURN_NO_FAULT);

  DdTableResults table_one{};
  ASSERT_EQ(CalcDDtableN(deal, &table_one, /*maxThreads=*/1), RETURN_NO_FAULT);
  expect_tables_equal(table_auto, table_one);

  if (std::thread::hardware_concurrency() > 2)
  {
    DdTableResults table_two{};
    ASSERT_EQ(CalcDDtableN(deal, &table_two, /*maxThreads=*/2), RETURN_NO_FAULT);
    expect_tables_equal(table_auto, table_two);
  }
}

// SolveAllBoardsBinN with maxThreads=1 must match the auto SolveAllBoardsBin.
TEST(MaxThreadsEquivalence, SolveAllBoardsBinNMatchesAuto)
{
  InitializeStaticMemory();
  DdTableDeal table_deal = make_known_deal();

  // Solve all five strains as separate boards.
  Boards bo{};
  bo.no_of_boards = DDS_STRAINS;
  for (int tr = 0; tr < DDS_STRAINS; tr++)
  {
    Deal dl{};
    for (int h = 0; h < DDS_HANDS; h++)
      for (int s = 0; s < DDS_SUITS; s++)
        dl.remainCards[h][s] = table_deal.cards[h][s];
    dl.trump = tr;
    dl.first = 0;
    bo.deals[tr] = dl;
    bo.target[tr] = -1;
    bo.solutions[tr] = 1;
    bo.mode[tr] = 1;
  }

  SolvedBoards solved_auto{};
  ASSERT_EQ(SolveAllBoardsBin(&bo, &solved_auto), RETURN_NO_FAULT);

  SolvedBoards solved_one{};
  ASSERT_EQ(SolveAllBoardsBinN(&bo, &solved_one, /*maxThreads=*/1), RETURN_NO_FAULT);

  ASSERT_EQ(solved_auto.no_of_boards, solved_one.no_of_boards);
  for (int b = 0; b < solved_auto.no_of_boards; b++)
  {
    const FutureTricks& fa = solved_auto.solved_board[b];
    const FutureTricks& fo = solved_one.solved_board[b];
    ASSERT_EQ(fa.cards, fo.cards) << "card count differs at board=" << b;
    // Only the first `cards` entries are meaningful; the tail is uninitialized.
    for (int c = 0; c < fa.cards; c++)
    {
      EXPECT_EQ(fa.suit[c], fo.suit[c]) << "suit at board=" << b << " c=" << c;
      EXPECT_EQ(fa.rank[c], fo.rank[c]) << "rank at board=" << b << " c=" << c;
      EXPECT_EQ(fa.equals[c], fo.equals[c]) << "equals at board=" << b << " c=" << c;
      EXPECT_EQ(fa.score[c], fo.score[c]) << "score at board=" << b << " c=" << c;
    }
  }
}

// InitializeStaticMemory leaves the library usable for a subsequent solve.
TEST(MaxThreadsEquivalence, InitializeStaticMemoryThenSolve)
{
  InitializeStaticMemory();
  DdTableDeal deal = make_known_deal();
  DdTableResults table{};
  EXPECT_EQ(CalcDDtable(deal, &table), RETURN_NO_FAULT);
}

// The deprecated SetMaxThreads alias still initializes the library.
TEST(MaxThreadsEquivalence, DeprecatedSetMaxThreadsAliasStillWorks)
{
  SetMaxThreads(1);
  DdTableDeal deal = make_known_deal();
  DdTableResults table{};
  EXPECT_EQ(CalcDDtable(deal, &table), RETURN_NO_FAULT);
}
