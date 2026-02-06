#include <gtest/gtest.h>
#include <solver_context/solver_context.hpp>
#include "system/memory.hpp"
#include <api/dds.h>  // THREADMEM_* defaults

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
  ctx.utilities().log_clear();

  // Create TT and dispose it; without define there should be no logs
  (void)ctx.trans_table();
  ctx.dispose_trans_table();

  EXPECT_TRUE(ctx.utilities().log_buffer().empty());
}
