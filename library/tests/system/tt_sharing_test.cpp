/// @file tt_sharing_test.cpp
/// @brief Tests for transposition table sharing across SolverContext instances.
///
/// Validates that multiple SolverContext instances with the same thread
/// data share the same transposition table instance.

#include <gtest/gtest.h>
#include <solver_context/solver_context.hpp>
#include "system/memory.hpp"

extern Memory memory;

TEST(TransTableSharingTest, SameThreadSharesTT)
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

  // Create an owning context for this (simulates a thread-local owner)
  SolverContext owner;
  auto thr = owner.thread();
  SolverContext ctx1{thr};
  SolverContext ctx2{thr};

  // Initially no TT
  EXPECT_EQ(ctx1.maybe_trans_table(), nullptr);
  EXPECT_EQ(ctx2.maybe_trans_table(), nullptr);

  TransTable* t1 = ctx1.trans_table();
  ASSERT_NE(t1, nullptr);
  TransTable* t2 = ctx2.maybe_trans_table();
  ASSERT_NE(t2, nullptr);
  EXPECT_EQ(t1, t2);

  // Dispose via one context should remove from registry
  ctx1.dispose_trans_table();
  EXPECT_EQ(ctx2.maybe_trans_table(), nullptr);
}
