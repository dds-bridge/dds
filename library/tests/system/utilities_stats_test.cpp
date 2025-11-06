#include <gtest/gtest.h>
#include <solver_context/SolverContext.hpp>
#include "system/Memory.hpp"
#include <api/dds.h>

extern Memory memory;

static void ensureThread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesStatsTest, CountersRemainZeroWithoutDefine)
{
  ensureThread();
  SolverContext ctx;
  ctx.utilities().util().stats_reset();

  (void)ctx.transTable();
  ctx.DisposeTransTable();

  const auto& st = ctx.utilities().util().stats();
  EXPECT_EQ(0u, st.tt_creates);
  EXPECT_EQ(0u, st.tt_disposes);
}
