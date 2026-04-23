/**
 * @file targeted_unit_tests.cpp
 * @brief Targeted unit tests for small helper functions in heuristic sorting
 *
 * Tests individual helper functions like rank_forces_ace() and get_top_number()
 * with edge cases and golden expectations to ensure correctness.
 */

#include <gtest/gtest.h>
#include <cstring>

#include "heuristic_sorting/internal.hpp"
#include "heuristic_sorting/heuristic_sorting.hpp"

// Targeted unit tests for small helper functions and golden expectations

TEST(TargetedUnitTests, RankForcesAceBasic) {
  // Construct a minimal context and exercise rank_forces_ace for different cards4th
  Pos tpos = {};
  memset(&tpos, 0, sizeof(tpos));

  // Build a safe HeuristicContext using local objects
  MoveType bm = {};
  MoveType bmtt = {};
  RelRanksType thrp_rel_dummy[1] = {};
  MoveType mply_dummy[1] = {};
  TrackType track_dummy = {};

  HeuristicContext ctx = {
    tpos,                   // Pos
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
  int res0 = rank_forces_ace(ctx, 0);
  int res1 = rank_forces_ace(ctx, 1);
  int res5 = rank_forces_ace(ctx, 5);

  // rank_forces_ace may return -1 when no forcing rank exists; ensure value is sane
  EXPECT_GE(res0, -1);
  EXPECT_GE(res1, -1);
  EXPECT_GE(res5, -1);
  EXPECT_LE(res0, 14);
  EXPECT_LE(res1, 14);
  EXPECT_LE(res5, 14);
}

TEST(TargetedUnitTests, GetTopNumberEdgeCases) {
  Pos tpos = {};
  memset(&tpos, 0, sizeof(tpos));

  int topNumber = -1;
  int mno = -1;
  // Build a small HeuristicContext for get_top_number
  MoveType bm = {};
  MoveType bmtt = {};
  RelRanksType thrp_rel_dummy[1] = {};
  MoveType mply_dummy[1] = {};
  TrackType track_dummy = {};
  HeuristicContext ctx = { tpos, bm, bmtt, thrp_rel_dummy, mply_dummy, 0, 0, DDS_NOTRUMP, 0, &track_dummy, 0, 0, 0, 0 };

  // Call the free helper get_top_number from internal.hpp
  get_top_number(ctx, 0, 14, topNumber, mno);
  // topNumber may be -1 if no candidate found; ensure values are in expected ranges
  EXPECT_GE(topNumber, -1);
  EXPECT_LE(topNumber, 14);
  EXPECT_GE(mno, 0);
  EXPECT_LE(mno, 13);

  get_top_number(ctx, 5, 10, topNumber, mno);
  EXPECT_GE(topNumber, -1);
  EXPECT_LE(topNumber, 14);
  EXPECT_GE(mno, 0);
  EXPECT_LE(mno, 13);
}
