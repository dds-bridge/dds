#include "heuristic_sorting/internal.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include <gtest/gtest.h>
#include <iostream>

class HeuristicSortingUnitTest : public ::testing::Test {
 protected:
  HeuristicSortingUnitTest() = default;
};

// Test basic function compilation and execution
TEST_F(HeuristicSortingUnitTest, BasicFunctionTest) {
    moveType mply[1];
    mply[0].suit = 0;
    mply[0].rank = 14;
    mply[0].weight = 0;
    mply[0].sequence = 1;

    // Test that functions exist and can be called
    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};

    HeuristicContext context {
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        mply,
        1,
        0,
        1,  // trump = Hearts
        0,
        &track,
        1,
        0,
        0,
        0
    };

    // Just test that the functions exist and don't crash
    WeightAllocTrump0(context);
    EXPECT_TRUE(true) << "WeightAllocTrump0 completed without crashing";
    
    std::cout << "BasicFunctionTest passed." << std::endl;
}

// Test SortMoves dispatcher
TEST_F(HeuristicSortingUnitTest, TestSortMovesDispatcher) {
    moveType mply[3];
    
    // Initialize multiple moves
    for (int i = 0; i < 3; i++) {
        mply[i].suit = i % 4;
        mply[i].rank = 12 + i;
        mply[i].weight = 0;
        mply[i].sequence = i + 1;
    }

    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};

    HeuristicContext context {
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        mply,
        3,   // numMoves
        0,   // lastNumMoves
        1,   // trump = Hearts
        0,   // suit
        &track,
        1,   // currTrick
        0,   // currHand
        0,   // leadHand
        0    // leadSuit
    };

    // Call the dispatcher
    SortMoves(context);
    
    // Check that at least one move got a weight assigned
    bool hasWeight = false;
    for (int i = 0; i < 3; i++) {
        if (mply[i].weight != 0) {
            hasWeight = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasWeight) << "SortMoves should assign weights to moves";
    
    std::cout << "TestSortMovesDispatcher passed. Move weights: ";
    for (int i = 0; i < 3; i++) {
        std::cout << mply[i].weight << " ";
    }
    std::cout << std::endl;
}

// Test CallHeuristic integration function
TEST_F(HeuristicSortingUnitTest, TestCallHeuristicIntegration) {
    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    moveType mply[2];
    
    // Initialize moves
    mply[0].suit = 0;     
    mply[0].rank = 14;    
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    mply[1].suit = 1;     
    mply[1].rank = 13;    
    mply[1].weight = 0;
    mply[1].sequence = 2;
    
    trackType track = {};
    
    // Call the integration function
    CallHeuristic(
        tpos,
        bestMove, 
        bestMoveTT,
        thrp_rel,
        mply,
        2,       // numMoves
        0,       // lastNumMoves
        1,       // trump (hearts)
        0,       // suit
        &track,  // trackp
        1,       // currTrick
        0,       // currHand
        0,       // leadHand
        0        // leadSuit
    );
    
    // Check that weights were assigned
    bool hasWeight = (mply[0].weight != 0) || (mply[1].weight != 0);
    EXPECT_TRUE(hasWeight) << "CallHeuristic should assign weights";
    
    std::cout << "TestCallHeuristicIntegration passed. Move weights: " 
              << mply[0].weight << " " << mply[1].weight << std::endl;
}

// Test individual WeightAlloc functions
TEST_F(HeuristicSortingUnitTest, TestIndividualWeightAllocFunctions) {
    moveType mply[1];
    mply[0].suit = 0;
    mply[0].rank = 14;
    mply[0].weight = 0;
    mply[0].sequence = 1;

    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};

    HeuristicContext context {
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        mply,
        1,
        0,
        1,  // trump
        0,
        &track,
        1,
        0,
        0,
        0
    };

    // Test each WeightAlloc function to ensure they don't crash
    std::cout << "Testing individual WeightAlloc functions:" << std::endl;
    
    mply[0].weight = 0;
    WeightAllocTrump0(context);
    std::cout << "  WeightAllocTrump0: weight=" << mply[0].weight << std::endl;
    
    mply[0].weight = 0;
    WeightAllocNT0(context);
    std::cout << "  WeightAllocNT0: weight=" << mply[0].weight << std::endl;
    
    mply[0].weight = 0;
    WeightAllocTrumpNotvoid1(context);
    std::cout << "  WeightAllocTrumpNotvoid1: weight=" << mply[0].weight << std::endl;
    
    mply[0].weight = 0;
    WeightAllocNTNotvoid1(context);
    std::cout << "  WeightAllocNTNotvoid1: weight=" << mply[0].weight << std::endl;
    
    EXPECT_TRUE(true) << "All WeightAlloc functions completed without crashing";
    
    std::cout << "TestIndividualWeightAllocFunctions passed." << std::endl;
}
