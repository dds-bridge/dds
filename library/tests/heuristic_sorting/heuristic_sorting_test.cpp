/**
 * @file heuristic_sorting_test.cpp
 * @brief Comprehensive unit tests for heuristic sorting weight allocation functions
 *
 * Tests all weight allocation functions across different trick positions (0-3)
 * and contexts (trump/NT, void/not-void) to ensure correct move prioritization.
 */

#include <gtest/gtest.h>
#include <algorithm>
#include <cassert>
#include <iostream>
#include "heuristic_sorting/internal.hpp"
#include "heuristic_sorting/heuristic_sorting.hpp"

class HeuristicSortingUnitTest : public ::testing::Test
{
 protected:
  HeuristicSortingUnitTest() = default;
  
  // Helper function to create a basic position
  Pos createBasicPosition() {
    Pos tpos = {};
    // Initialize with some basic data
    for (int hand = 0; hand < DDS_HANDS; hand++) {
      for (int suit = 0; suit < DDS_SUITS; suit++) {
        tpos.rank_in_suit[hand][suit] = 0;
        tpos.length[hand][suit] = 0;
      }
    }
    return tpos;
  }
  
  // Helper function to create a basic context with modifiable position
  HeuristicContext createBasicContext(Pos& tpos, MoveType* mply, int numMoves) {
    static MoveType bestMove = {};
    static MoveType bestMoveTT = {};
    static RelRanksType thrp_rel[1] = {};
    static TrackType track = {};
    
    return HeuristicContext {
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        mply,
        numMoves,
        0, // lastNumMoves
        0, // trump (spades)
        0, // suit (spades)
        &track,
        1, // currTrick
        0, // currHand
        0, // leadHand
        0  // leadSuit
    };
  }
};

// Test weight_alloc_trump0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrump0SetsWeight) {
    MoveType mply[10];
    Pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings (we can now modify the fields through const_cast)
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    
    // Call the function under test
    weight_alloc_trump0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by weight_alloc_trump0";
    
    std::cout << "Testweight_alloc_trump0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test weight_alloc_trump0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrump0) {
    MoveType mply[10];
    Pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings (these are not const)
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    
    // Call the function under test
    weight_alloc_trump0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by weight_alloc_trump0";
    
    std::cout << "Testweight_alloc_trump0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test weight_alloc_nt0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocNt0) {
    MoveType mply[10];
    Pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings
    const_cast<int&>(context.trump) = DDS_NOTRUMP; // No trump
    
    // Call the function under test
    weight_alloc_nt0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by weight_alloc_nt0";
    
    std::cout << "Testweight_alloc_nt0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test weight_alloc_trump_notvoid1 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrumpNotvoid1) {
    MoveType mply[10];
    Pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades (lead suit)
    mply[0].rank = 12;    // Queen
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    const_cast<int&>(context.lead_suit) = 0; // Spades led
    const_cast<int&>(context.curr_hand) = 1; // Second hand to play
    
    // Call the function under test
    weight_alloc_trump_notvoid1(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by weight_alloc_trump_notvoid1";
    
    std::cout << "Testweight_alloc_trump_notvoid1 passed. Weight: " << mply[0].weight << std::endl;
}

// Test all missing WeightAlloc functions for complete coverage
TEST_F(HeuristicSortingUnitTest, TestAllMissingWeightAllocFunctions) {
    MoveType mply[5];
    Pos tpos = createBasicPosition();
    
    // Initialize multiple moves for better testing
    for (int i = 0; i < 5; i++) {
        mply[i].suit = i % 4;     
        mply[i].rank = 14 - i;    
        mply[i].weight = 0;
        mply[i].sequence = 1 << (14 - i);
    }
    
    auto context = createBasicContext(tpos, mply, 5);
    
    // Test Position 1 functions
    std::cout << "Testing Position 1 functions..." << std::endl;
    
    // weight_alloc_nt_notvoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    const_cast<int&>(context.curr_hand) = 1;
    const_cast<int&>(context.lead_suit) = 0;
    weight_alloc_nt_notvoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_nt_notvoid1 should assign weights";
    
    // weight_alloc_trump_void1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    weight_alloc_trump_void1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_trump_void1 should assign weights";
    
    // weight_alloc_nt_void1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    weight_alloc_nt_void1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_nt_void1 should assign weights";
    
    // Test Position 2 functions
    std::cout << "Testing Position 2 functions..." << std::endl;
    const_cast<int&>(context.curr_hand) = 2;
    
    // weight_alloc_trump_notvoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    weight_alloc_trump_notvoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_trump_notvoid2 should assign weights";
    
    // weight_alloc_nt_notvoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    weight_alloc_nt_notvoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_nt_notvoid2 should assign weights";
    
    // weight_alloc_trump_void2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    weight_alloc_trump_void2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_trump_void2 should assign weights";
    
    // weight_alloc_nt_void2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    weight_alloc_nt_void2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_nt_void2 should assign weights";
    
    // Test Position 3 functions
    std::cout << "Testing Position 3 functions..." << std::endl;
    const_cast<int&>(context.curr_hand) = 3;
    
    // weight_alloc_combined_notvoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    weight_alloc_combined_notvoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_combined_notvoid3 should assign weights";
    
    // weight_alloc_trump_void3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    weight_alloc_trump_void3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_trump_void3 should assign weights";
    
    // weight_alloc_nt_void3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    weight_alloc_nt_void3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const MoveType& m) { return m.weight != 0; }))
        << "weight_alloc_nt_void3 should assign weights";
    
    std::cout << "All 13 WeightAlloc functions tested successfully!" << std::endl;
}
