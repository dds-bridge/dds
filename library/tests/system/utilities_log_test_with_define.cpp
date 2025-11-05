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

TEST(UtilitiesLogTestWithDefine, LogsPresentWhenEnabled)
{
  ensureThread();
  SolverContext ctx;
  ctx.utilities().logClear();

  (void)ctx.transTable();
  ctx.DisposeTransTable();

  const auto& logs = ctx.utilities().logBuffer();
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
