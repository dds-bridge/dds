#include <gtest/gtest.h>
#include "system/SolverContext.h"
#include "system/Memory.h"

extern Memory memory;

static void ensureThread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(ArenaReuseTest, SameThreadSharesArenaAndResets)
{
  ensureThread();
  // Create an owning context to provide a ThreadData for the test.
  SolverContext owner;
  ThreadData* thr = owner.thread();

  // Two contexts pointing to the same ThreadData
  SolverContext ctx1{thr, SolverConfig{.arenaCapacityBytes = 4096}};
  SolverContext ctx2{thr, SolverConfig{.arenaCapacityBytes = 4096}};

  auto* a1 = ctx1.arena();
  auto* a2 = ctx2.arena();
  // Same underlying Arena instance via per-thread registry
  EXPECT_EQ(a1, a2);

  // Allocate via first context and verify offset advances
  if (a1) {
    auto off0 = a1->offset();
    void* p = a1->allocate({128});
    ASSERT_NE(p, nullptr);
    EXPECT_GT(a1->offset(), off0);
  }

  // Reset via second context and verify offset resets to zero
  ctx2.ResetForSolve();
  if (a2) {
    EXPECT_EQ(a2->offset(), 0u);
    EXPECT_EQ(a2->fallback_count(), 0u);
  }
}

TEST(ArenaReuseTest, DifferentThreadsGetDifferentArenas)
{
  // Ensure at least 2 thread slots
  if (memory.NumThreads() < 2)
    memory.Resize(2, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

  SolverContext owner0;
  ThreadData* thr = owner0.thread();
  SolverContext owner1;
  ThreadData* thr0 = owner0.thread();
  ThreadData* thr1 = owner1.thread();
  SolverContext ctx0{thr0, SolverConfig{.arenaCapacityBytes = 2048}};
  SolverContext ctx1{thr1, SolverConfig{.arenaCapacityBytes = 2048}};

  auto* a0 = ctx0.arena();
  auto* a1 = ctx1.arena();

  // Distinct Arena instances across different ThreadData
  if (a0 && a1) {
    EXPECT_NE(a0, a1);
  }
}
