#include <gtest/gtest.h>
#include <cstring>
#include "dds/dll.h"
#include "system/Memory.h"
#include "system/SolverContext.h"
#include "data_types/dds.h"  // THREADMEM_* constants

extern Memory memory;

TEST(SystemContextTTFacades, ResetAndResizeAreNoopsWithoutTT)
{
  SetMaxThreads(1);
  const int thr = 0;
  // Some environments may compute 0 allowable threads (e.g., macOS sandbox),
  // so ensure we have at least one thread allocated for the test.
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
  ThreadData* tdp = memory.GetPtr(static_cast<unsigned>(thr));
  // Ensure no TT yet (construction is lazy until first use)
  {
    SolverContext pre{tdp};
    ASSERT_EQ(nullptr, pre.maybeTransTable());
  }

  SolverContext ctx{tdp};
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
  ThreadData* tdp = memory.GetPtr(static_cast<unsigned>(thr));
  // Force create via transTable()
  SolverContext ctx{tdp};
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

  ThreadData* tdp = memory.GetPtr(static_cast<unsigned>(thr));
  SolverContext ctx{tdp};

  // Create TT and perform an initial lookup (expect miss)
  auto* tt = ctx.transTable();
  ASSERT_NE(nullptr, tt);

  // Ensure TT internal roots are initialized before Lookup/Add for the test.
  // Production resets happen in SolverIF around new deals/trumps.
  ctx.ResetForSolve();

  // Minimal initialization for TT internals (aggr tables)
  int handLookup[DDS_SUITS][15] = {};
  // Leave all zeros (map ranks to North=0) which is sufficient for basic TT wiring
  tt->Init(handLookup);

  const int trick = 11; // any valid trick index in [1..11] per implementation
  const int hand = 0;   // North
  unsigned short aggrTarget[DDS_HANDS] = {0, 0, 0, 0};
  int handDist[DDS_HANDS] = {0, 0, 0, 0}; // 0 spades/hearts/diamonds; clubs inferred
  bool lowerFlag = false;

  // Miss before any Add
  auto* missNode = tt->Lookup(trick, hand, aggrTarget, handDist, /*limit*/0, lowerFlag);
  EXPECT_EQ(nullptr, missNode);

  // Add a minimal node for the same suit distribution so subsequent Lookup hits
  nodeCardsType first{};
  first.lbound = 0;
  first.ubound = 0;
  first.bestMoveSuit = 0;
  first.bestMoveRank = 0;
  std::memset(first.leastWin, 0, sizeof(first.leastWin));

  unsigned short ourWinRanks[DDS_HANDS] = {0, 0, 0, 0};
  tt->Add(trick, hand, aggrTarget, ourWinRanks, first, /*flag*/false);

  // Hit now (bounds allow returning the stored node)
  auto* hitNode = tt->Lookup(trick, hand, aggrTarget, handDist, /*limit*/0, lowerFlag);
  ASSERT_NE(nullptr, hitNode);
  EXPECT_EQ(0, static_cast<int>(hitNode->lbound));
  EXPECT_EQ(0, static_cast<int>(hitNode->ubound));

  // Dispose destroys the TT instance from the registry
  ctx.DisposeTransTable();
  EXPECT_EQ(nullptr, ctx.maybeTransTable());
}
