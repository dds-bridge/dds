#include <gtest/gtest.h>
#include <cstring>

#include "test_utils.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "moves/Moves.h"

// Helper to run heuristic and get normalized ordering (re-implemented locally)
static std::string run_and_serialize_once(const pos& tpos, moveType* moves, int numMoves, int trump) {
  moveType bestMove = {};
  moveType bestMoveTT = {};
  static relRanksType relTable[8192];
  trackType track = {};
  init_rel_and_track(tpos, relTable, &track);

  CallHeuristic(tpos, bestMove, bestMoveTT, relTable, moves, numMoves,
                0, trump, 0, &track, 1, 0, 0, 0);
  return normalize_ordering(moves, numMoves, true);
}

TEST(CanonicalCases, AllMatchLegacyNew) {
  const int numCases = 6;
  for (int c = 0; c < numCases; ++c) {
    pos tpos;
    memset(&tpos, 0, sizeof(tpos));

    // Make small variations across cases
    switch (c) {
      case 0:
        tpos.length[0][0] = 4; tpos.length[1][0] = 3; tpos.length[2][0] = 3; tpos.length[3][0] = 3; break;
      case 1:
        tpos.length[0][0] = 5; tpos.length[1][0] = 4; tpos.length[2][0] = 2; tpos.length[3][0] = 2; break;
      case 2:
        tpos.length[0][1] = 4; tpos.length[1][1] = 3; tpos.length[2][1] = 3; tpos.length[3][1] = 3; break;
      case 3:
        tpos.length[0][2] = 1; tpos.length[1][2] = 1; tpos.length[2][2] = 1; tpos.length[3][2] = 1; break;
      case 4:
        tpos.length[0][3] = 7; tpos.length[1][3] = 6; tpos.length[2][3] = 0; tpos.length[3][3] = 0; break;
      case 5:
        // a near-empty hand
        tpos.length[0][0] = 1; tpos.length[1][0] = 0; tpos.length[2][0] = 0; tpos.length[3][0] = 0; break;
    }

    const int numMoves = 6;
    moveType moves[numMoves];
    for (int i = 0; i < numMoves; ++i) {
      moves[i].suit = i % 4;
      moves[i].rank = 14 - i;
      moves[i].weight = 0;
      moves[i].sequence = i + 1;
    }

#ifdef DDS_USE_NEW_HEURISTIC
    set_use_new_heuristic(false);
    std::string legacy = run_and_serialize_once(tpos, moves, numMoves, 1);
    for (int i = 0; i < numMoves; ++i) moves[i].weight = 0;
    set_use_new_heuristic(true);
    std::string neu = run_and_serialize_once(tpos, moves, numMoves, 1);
    if (legacy != neu) {
      // write diagnostics
      write_json_log("build/compare-results/canonical_legacy.json", legacy);
      write_json_log("build/compare-results/canonical_new.json", neu);
    }
    EXPECT_EQ(legacy, neu);
#else
    GTEST_SKIP() << "Runtime toggling not available: build with --define=new_heuristic=true";
#endif
  }
}
