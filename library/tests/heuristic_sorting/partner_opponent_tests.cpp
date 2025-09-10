#include <gtest/gtest.h>
#include <cstring>

#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "test_utils.h"
#include "moves/Moves.h"

// Partner and opponent focused golden tests.

static void make_moves(moveType* moves, int numMoves) {
  for (int i = 0; i < numMoves; ++i) {
    moves[i].suit = i % 4;
    moves[i].rank = 14 - (i % 13);
    moves[i].weight = 0;
    moves[i].sequence = i + 1;
  }
}

TEST(PartnerOpponent, LHO_has_singleton_and_top) {
  pos tpos; memset(&tpos, 0, sizeof(tpos));
  // LHO (hand 1) has singleton top in suit 0
  tpos.length[1][0] = 1; tpos.length[0][0] = 4; tpos.length[2][0] = 4; tpos.length[3][0] = 4;
  tpos.winner[0].hand = 1; tpos.winner[0].rank = 14;

  const int numMoves = 6; moveType moves[numMoves]; make_moves(moves, numMoves);
  moveType bm = {}; moveType bmtt = {}; relRanksType rel[1] = {}; trackType track = {};

  set_use_new_heuristic(false);
  CallHeuristic(tpos, bm, bmtt, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);
  for (int i=0;i<numMoves;++i) moves[i].weight = 0;
  set_use_new_heuristic(true);
  CallHeuristic(tpos, bm, bmtt, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);
  EXPECT_EQ(legacy, neu);
}

TEST(PartnerOpponent, Partner_has_second_highest) {
  pos tpos; memset(&tpos, 0, sizeof(tpos));
  // Partner (hand 2) has second highest in suit 1, hand-to-play is 0
  tpos.length[2][1] = 3; tpos.length[0][1] = 3; tpos.length[1][1] = 3; tpos.length[3][1] = 3;
  tpos.winner[1].hand = 2; tpos.winner[1].rank = 13; // second highest

  const int numMoves = 5; moveType moves[numMoves]; make_moves(moves, numMoves);
  moveType bm = {}; moveType bmtt = {}; relRanksType rel[1] = {}; trackType track = {};

  set_use_new_heuristic(false);
  CallHeuristic(tpos, bm, bmtt, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);
  for (int i=0;i<numMoves;++i) moves[i].weight = 0;
  set_use_new_heuristic(true);
  CallHeuristic(tpos, bm, bmtt, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);
  EXPECT_EQ(legacy, neu);
}

TEST(PartnerOpponent, RHO_raises_and_wins) {
  pos tpos; memset(&tpos, 0, sizeof(tpos));
  // RHO (hand 3) will have a raising card in suit 2
  tpos.length[3][2] = 2; tpos.length[0][2] = 4; tpos.length[1][2] = 4; tpos.length[2][2] = 3;
  tpos.winner[2].hand = 3; tpos.winner[2].rank = 12;

  const int numMoves = 6; moveType moves[numMoves]; make_moves(moves, numMoves);
  moveType bm = {}; moveType bmtt = {}; relRanksType rel[1] = {}; trackType track = {};

  set_use_new_heuristic(false);
  CallHeuristic(tpos, bm, bmtt, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);
  for (int i=0;i<numMoves;++i) moves[i].weight = 0;
  set_use_new_heuristic(true);
  CallHeuristic(tpos, bm, bmtt, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);
  EXPECT_EQ(legacy, neu);
}
