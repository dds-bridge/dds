#include <gtest/gtest.h>
#include <cstring>

#include "dds/dll.h"
#include "system/SolverContext.h"
#include "system/Memory.h"
#include "dds/SolverIF.h"

extern Memory memory;

static deal make_empty_deal()
{
  deal dl{};
  dl.trump = 0;
  dl.first = 0;
  std::memset(dl.currentTrickSuit, 0, sizeof(dl.currentTrickSuit));
  std::memset(dl.currentTrickRank, 0, sizeof(dl.currentTrickRank));
  std::memset(dl.remainCards, 0, sizeof(dl.remainCards));
  return dl;
}

TEST(SystemContextEquivalence, LegacyVsContextReturnCode)
{
  // Ensure DDS system and thread-local memory are initialized
  SetMaxThreads(1);
  const int thr = 0;
  futureTricks ft_legacy{};
  futureTricks ft_ctx{};
  deal dl = make_empty_deal();

  const int r_legacy = SolveBoard(dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft_legacy, thr);

  // Construct a SolverContext-owned ThreadData for the context-based call.
  SolverContext ctx;
  const int r_ctx = SolveBoardWithContext(ctx, dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft_ctx);

  EXPECT_EQ(r_legacy, r_ctx) << "Legacy and context return codes should match";
}
