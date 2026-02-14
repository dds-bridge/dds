/// @file utilities_log_ctx_ops_test_with_define.cpp
/// @brief Tests for SolverContext operations logging with DDS_UTILITIES_LOG.
///
/// Validates that context and transposition table operations are properly
/// logged when DDS_UTILITIES_LOG is defined.

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

TEST(UtilitiesLogCtxOpsWithDefine, EmitsCtxAndTTOps)
{
  ensure_thread();
  SolverContext ctx;

  // Start from a clean log buffer
  ctx.utilities().log_clear();

  // Exercise the newly logged operations
  ctx.reset_for_solve();
  ctx.reset_best_moves_lite();
  ctx.resize_tt(8, 16);
  ctx.clear_tt();

  const auto& logs = ctx.utilities().log_buffer();
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
