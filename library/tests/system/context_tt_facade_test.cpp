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
