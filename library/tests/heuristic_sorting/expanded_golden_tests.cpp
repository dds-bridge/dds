#include <gtest/gtest.h>
#include <cstring>

#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "test_utils.h"
#include "moves/Moves.h"

// Expanded golden tests: several handcrafted positions to exercise different
// branches in the heuristic logic (lead, void, trump, mid-trick).

static void make_moves(moveType* moves, int numMoves) {
  for (int i = 0; i < numMoves; ++i) {
    moves[i].suit = i % 4;
    moves[i].rank = 14 - (i % 13);
    moves[i].weight = 0;
    moves[i].sequence = i + 1;
  }
}

TEST(ExpandedGolden, LeadAndVoidCases) {
  pos tpos; memset(&tpos, 0, sizeof(tpos));
  // North has a singleton in hearts, others balanced
  tpos.length[0][1] = 1; tpos.length[1][1] = 4; tpos.length[2][1] = 4; tpos.length[3][1] = 4;
  // some winners
  tpos.winner[0].rank = 14; tpos.winner[0].hand = 0;

  const int numMoves = 6;
  moveType moves[numMoves]; make_moves(moves, numMoves);

  moveType bestMove = {}; moveType bestMoveTT = {}; relRanksType rel[1] = {}; trackType track = {};

  // Runtime toggle removed; previous call to set_use_new_heuristic(false) omitted.
  CallHeuristic(tpos, bestMove, bestMoveTT, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);

  for (int i = 0; i < numMoves; ++i) moves[i].weight = 0;

  // Runtime toggle removed; previous call to set_use_new_heuristic(true) omitted.
  CallHeuristic(tpos, bestMove, bestMoveTT, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);

  EXPECT_EQ(legacy, neu);
}

TEST(ExpandedGolden, MidTrickRuffing) {
  pos tpos; memset(&tpos, 0, sizeof(tpos));
  // Set up a small deal where some cards already played
  for (int h = 0; h < 4; ++h) for (int s = 0; s < 4; ++s) tpos.length[h][s] = 3;
  tpos.rankInSuit[0][2] = 0x7000; // A K Q diamonds on hand 0

  const int numMoves = 5;
  moveType moves[numMoves]; make_moves(moves, numMoves);

  moveType bestMove = {}; moveType bestMoveTT = {}; relRanksType rel[8192]; trackType track = {};
  init_rel_and_track(tpos, rel, &track, 2, moves, 0, 1);

  // Runtime toggle removed; previous call to set_use_new_heuristic(false) omitted.
  CallHeuristic(tpos, bestMove, bestMoveTT, rel, moves, numMoves, 2, 1, 0, &track, 1, 2, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);

  for (int i = 0; i < numMoves; ++i) moves[i].weight = 0;

  // Runtime toggle removed; previous call to set_use_new_heuristic(true) omitted.
  CallHeuristic(tpos, bestMove, bestMoveTT, rel, moves, numMoves, 2, 1, 0, &track, 1, 2, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);

  EXPECT_EQ(legacy, neu);
}

TEST(ExpandedGolden, LargeLengths) {
  pos tpos; memset(&tpos, 0, sizeof(tpos));
  // Test with long suit lengths to exercise count-based logic
  tpos.length[0][3] = 7; tpos.length[1][3] = 6; tpos.length[2][3] = 4; tpos.length[3][3] = 2;

  const int numMoves = 7;
  moveType moves[numMoves]; make_moves(moves, numMoves);

  moveType bestMove = {}; moveType bestMoveTT = {}; relRanksType rel[1] = {}; trackType track = {};

  // Runtime toggle removed; previous call to set_use_new_heuristic(false) omitted.
  CallHeuristic(tpos, bestMove, bestMoveTT, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string legacy = normalize_ordering(moves, numMoves, true);

  for (int i = 0; i < numMoves; ++i) moves[i].weight = 0;

  // Runtime toggle removed; previous call to set_use_new_heuristic(true) omitted.
  CallHeuristic(tpos, bestMove, bestMoveTT, rel, moves, numMoves, 0, 1, 0, &track, 1, 0, 0, 0);
  std::string neu = normalize_ordering(moves, numMoves, true);

  EXPECT_EQ(legacy, neu);
}
