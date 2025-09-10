#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "test_utils.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "moves/Moves.h"

using namespace std::chrono_literals;

class HeuristicSortingCompareFixture : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}

  std::string run_and_serialize(const pos& tpos, moveType* moves, int numMoves, int trump) {
  moveType bestMove = {};
  moveType bestMoveTT = {};
  relRanksType thrp_rel[1] = {};
  trackType track = {};

  // Run heuristic (we keep timing locally but do not include it in the returned
  // string so comparisons don't fail due to minor timing differences).
  auto start = std::chrono::high_resolution_clock::now();
  CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
          0, trump, 0, &track, 1, 0, 0, 0);
  auto end = std::chrono::high_resolution_clock::now();
  (void)std::chrono::duration<double>(end - start);

  // Return ordering only for stable comparisons.
  std::string serialized = normalize_ordering(moves, numMoves, true);
  return serialized;
  }
};

TEST_F(HeuristicSortingCompareFixture, LegacyVsNewSinglePosition) {
#ifdef DDS_USE_NEW_HEURISTIC
  // Create a basic position (minimal data)
  pos tpos;
  memset(&tpos, 0, sizeof(tpos));
  // set some lengths to avoid degenerate cases
  tpos.length[0][0] = 4;
  tpos.length[1][0] = 3;
  tpos.length[2][0] = 3;
  tpos.length[3][0] = 3;

  const int numMoves = 5;
  moveType moves[numMoves];
  for (int i = 0; i < numMoves; ++i) {
    moves[i].suit = i % 4;
    moves[i].rank = 14 - i;
    moves[i].weight = 0;
    moves[i].sequence = i + 1;
  }

  // Ensure runtime switch exists (function provided when built with the define)
  set_use_new_heuristic(false);
  std::string legacy = run_and_serialize(tpos, moves, numMoves, 1);

  // Reset weights before rerunning
  for (int i = 0; i < numMoves; ++i) moves[i].weight = 0;

  set_use_new_heuristic(true);
  std::string neu = run_and_serialize(tpos, moves, numMoves, 1);

  if (legacy != neu) {
    // write logs for triage
    std::filesystem::create_directories("build/compare-results");
    auto ts = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(ts);
    std::ostringstream fname;
    fname << "build/compare-results/compare_" << t << ".legacy.json";
    write_json_log(fname.str(), legacy);
    fname.str(""); fname << "build/compare-results/compare_" << t << ".new.json";
    write_json_log(fname.str(), neu);
  }

  EXPECT_EQ(legacy, neu);
#else
  GTEST_SKIP() << "Runtime toggling not available: build with --define=use_new_heuristic=true";
#endif
}
