/// @file worker_context_reuse_test.cpp
/// @brief Tests for the persistent per-worker SolverContext used by batch
///        paths. Contexts (and their transposition tables) must be created
///        once per worker thread and reused across consecutive batch calls,
///        without changing results.

#include <gtest/gtest.h>
#include <cstdint>
#include <thread>

#include <api/dll.h>
#include <dds/dds.hpp>
#include <solver_context/solver_context.hpp>

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

// The helper hands out one persistent context per calling thread.
TEST(WorkerSolverContext, SameThreadReturnsSameInstance)
{
  SolverContext& first = dds::internal::worker_solver_context();
  SolverContext& second = dds::internal::worker_solver_context();
  EXPECT_EQ(&first, &second);
}

// Distinct threads must not share a context (SolverContext is not thread-safe).
TEST(WorkerSolverContext, DistinctThreadsGetDistinctInstances)
{
  SolverContext* main_ctx = &dds::internal::worker_solver_context();
  SolverContext* other_ctx = nullptr;
  std::thread t([&] { other_ctx = &dds::internal::worker_solver_context(); });
  t.join();
  ASSERT_NE(other_ctx, nullptr);
  EXPECT_NE(main_ctx, other_ctx);
}

// Repeated access on one thread creates exactly one context (counter seam).
TEST(WorkerSolverContext, RepeatedCallsOnSameThreadCreateOneContext)
{
  (void)dds::internal::worker_solver_context();
  const std::uint64_t before = dds::internal::worker_solver_contexts_created();
  (void)dds::internal::worker_solver_context();
  (void)dds::internal::worker_solver_context();
  const std::uint64_t after = dds::internal::worker_solver_contexts_created();
  EXPECT_EQ(before, after);
}

// Consecutive batch calc calls must reuse worker contexts: since pool worker
// threads persist across calls, the total number of contexts ever created is
// bounded by the worker count, no matter how many batches run. Results must
// stay identical across calls (no stale transposition-table pollution).
TEST(WorkerContextReuse, RepeatedCalcCallsAreBoundedByWorkerCountAndStayCorrect)
{
  InitializeStaticMemory();
  const DdTableDeal deal = make_known_deal();
  constexpr int kWorkers = 2;
  constexpr int kCalls = 3;

  DdTableResults reference{};
  ASSERT_EQ(CalcDDtableN(deal, &reference, /*maxThreads=*/1), RETURN_NO_FAULT);

  const std::uint64_t before = dds::internal::worker_solver_contexts_created();

  for (int call = 0; call < kCalls; call++)
  {
    DdTableResults table{};
    ASSERT_EQ(CalcDDtableN(deal, &table, kWorkers), RETURN_NO_FAULT)
        << "call=" << call;
    expect_tables_equal(reference, table);
  }

  const std::uint64_t created =
    dds::internal::worker_solver_contexts_created() - before;
  EXPECT_LE(created, static_cast<std::uint64_t>(kWorkers))
      << "worker contexts must be reused across consecutive batch calls";
}

// The batch solve path must also stay correct when worker contexts (and their
// transposition tables) are reused across consecutive calls.
TEST(WorkerContextReuse, RepeatedSolveCallsStayCorrect)
{
  InitializeStaticMemory();
  const DdTableDeal table_deal = make_known_deal();

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

  SolvedBoards first{};
  ASSERT_EQ(SolveAllBoardsBinN(&bo, &first, /*maxThreads=*/2), RETURN_NO_FAULT);

  SolvedBoards second{};
  ASSERT_EQ(SolveAllBoardsBinN(&bo, &second, /*maxThreads=*/2), RETURN_NO_FAULT);

  ASSERT_EQ(first.no_of_boards, second.no_of_boards);
  for (int b = 0; b < first.no_of_boards; b++)
  {
    const FutureTricks& fa = first.solved_board[b];
    const FutureTricks& fb = second.solved_board[b];
    ASSERT_EQ(fa.cards, fb.cards) << "card count differs at board=" << b;
    for (int c = 0; c < fa.cards; c++)
    {
      EXPECT_EQ(fa.suit[c], fb.suit[c]) << "suit at board=" << b << " c=" << c;
      EXPECT_EQ(fa.rank[c], fb.rank[c]) << "rank at board=" << b << " c=" << c;
      EXPECT_EQ(fa.score[c], fb.score[c]) << "score at board=" << b << " c=" << c;
    }
  }
}
