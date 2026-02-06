#include <gtest/gtest.h>
#include <solver_context/solver_context.hpp>
#include "system/memory.hpp"
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
  ctx.utilities().log_clear();

  // Exercise the context operations (should not produce log entries by default)
  ctx.reset_for_solve();
  ctx.reset_best_moves_lite();
  ctx.resize_tt(8, 16);
  ctx.clear_tt();

  EXPECT_TRUE(ctx.utilities().log_buffer().empty());
}
