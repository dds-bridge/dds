#include <gtest/gtest.h>
#include <cstring>
#include "dds/dll.h"
#include "system/Memory.h"
#include <solver_context/SolverContext.h>
#include <dds/dds.h>  // THREADMEM_* constants

extern Memory memory;

TEST(SystemContextTTFacades, ResetAndResizeAreNoopsWithoutTT)
{
  SetMaxThreads(1);
  const int thr = 0;
  // Some environments may compute 0 allowable threads (e.g., macOS sandbox),
  // so ensure we have at least one thread allocated for the test.
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
  // Create a context that owns its ThreadData for this test.
  SolverContext ctx;
  // Ensure no TT yet (construction is lazy until first use)
  ASSERT_EQ(nullptr, ctx.maybeTransTable());
  // Should not crash and should not create TT
  ctx.ResetForSolve();
  ctx.ClearTT();
  ctx.ResizeTT(8, 16);

  EXPECT_EQ(nullptr, ctx.maybeTransTable());
}

TEST(SystemContextTTFacades, ResizeCreatesWhenExisting)
{
  SetMaxThreads(1);
  const int thr = 0;
  // Ensure at least one thread exists; fall back to a small thread config.
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
  // Use owned context for the test
  SolverContext ctx;
  // Force create via transTable()
  auto* tt = ctx.transTable();
  ASSERT_NE(nullptr, tt);

  // Resize should apply immediately and keep TT alive
  ctx.ResizeTT(8, 16);
  EXPECT_NE(nullptr, ctx.maybeTransTable());
}

TEST(SystemContextTTFacades, Lifecycle_LookupAddClearDispose)
{
  SetMaxThreads(1);
  const int thr = 0;
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

  SolverContext ctx;

  // Create TT and perform an initial lookup (expect miss)
  auto* tt = ctx.transTable();
  ASSERT_NE(nullptr, tt);

  // Ensure TT internal roots are initialized before Lookup/Add for the test.
  // Production resets happen in SolverIF around new deals/trumps.
  ctx.ResetForSolve();

  // Minimal initialization for TT internals (aggr tables)
  int handLookup[DDS_SUITS][15] = {};
  // Leave all zeros (map ranks to North=0) which is sufficient for basic TT wiring
  tt->init(handLookup);

  const int trick = 11; // any valid trick index in [1..11] per implementation
  const int hand = 0;   // North
  unsigned short aggrTarget[DDS_HANDS] = {0, 0, 0, 0};
  int handDist[DDS_HANDS] = {0, 0, 0, 0}; // 0 spades/hearts/diamonds; clubs inferred
  bool lowerFlag = false;

  // Miss before any Add
  auto* missNode = tt->lookup(trick, hand, aggrTarget, handDist, /*limit*/0, lowerFlag);
  EXPECT_EQ(nullptr, missNode);

  // Add a minimal node for the same suit distribution so subsequent Lookup hits
  NodeCards first{};
  first.lower_bound = 0;
  first.upper_bound = 0;
  first.best_move_suit = 0;
  first.best_move_rank = 0;
  std::memset(first.least_win, 0, sizeof(first.least_win));

  unsigned short ourWinRanks[DDS_HANDS] = {0, 0, 0, 0};
  tt->add(trick, hand, aggrTarget, ourWinRanks, first, /*flag*/false);

  // Hit now (bounds allow returning the stored node)
  auto* hitNode = tt->lookup(trick, hand, aggrTarget, handDist, /*limit*/0, lowerFlag);
  ASSERT_NE(nullptr, hitNode);
  EXPECT_EQ(0, static_cast<int>(hitNode->lower_bound));
  EXPECT_EQ(0, static_cast<int>(hitNode->upper_bound));

  // Dispose destroys the TT instance from the registry
  ctx.DisposeTransTable();
  EXPECT_EQ(nullptr, ctx.maybeTransTable());
}
