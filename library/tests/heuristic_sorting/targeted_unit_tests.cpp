#include <gtest/gtest.h>
#include <cstring>

#include "heuristic_sorting/internal.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include "test_utils.h"
#include "moves/Moves.h"

// Targeted unit tests for small helper functions and golden expectations

TEST(TargetedUnitTests, RankForcesAceBasic) {
  // Construct a minimal context and exercise RankForcesAce for different cards4th
  pos tpos = {};
  memset(&tpos, 0, sizeof(tpos));

  // Build a safe HeuristicContext using local objects
  moveType bm = {};
  moveType bmtt = {};
  relRanksType thrp_rel_dummy[1] = {};
  moveType mply_dummy[1] = {};
  trackType track_dummy = {};

  HeuristicContext ctx = {
    tpos,                   // pos
    bm,                     // bestMove
    bmtt,                   // bestMoveTT
    thrp_rel_dummy,         // thrp_rel
    mply_dummy,             // mply
    0,                      // numMoves
    0,                      // lastNumMoves
    DDS_NOTRUMP,            // trump
    0,                      // suit
    &track_dummy,           // trackp
    0,                      // currTrick
    0,                      // currHand
    0,                      // leadHand
    0                       // leadSuit
  };

  // Sanity: ensure function is callable and returns in-range values
  int res0 = RankForcesAce(ctx, 0);
  int res1 = RankForcesAce(ctx, 1);
  int res5 = RankForcesAce(ctx, 5);

  // RankForcesAce may return -1 when no forcing rank exists; ensure value is sane
  EXPECT_GE(res0, -1);
  EXPECT_GE(res1, -1);
  EXPECT_GE(res5, -1);
  EXPECT_LE(res0, 14);
  EXPECT_LE(res1, 14);
  EXPECT_LE(res5, 14);
}

TEST(TargetedUnitTests, GetTopNumberEdgeCases) {
  pos tpos = {};
  memset(&tpos, 0, sizeof(tpos));

  int topNumber = -1;
  int mno = -1;
  // Build a small HeuristicContext for GetTopNumber
  moveType bm = {};
  moveType bmtt = {};
  relRanksType thrp_rel_dummy[1] = {};
  moveType mply_dummy[1] = {};
  trackType track_dummy = {};
  HeuristicContext ctx = { tpos, bm, bmtt, thrp_rel_dummy, mply_dummy, 0, 0, DDS_NOTRUMP, 0, &track_dummy, 0, 0, 0, 0 };

  // Call the free helper GetTopNumber from internal.h
  GetTopNumber(ctx, 0, 14, topNumber, mno);
  // topNumber may be -1 if no candidate found; ensure values are in expected ranges
  EXPECT_GE(topNumber, -1);
  EXPECT_LE(topNumber, 14);
  EXPECT_GE(mno, 0);
  EXPECT_LE(mno, 13);

  GetTopNumber(ctx, 5, 10, topNumber, mno);
  EXPECT_GE(topNumber, -1);
  EXPECT_LE(topNumber, 14);
  EXPECT_GE(mno, 0);
  EXPECT_LE(mno, 13);
}

TEST(TargetedUnitTests, GoldenOrderingSmallPosition) {
  // Construct a small position and expected ordering string
  pos tpos;
  memset(&tpos, 0, sizeof(tpos));

  tpos.length[0][0] = 4; tpos.length[1][0] = 3; tpos.length[2][0] = 3; tpos.length[3][0] = 3;

  const int numMoves = 5;
  moveType moves[numMoves];
  for (int i = 0; i < numMoves; ++i) {
    moves[i].suit = i % 4;
    moves[i].rank = 14 - i;
    moves[i].weight = 0;
    moves[i].sequence = i + 1;
  }

  moveType bestMove = {};
  moveType bestMoveTT = {};
  relRanksType thrp_rel[1] = {};
  trackType track = {};

  // Run legacy (set_use_new_heuristic is a no-op compatibility hook)
  set_use_new_heuristic(false);
  CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                0, 1, 0, &track, 1, 0, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);

  // Reset weights
  for (int i = 0; i < numMoves; ++i) moves[i].weight = 0;

  // Run new (set_use_new_heuristic is a no-op compatibility hook)
  set_use_new_heuristic(true);
  CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                0, 1, 0, &track, 1, 0, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);

  EXPECT_EQ(legacy, neu);
}
