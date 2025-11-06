#include <gtest/gtest.h>
#include <cstring>

#include "heuristic_sorting/internal.hpp"
#include "heuristic_sorting/heuristic_sorting.hpp"
#include "test_utils.hpp"
#include "moves/Moves.hpp"

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

  // Call the free helper GetTopNumber from internal.hpp
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
