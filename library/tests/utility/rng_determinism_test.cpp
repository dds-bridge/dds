#include "gtest/gtest.h"
#include <random>
#include <vector>
#include "system/SolverContext.h"
#include "system/Memory.h"

TEST(UtilitiesRngTest, DeterministicSequenceWithSeed) {
  // Set up a minimal ThreadData directly; TT is unused in this test.
  auto thr = std::make_shared<ThreadData>();

  // Seeded contexts should produce the same sequence for same seed.
  SolverConfig cfg{};
  cfg.rngSeed = 123456789ULL;

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

  SolverConfig cfgA{}; cfgA.rngSeed = 42ULL;
  SolverConfig cfgB{}; cfgB.rngSeed = 43ULL;

  SolverContext ctxA(thr, cfgA);
  SolverContext ctxB(thr, cfgB);

  std::uniform_int_distribution<int> dist(0, 1000000);
  // Compare first few numbers; it's extremely likely to be different.
  int a0 = dist(ctxA.utilities().rng());
  int b0 = dist(ctxB.utilities().rng());
  // It's possible but astronomically unlikely they match; still assert inequality.
  EXPECT_NE(a0, b0);
}
