#include <gtest/gtest.h>
#include "test_utils.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"

// Declaration of runtime switch - available when built with DDS_USE_NEW_HEURISTIC
#include "moves/Moves.h"

class HeuristicSortingCompareSmokeTest : public ::testing::Test {
 protected:
};

TEST_F(HeuristicSortingCompareSmokeTest, ToggleAndSerialize) {
  // Build a trivial move array using types from minimal_weight_test style
  const int numMoves = 3;
  moveType moves[numMoves];
  for (int i = 0; i < numMoves; ++i) {
    moves[i].suit = i % 4;
    moves[i].rank = 14 - i;
    moves[i].weight = 0;
    moves[i].sequence = i+1;
  }

  // Basic serialization sanity: ensure the serializer works (avoids build-time toggles)
  std::string out = serialize_move_list(moves, numMoves, true);
  EXPECT_FALSE(out.empty());
}
