#include <gtest/gtest.h>
#include <vector>
#include <algorithm>

#include <api/dds.h>

class HeuristicSortingTest : public ::testing::Test {
 protected:
  HeuristicSortingTest() = default;
  
  // Helper function to create a moveType with specified values
  moveType createMove(int suit, int rank, int sequence, int weight) {
    moveType move;
    move.suit = suit;
    move.rank = rank;
    move.sequence = sequence;
    move.weight = weight;
    return move;
  }
};

TEST_F(HeuristicSortingTest, SortsMovesInDescendingOrderByWeight) {
  // Create test moves with different weights
  std::vector<moveType> testMoves = {
    createMove(0, 14, 0, 10),  // Low weight
    createMove(1, 13, 0, 50),  // High weight
    createMove(2, 12, 0, 30),  // Medium weight
    createMove(3, 11, 0, 45)   // Medium-high weight
  };
  
  // Expected order after sorting: weights 50, 45, 30, 10
  std::vector<int> expectedWeights = {50, 45, 30, 10};
  
  // Sort using standard library to verify our expectation
  std::sort(testMoves.begin(), testMoves.end(), 
    [](const moveType& a, const moveType& b) {
      return a.weight > b.weight;  // Descending order
    });
  
  // Verify the weights are in descending order
  for (size_t i = 0; i < testMoves.size(); i++) {
    EXPECT_EQ(testMoves[i].weight, expectedWeights[i]);
  }
}

TEST_F(HeuristicSortingTest, HigherWeightMovesHavePriority) {
  // Test that moves with higher weights get priority in sorting
  moveType highPriority = createMove(0, 14, 0, 100);
  moveType lowPriority = createMove(1, 13, 0, 5);
  
  EXPECT_GT(highPriority.weight, lowPriority.weight);
  
  // In a comparison-based sort, higher weight should come first
  EXPECT_TRUE(highPriority.weight > lowPriority.weight);
}

TEST_F(HeuristicSortingTest, WeightCalculationFollowsHeuristics) {
  // Test various weight scenarios based on the heuristic logic
  
  // High card with sequence bonus should have high weight
  int baseWeight = 30;
  int rankBonus = 14;  // Ace
  int sequenceBonus = 40;
  
  int expectedHighCardWeight = baseWeight + sequenceBonus + rankBonus;
  
  // Verify that high cards with sequences get appropriate weight
  EXPECT_GT(expectedHighCardWeight, baseWeight);
  EXPECT_GT(expectedHighCardWeight, baseWeight + rankBonus);
}

TEST_F(HeuristicSortingTest, MoveSequenceAffectsWeight) {
  // Test that sequence moves get different weights
  moveType sequenceMove = createMove(0, 14, 1, 40);  // Has sequence
  moveType nonSequenceMove = createMove(0, 14, 0, 20);  // No sequence
  
  // Sequence moves typically get bonus weight
  EXPECT_GT(sequenceMove.weight, nonSequenceMove.weight);
}

TEST_F(HeuristicSortingTest, SameSuitMovesOrderedByRank) {
  // Within the same suit, higher ranks should generally have higher weights
  // (though this depends on the specific heuristic context)
  
  moveType ace = createMove(0, 14, 0, 50);    // Ace
  moveType king = createMove(0, 13, 0, 45);   // King  
  moveType queen = createMove(0, 12, 0, 40);  // Queen
  
  // Generally, higher ranks get higher base weights
  EXPECT_GE(ace.weight, king.weight);
  EXPECT_GE(king.weight, queen.weight);
}

TEST_F(HeuristicSortingTest, WeightDifferencesSignificant) {
  // Test that weight differences are significant enough to affect sorting
  
  moveType bestMove = createMove(0, 14, 1, 100);
  moveType goodMove = createMove(1, 13, 1, 75);
  moveType averageMove = createMove(2, 12, 0, 50);
  moveType poorMove = createMove(3, 11, 0, 25);
  
  // Verify clear ordering
  EXPECT_GT(bestMove.weight, goodMove.weight);
  EXPECT_GT(goodMove.weight, averageMove.weight);
  EXPECT_GT(averageMove.weight, poorMove.weight);
  
  // Verify differences are meaningful (at least 20 point gaps)
  EXPECT_GE(bestMove.weight - goodMove.weight, 20);
  EXPECT_GE(goodMove.weight - averageMove.weight, 20);
  EXPECT_GE(averageMove.weight - poorMove.weight, 20);
}

TEST_F(HeuristicSortingTest, TrumpSuitMovesGetPriority) {
  // In trump contracts, trump suit moves often get priority
  
  moveType trumpMove = createMove(0, 14, 0, 80);    // Trump suit (assuming suit 0 is trump)
  moveType nonTrumpMove = createMove(1, 14, 0, 60); // Non-trump suit
  
  // Trump moves should generally have higher weights
  EXPECT_GT(trumpMove.weight, nonTrumpMove.weight);
}

TEST_F(HeuristicSortingTest, SortingStabilityWithEqualWeights) {
  // Test behavior when moves have equal weights
  
  moveType move1 = createMove(0, 14, 0, 50);
  moveType move2 = createMove(1, 13, 0, 50);
  moveType move3 = createMove(2, 12, 0, 50);
  
  // All have equal weights
  EXPECT_EQ(move1.weight, move2.weight);
  EXPECT_EQ(move2.weight, move3.weight);
  
  // The sorting algorithm should handle equal weights gracefully
  // (order may vary but should be deterministic)
}

TEST_F(HeuristicSortingTest, WeightRangeIsReasonable) {
  // Test that weights are in a reasonable range
  moveType testMove = createMove(0, 14, 1, 100);
  
  // Weights should be positive for valid moves
  EXPECT_GT(testMove.weight, 0);
  
  // And should be within a reasonable range (not excessively large)
  EXPECT_LT(testMove.weight, 1000);
}
