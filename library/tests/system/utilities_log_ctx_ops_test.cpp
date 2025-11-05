#include <gtest/gtest.h>
#include <solver_context/SolverContext.h>
#include "system/Memory.h"
#include <api/dds.h>

extern Memory memory;

static void ensureThread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesLogCtxOpsNoDefine, NoEntriesWhenDisabled)
{
  ensureThread();
  SolverContext ctx;

  // Start from a clean log buffer
  ctx.utilities().logClear();

  // Exercise the context operations (should not produce log entries by default)
  ctx.ResetForSolve();
  ctx.ResetBestMovesLite();
  ctx.ResizeTT(8, 16);
  ctx.ClearTT();

  EXPECT_TRUE(ctx.utilities().logBuffer().empty());
}
