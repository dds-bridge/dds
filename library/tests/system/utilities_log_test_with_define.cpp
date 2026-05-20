/// @file utilities_log_test_with_define.cpp
/// @brief Tests for utilities logging with DDS_UTILITIES_LOG defined.
///
/// Validates that transposition table operations are properly logged
/// when DDS_UTILITIES_LOG is defined at compile time.

#include <gtest/gtest.h>
#include <solver_context/solver_context.hpp>
#include "system/memory.hpp"
#include <api/dds.h>

extern Memory memory;

static void ensure_thread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesLogTestWithDefine, LogsPresentWhenEnabled)
{
  ensure_thread();
  SolverContext ctx;
  ctx.utilities().log_clear();

  (void)ctx.trans_table();
  ctx.dispose_trans_table();

  const auto& logs = ctx.utilities().log_buffer();
  ASSERT_GE(logs.size(), 1u);
  // First log must be tt:create|K|def|max where K in {S,L}
  EXPECT_TRUE(logs[0].rfind("tt:create|", 0) == 0);
  // Last (or one of) should be dispose
  bool sawDispose = false;
  for (const auto& s : logs) {
    if (s == "tt:dispose") { sawDispose = true; break; }
  }
  EXPECT_TRUE(sawDispose);
}
