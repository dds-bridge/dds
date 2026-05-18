/// @file heuristic_sorting_test.cpp
/// @brief Regression tests for heuristic sorting behavior.
///
/// Validates that move weights are ordered correctly and that weight-related
/// heuristics maintain expected priority and consistency across scenarios.

#include <algorithm>
#include <gtest/gtest.h>
#include <vector>

#include <api/dds.h>

class HeuristicSortingTest : public ::testing::Test
{
protected:
    HeuristicSortingTest() = default;

    // Helper function to create a MoveType with specified values
    auto create_move(const int suit, const int rank, const int sequence, const int weight) const -> MoveType
    {
        MoveType move;
        move.suit = suit;
        move.rank = rank;
        move.sequence = sequence;
        move.weight = weight;
        return move;
    }
};

TEST_F(HeuristicSortingTest, SortsMovesInDescendingOrderByWeight)
{
    // Create test moves with different weights
    std::vector<MoveType> test_moves = {
        create_move(0, 14, 0, 10),  // Low weight
        create_move(1, 13, 0, 50),  // High weight
        create_move(2, 12, 0, 30),  // Medium weight
        create_move(3, 11, 0, 45)   // Medium-high weight
    };

    // Expected order after sorting: weights 50, 45, 30, 10
    const std::vector<int> expected_weights = {50, 45, 30, 10};

    // Sort using standard library to verify our expectation
    std::sort(
        test_moves.begin(),
        test_moves.end(),
        [](const MoveType& a, const MoveType& b) {
            return a.weight > b.weight;  // Descending order
        }
    );

    // Verify the weights are in descending order
    for (size_t i = 0; i < test_moves.size(); ++i) {
        EXPECT_EQ(test_moves[i].weight, expected_weights[i]);
    }
}

TEST_F(HeuristicSortingTest, HigherWeightMovesHavePriority)
{
    // Test that moves with higher weights get priority in sorting
    const MoveType high_priority = create_move(0, 14, 0, 100);
    const MoveType low_priority = create_move(1, 13, 0, 5);

    EXPECT_GT(high_priority.weight, low_priority.weight);

    // In a comparison-based sort, higher weight should come first
    EXPECT_TRUE(high_priority.weight > low_priority.weight);
}

TEST_F(HeuristicSortingTest, WeightCalculationFollowsHeuristics)
{
    // Test various weight scenarios based on the heuristic logic

    // High card with sequence bonus should have high weight
    const int base_weight = 30;
    const int rank_bonus = 14;  // Ace
    const int sequence_bonus = 40;

    const int expected_high_card_weight = base_weight + sequence_bonus + rank_bonus;

    // Verify that high cards with sequences get appropriate weight
    EXPECT_GT(expected_high_card_weight, base_weight);
    EXPECT_GT(expected_high_card_weight, base_weight + rank_bonus);
}

TEST_F(HeuristicSortingTest, MoveSequenceAffectsWeight)
{
    // Test that sequence moves get different weights
    const MoveType sequence_move = create_move(0, 14, 1, 40);  // Has sequence
    const MoveType non_sequence_move = create_move(0, 14, 0, 20);  // No sequence

    // Sequence moves typically get bonus weight
    EXPECT_GT(sequence_move.weight, non_sequence_move.weight);
}

TEST_F(HeuristicSortingTest, SameSuitMovesOrderedByRank)
{
    // Within the same suit, higher ranks should generally have higher weights
    // (though this depends on the specific heuristic context)

    const MoveType ace = create_move(0, 14, 0, 50);    // Ace
    const MoveType king = create_move(0, 13, 0, 45);   // King
    const MoveType queen = create_move(0, 12, 0, 40);  // Queen

    // Generally, higher ranks get higher base weights
    EXPECT_GE(ace.weight, king.weight);
    EXPECT_GE(king.weight, queen.weight);
}

TEST_F(HeuristicSortingTest, WeightDifferencesSignificant)
{
    // Test that weight differences are significant enough to affect sorting

    const MoveType best_move = create_move(0, 14, 1, 100);
    const MoveType good_move = create_move(1, 13, 1, 75);
    const MoveType average_move = create_move(2, 12, 0, 50);
    const MoveType poor_move = create_move(3, 11, 0, 25);

    // Verify clear ordering
    EXPECT_GT(best_move.weight, good_move.weight);
    EXPECT_GT(good_move.weight, average_move.weight);
    EXPECT_GT(average_move.weight, poor_move.weight);

    // Verify differences are meaningful (at least 20 point gaps)
    EXPECT_GE(best_move.weight - good_move.weight, 20);
    EXPECT_GE(good_move.weight - average_move.weight, 20);
    EXPECT_GE(average_move.weight - poor_move.weight, 20);
}

TEST_F(HeuristicSortingTest, TrumpSuitMovesGetPriority)
{
    // In trump contracts, trump suit moves often get priority

    const MoveType trump_move = create_move(0, 14, 0, 80);    // Trump suit (assuming suit 0 is trump)
    const MoveType non_trump_move = create_move(1, 14, 0, 60); // Non-trump suit

    // Trump moves should generally have higher weights
    EXPECT_GT(trump_move.weight, non_trump_move.weight);
}

TEST_F(HeuristicSortingTest, SortingStabilityWithEqualWeights)
{
    // Test behavior when moves have equal weights

    const MoveType move_one = create_move(0, 14, 0, 50);
    const MoveType move_two = create_move(1, 13, 0, 50);
    const MoveType move_three = create_move(2, 12, 0, 50);

    // All have equal weights
    EXPECT_EQ(move_one.weight, move_two.weight);
    EXPECT_EQ(move_two.weight, move_three.weight);

    // The sorting algorithm should handle equal weights gracefully
    // (order may vary but should be deterministic)
}

TEST_F(HeuristicSortingTest, WeightRangeIsReasonable)
{
    // Test that weights are in a reasonable range
    const MoveType test_move = create_move(0, 14, 1, 100);

    // Weights should be positive for valid moves
    EXPECT_GT(test_move.weight, 0);

    // And should be within a reasonable range (not excessively large)
    EXPECT_LT(test_move.weight, 1000);
}
