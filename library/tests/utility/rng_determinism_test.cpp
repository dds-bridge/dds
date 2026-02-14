/// @file rng_determinism_test.cpp
/// @brief Tests for RNG determinism with seed-based reproduction.
///
/// Validates that the RNG (mt19937) in SolverContext produces deterministic
/// sequences when seeded with the same value, and produces different sequences
/// with different seeds.

#include <random>
#include <vector>
#include <gtest/gtest.h>

#include <solver_context/solver_context.hpp>
#include "system/memory.hpp"

TEST(UtilitiesRngTest, DeterministicSequenceWithSeed) {
  // Set up a minimal ThreadData directly; TT is unused in this test.
  auto thr = std::make_shared<ThreadData>();

  // Seeded contexts should produce the same sequence for same seed.
  SolverConfig cfg{};
  cfg.rng_seed_ = 123456789ULL;

  SolverContext ctx1(thr, cfg);
  SolverContext ctx2(thr, cfg);

  // Generate a few values using uniform_int_distribution.
  std::uniform_int_distribution<int> dist(0, 1000000);
  std::vector<int> seq1, seq2;
  for (int i = 0; i < 10; ++i) {
    seq1.push_back(dist(ctx1.utilities().rng()));
    seq2.push_back(dist(ctx2.utilities().rng()));
  }

  EXPECT_EQ(seq1, seq2);
}

TEST(UtilitiesRngTest, DifferentSeedsYieldDifferentSequences) {
  auto thr = std::make_shared<ThreadData>();

  SolverConfig cfgA{}; cfgA.rng_seed_ = 42ULL;
  SolverConfig cfgB{}; cfgB.rng_seed_ = 43ULL;

  SolverContext ctxA(thr, cfgA);
  SolverContext ctxB(thr, cfgB);

  std::uniform_int_distribution<int> dist(0, 1000000);
  // Compare first few numbers; it's extremely likely to be different.
  int a0 = dist(ctxA.utilities().rng());
  int b0 = dist(ctxB.utilities().rng());
  // It's possible but astronomically unlikely they match; still assert inequality.
  EXPECT_NE(a0, b0);
}
