
#include <gtest/gtest.h>
#include <cassert>
#include <iostream>
#include "heuristic_sorting/internal.h"
#include "heuristic_sorting/heuristic_sorting.h"
class HeuristicSortingUnitTest : public ::testing::Test {
 protected:
  HeuristicSortingUnitTest() = default;
  
  // Helper function to create a basic position
  pos createBasicPosition() {
    pos tpos = {};
    // Initialize with some basic data
    for (int hand = 0; hand < DDS_HANDS; hand++) {
      for (int suit = 0; suit < DDS_SUITS; suit++) {
        tpos.rankInSuit[hand][suit] = 0;
        tpos.length[hand][suit] = 0;
      }
    }
    return tpos;
  }
  
  // Helper function to create a basic context with modifiable position
  HeuristicContext createBasicContext(pos& tpos, moveType* mply, int numMoves) {
    static moveType bestMove = {};
    static moveType bestMoveTT = {};
    static relRanksType thrp_rel[1] = {};
    static trackType track = {};
    
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

// Test WeightAllocTrump0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrump0SetsWeight) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings (we can now modify the fields through const_cast)
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    
    // Call the function under test
    WeightAllocTrump0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocTrump0";
    
    std::cout << "TestWeightAllocTrump0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test CallHeuristic integration
TEST_F(HeuristicSortingUnitTest, TestCallHeuristicIntegration) {
    pos tpos = createBasicPosition();
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    moveType mply[10];
    
    // Initialize a move
    mply[0].suit = 0;     
    mply[0].rank = 14;    
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    trackType track = {};
    
    // Call the integration function
    CallHeuristic(
        tpos,
        bestMove, 
        bestMoveTT,
        thrp_rel,
        mply,
        1,       // numMoves
        0,       // lastNumMoves
        1,       // trump (hearts)
        0,       // suit
        &track,  // trackp
        1,       // currTrick
        0,       // currHand
        0,       // leadHand
        0        // leadSuit
    );
    
    // The weight should have been set
    EXPECT_NE(mply[0].weight, 0) << "CallHeuristic should set move weights";
    
    std::cout << "TestCallHeuristicIntegration passed. Weight: " << mply[0].weight << std::endl;
}

// Test WeightAllocTrump0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrump0) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings (these are not const)
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    
    // Call the function under test
    WeightAllocTrump0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocTrump0";
    
    std::cout << "TestWeightAllocTrump0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test WeightAllocNT0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocNT0) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings
    const_cast<int&>(context.trump) = DDS_NOTRUMP; // No trump
    
    // Call the function under test
    WeightAllocNT0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocNT0";
    
    std::cout << "TestWeightAllocNT0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test WeightAllocTrumpNotvoid1 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrumpNotvoid1) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades (lead suit)
    mply[0].rank = 12;    // Queen
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    const_cast<int&>(context.leadSuit) = 0; // Spades led
    const_cast<int&>(context.currHand) = 1; // Second hand to play
    
    // Call the function under test
    WeightAllocTrumpNotvoid1(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocTrumpNotvoid1";
    
    std::cout << "TestWeightAllocTrumpNotvoid1 passed. Weight: " << mply[0].weight << std::endl;
}

// Test all missing WeightAlloc functions for complete coverage
TEST_F(HeuristicSortingUnitTest, TestAllMissingWeightAllocFunctions) {
    moveType mply[5];
    pos tpos = createBasicPosition();
    
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
    
    // WeightAllocNTNotvoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    const_cast<int&>(context.currHand) = 1;
    const_cast<int&>(context.leadSuit) = 0;
    WeightAllocNTNotvoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTNotvoid1 should assign weights";
    
    // WeightAllocTrumpVoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpVoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpVoid1 should assign weights";
    
    // WeightAllocNTVoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTVoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTVoid1 should assign weights";
    
    // Test Position 2 functions
    std::cout << "Testing Position 2 functions..." << std::endl;
    const_cast<int&>(context.currHand) = 2;
    
    // WeightAllocTrumpNotvoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpNotvoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpNotvoid2 should assign weights";
    
    // WeightAllocNTNotvoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTNotvoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTNotvoid2 should assign weights";
    
    // WeightAllocTrumpVoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpVoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpVoid2 should assign weights";
    
    // WeightAllocNTVoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTVoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTVoid2 should assign weights";
    
    // Test Position 3 functions
    std::cout << "Testing Position 3 functions..." << std::endl;
    const_cast<int&>(context.currHand) = 3;
    
    // WeightAllocCombinedNotvoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocCombinedNotvoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocCombinedNotvoid3 should assign weights";
    
    // WeightAllocTrumpVoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpVoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpVoid3 should assign weights";
    
    // WeightAllocNTVoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTVoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTVoid3 should assign weights";
    
    std::cout << "All 13 WeightAlloc functions tested successfully!" << std::endl;
}
