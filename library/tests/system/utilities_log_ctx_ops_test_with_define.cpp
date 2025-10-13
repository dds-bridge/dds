#include <gtest/gtest.h>
#include "system/SolverContext.h"
#include "system/Memory.h"
#include "data_types/dds.h"

extern Memory memory;

static void ensureThread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesLogCtxOpsWithDefine, EmitsCtxAndTTOps)
{
  ensureThread();
  ThreadData* thr = memory.GetPtr(0);
  SolverContext ctx{thr};

  // Start from a clean log buffer
  ctx.utilities().logClear();

  // Exercise the newly logged operations
  ctx.ResetForSolve();
  ctx.ResetBestMovesLite();
  ctx.ResizeTT(8, 16);
  ctx.ClearTT();

  const auto& logs = ctx.utilities().logBuffer();
  // We expect at least the four entries we just invoked
  ASSERT_GE(logs.size(), 4u);

  bool sawResetForSolve = false;
  bool sawResetBestMovesLite = false;
  bool sawResize = false;
  bool sawClear = false;
  for (const auto& s : logs) {
    if (s == "ctx:reset_for_solve") sawResetForSolve = true;
    if (s == "ctx:reset_best_moves_lite") sawResetBestMovesLite = true;
    if (s.rfind("tt:resize|", 0) == 0) sawResize = true;
    if (s == "tt:clear") sawClear = true;
  }

  EXPECT_TRUE(sawResetForSolve);
  EXPECT_TRUE(sawResetBestMovesLite);
  EXPECT_TRUE(sawResize);
  EXPECT_TRUE(sawClear);
}
