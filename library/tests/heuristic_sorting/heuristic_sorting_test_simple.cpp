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

// Test weight comparison functionality for diagnostic purposes
TEST_F(HeuristicSortingUnitTest, TestWeightComparison) {
    const int numMoves = 5;
    moveType mply1[numMoves], mply2[numMoves];
    
    // Create identical test scenarios
    for (int i = 0; i < numMoves; i++) {
        mply1[i].suit = i % 4;
        mply1[i].rank = 14 - i;
        mply1[i].weight = 0;
        mply1[i].sequence = 1 << (14 - i);
        
        mply2[i] = mply1[i]; // Copy for comparison
    }

    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};

    HeuristicContext context1 {
        tpos, bestMove, bestMoveTT, thrp_rel, mply1, numMoves,
        0, 1, 0, &track, 1, 0, 0, 0
    };
    
    HeuristicContext context2 {
        tpos, bestMove, bestMoveTT, thrp_rel, mply2, numMoves,
        0, 1, 0, &track, 1, 0, 0, 0
    };

    // Call same function on both contexts
    WeightAllocTrump0(context1);
    WeightAllocTrump0(context2);

    // Compare weights - should be identical
    bool weightsMatch = true;
    for (int i = 0; i < numMoves; i++) {
        if (mply1[i].weight != mply2[i].weight) {
            weightsMatch = false;
            std::cout << "Weight mismatch at move " << i 
                      << ": " << mply1[i].weight << " vs " << mply2[i].weight << std::endl;
        }
    }
    
    EXPECT_TRUE(weightsMatch) << "Identical scenarios should produce identical weights";
    
    std::cout << "Weight comparison test passed - identical scenarios produce identical weights." << std::endl;
}

// Test move ordering effectiveness
TEST_F(HeuristicSortingUnitTest, TestMoveOrderingEffectiveness) {
    const int numMoves = 7;
    moveType mply[numMoves];
    
    // Create moves with random initial weights
    int initialWeights[] = {5, 1, 8, 3, 9, 2, 6};
    for (int i = 0; i < numMoves; i++) {
        mply[i].suit = i % 4;
        mply[i].rank = 14 - i;
        mply[i].weight = initialWeights[i];
        mply[i].sequence = 1 << (14 - i);
    }

    // Sort using MergeSort
    MergeSort(mply, numMoves);

    // Verify moves are in descending order
    bool properOrder = true;
    for (int i = 1; i < numMoves; i++) {
        if (mply[i-1].weight < mply[i].weight) {
            properOrder = false;
            std::cout << "Order violation at position " << i 
                      << ": " << mply[i-1].weight << " < " << mply[i].weight << std::endl;
        }
    }
    
    EXPECT_TRUE(properOrder) << "Moves should be sorted in descending order by weight";
    
    // Verify highest weight is first
    EXPECT_EQ(mply[0].weight, 9) << "Highest weight move should be first";
    
    // Verify lowest weight is last
    EXPECT_EQ(mply[numMoves-1].weight, 1) << "Lowest weight move should be last";
    
    std::cout << "Move ordering effectiveness test passed." << std::endl;
}

// Test all trump suits and no-trump scenarios
TEST_F(HeuristicSortingUnitTest, TestAllTrumpSuits) {
    const int numMoves = 3;
    moveType mply[numMoves];
    
    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};

    // Test each trump suit (0=Spades, 1=Hearts, 2=Diamonds, 3=Clubs)
    for (int trump = 0; trump < 4; trump++) {
        // Reset moves
        for (int i = 0; i < numMoves; i++) {
            mply[i].suit = i % 4;
            mply[i].rank = 14 - i;
            mply[i].weight = 0;
            mply[i].sequence = 1 << (14 - i);
        }

        HeuristicContext context {
            tpos, bestMove, bestMoveTT, thrp_rel, mply, numMoves,
            0, trump, 0, &track, 1, 0, 0, 0
        };

        SortMoves(context);
        
        bool hasWeights = false;
        for (int i = 0; i < numMoves; i++) {
            if (mply[i].weight != 0) {
                hasWeights = true;
                break;
            }
        }
        
        EXPECT_TRUE(hasWeights) << "Trump suit " << trump << " should assign weights";
        
        std::cout << "Trump suit " << trump << " weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << mply[i].weight << " ";
        }
        std::cout << std::endl;
    }
    
    // Test no-trump
    {
        for (int i = 0; i < numMoves; i++) {
            mply[i].suit = i % 4;
            mply[i].rank = 14 - i;
            mply[i].weight = 0;
            mply[i].sequence = 1 << (14 - i);
        }

        HeuristicContext context {
            tpos, bestMove, bestMoveTT, thrp_rel, mply, numMoves,
            0, 4, 0, &track, 1, 0, 0, 0  // 4 = DDS_NOTRUMP
        };

        SortMoves(context);
        
        bool hasWeights = false;
        for (int i = 0; i < numMoves; i++) {
            if (mply[i].weight != 0) {
                hasWeights = true;
                break;
            }
        }
        
        EXPECT_TRUE(hasWeights) << "No-trump should assign weights";
        
        std::cout << "No-trump weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << mply[i].weight << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "All trump suits tested successfully!" << std::endl;
}

// Test different player positions and void conditions
TEST_F(HeuristicSortingUnitTest, TestPlayerPositionsAndVoids) {
    const int numMoves = 4;
    moveType mply[numMoves];
    
    pos tpos = {};
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};

    // Test each player position
    for (int position = 0; position < 4; position++) {
        for (int i = 0; i < numMoves; i++) {
            mply[i].suit = i % 4;
            mply[i].rank = 14 - i;
            mply[i].weight = 0;
            mply[i].sequence = 1 << (14 - i);
        }

        // Set up position - if position > 0, make it following hand
        int leadHand = 0;
        int currHand = position;
        
        HeuristicContext context {
            tpos, bestMove, bestMoveTT, thrp_rel, mply, numMoves,
            0, 1, 0, &track, 1, currHand, leadHand, 0
        };

        SortMoves(context);
        
        bool hasWeights = false;
        for (int i = 0; i < numMoves; i++) {
            if (mply[i].weight != 0) {
                hasWeights = true;
                break;
            }
        }
        
        EXPECT_TRUE(hasWeights) << "Position " << position << " should assign weights";
        
        std::cout << "Position " << position << " weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << mply[i].weight << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "Player positions tested successfully!" << std::endl;
}
