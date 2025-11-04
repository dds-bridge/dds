#include <gtest/gtest.h>
#include "system/SolverContext.h"
#include "system/Memory.h"
#include <dds/dds.h>

extern Memory memory;

static void ensureThread()
{
  if (memory.NumThreads() == 0)
    memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
}

TEST(UtilitiesStatsTestWithDefine, CountersIncreaseWhenEnabled)
{
  ensureThread();
  SolverContext ctx;
  ctx.utilities().util().stats_reset();

  (void)ctx.transTable();
  ctx.DisposeTransTable();

  const auto& st = ctx.utilities().util().stats();
  EXPECT_EQ(1u, st.tt_creates);
  EXPECT_EQ(1u, st.tt_disposes);
}
