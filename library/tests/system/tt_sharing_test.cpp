#include <gtest/gtest.h>
#include "system/SolverContext.h"
#include "system/Memory.h"

extern Memory memory;

TEST(TransTableSharingTest, SameThreadSharesTT)
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

  // Create an owning context for this (simulates a thread-local owner)
  SolverContext owner;
  ThreadData* thr = owner.thread();
  SolverContext ctx1{thr};
  SolverContext ctx2{thr};

  // Initially no TT
  EXPECT_EQ(ctx1.maybeTransTable(), nullptr);
  EXPECT_EQ(ctx2.maybeTransTable(), nullptr);

  TransTable* t1 = ctx1.transTable();
  ASSERT_NE(t1, nullptr);
  TransTable* t2 = ctx2.maybeTransTable();
  ASSERT_NE(t2, nullptr);
  EXPECT_EQ(t1, t2);

  // Dispose via one context should remove from registry
  ctx1.DisposeTransTable();
  EXPECT_EQ(ctx2.maybeTransTable(), nullptr);
}
