/// @file utilities_log_test.cpp
/// @brief Tests for utilities logging without DDS_UTILITIES_LOG define.
///
/// Validates that no log entries are recorded when DDS_UTILITIES_LOG
/// is not defined at compile time.

#include <gtest/gtest.h>
#include <solver_context/solver_context.hpp>
#include "system/memory.hpp"
#include <api/dds.h>  // THREADMEM_* defaults

extern Memory memory;

static void ensure_thread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesLogTest, NoLogWithoutDefine)
{
  ensure_thread();
  SolverContext ctx;

  // Ensure clean start
  ctx.utilities().log_clear();

  // Create TT and dispose it; without define there should be no logs
  (void)ctx.trans_table();
  ctx.dispose_trans_table();

  EXPECT_TRUE(ctx.utilities().log_buffer().empty());
}
