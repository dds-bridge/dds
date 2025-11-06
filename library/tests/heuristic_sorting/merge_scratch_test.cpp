#include <gtest/gtest.h>
#include "moves/Moves.hpp"

// Minimal harness: construct a Moves instance, populate a small move list with
// varying weights, call MergeSort via Sort(), and verify descending order.

TEST(MergeScratchTest, OrdersByWeightDescending) {
  Moves mv;
  const int trick = 12;
  const int relHand = 0;

  // Build a tiny list of moves with out-of-order weights
  movePlyType& list = mv.moveList[trick][relHand];
  list.last = 4;
  list.current = 0;
  // Initialize entries
  for (int i = 0; i <= list.last; ++i) {
    list.move[i].suit = 0;
    list.move[i].rank = i + 2;
    list.move[i].sequence = 0;
    list.move[i].weight = 0;
  }
  list.move[0].weight = 5;
  list.move[1].weight = 10;
  list.move[2].weight = 7;
  list.move[3].weight = 9;
  list.move[4].weight = 3;

  mv.Sort(trick, relHand);

  // Verify non-increasing order of weights
  int prev = list.move[0].weight;
  for (int i = 1; i <= list.last; ++i) {
    EXPECT_LE(list.move[i].weight, prev);
    prev = list.move[i].weight;
  }
}
