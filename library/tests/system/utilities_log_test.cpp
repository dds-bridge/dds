#include <gtest/gtest.h>
#include <solver_context/SolverContext.h>
#include "system/Memory.h"
#include <dds/dds.h>  // THREADMEM_* defaults

extern Memory memory;

static void ensureThread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesLogTest, NoLogWithoutDefine)
{
  ensureThread();
  SolverContext ctx;

  // Ensure clean start
  ctx.utilities().logClear();

  // Create TT and dispose it; without define there should be no logs
  (void)ctx.transTable();
  ctx.DisposeTransTable();

  EXPECT_TRUE(ctx.utilities().logBuffer().empty());
}
