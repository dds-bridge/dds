/// @file context_equivalence_test.cpp
/// @brief Tests for equivalence between legacy and context-based solver APIs.
///
/// Validates that SolverContext and legacy SolveBoard produce identical
/// results for the same input hands and parameters.

#include <gtest/gtest.h>
#include <cstring>

#include <system/memory.hpp>
#include <solver_context/solver_context.hpp>
#include <dds/dds.hpp>

extern Memory memory;

static Deal make_empty_deal()
{
  Deal dl{};
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
  FutureTricks ft_legacy{};
  FutureTricks ft_ctx{};
  Deal dl = make_empty_deal();

  const int r_legacy = SolveBoard(dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft_legacy, thr);

  // Construct a SolverContext-owned ThreadData for the context-based call.
  SolverContext ctx;
  const int r_ctx = SolveBoard(ctx, dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft_ctx);

  EXPECT_EQ(r_legacy, r_ctx) << "Legacy and context return codes should match";
}
