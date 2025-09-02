#include <gtest/gtest.h>
#include <iostream>
#include "heuristic_sorting/internal.h"

/**
 * Old vs New Implementation Comparison Test
 * This test captures weight outputs from the new implementation
 * Run twice: once with --define=use_new_heuristic=false, once with =true
 */
class OldVsNewComparisonTest : public ::testing::Test {
protected:
    pos createStandardTestPosition() {
        pos tpos = {};
        
        // Use the same setup that works in working_comparison_test
        for (int h = 0; h < 4; h++) {
            for (int s = 0; s < 4; s++) {
                tpos.length[h][s] = 3; // Each hand has 3 cards in each suit
                tpos.rankInSuit[h][s] = 0x7000; // Some high cards
            }
        }
        return tpos;
    }
};

TEST_F(OldVsNewComparisonTest, CaptureWeightOutputs) {
    std::cout << "\n=== WEIGHT OUTPUT CAPTURE TEST ===" << std::endl;
    
    pos tpos = createStandardTestPosition();
    relRanksType thrp_rel[4] = {};
    moveType bestMove = {0, 14, 1, 0};
    moveType bestMoveTT = {0, 13, 1, 0};
    trackType track = {};
    
    // Test a variety of moves
    moveType testMoves[8];
    testMoves[0] = {0, 14, 0, 0}; // Ace of spades
    testMoves[1] = {0, 13, 0, 0}; // King of spades
    testMoves[2] = {0, 12, 1, 0}; // Queen of hearts
    testMoves[3] = {0, 11, 1, 0}; // Jack of hearts
    testMoves[4] = {0, 10, 2, 0}; // Ten of diamonds
    testMoves[5] = {0, 9, 2, 0};  // Nine of diamonds
    testMoves[6] = {0, 8, 3, 0};  // Eight of clubs
    testMoves[7] = {0, 7, 3, 0};  // Seven of clubs
    
    // Reset weights
    for (int i = 0; i < 8; i++) {
        testMoves[i].weight = 0;
    }
    
    // Test Trump WeightAlloc
    std::cout << "\nTRUMP WEIGHTS (use_new_heuristic=" 
#ifdef DDS_USE_NEW_HEURISTIC
              << "true" 
#else
              << "false"
#endif
              << "):" << std::endl;
    
    HeuristicContext trumpContext = {
        tpos, bestMove, bestMoveTT, thrp_rel,
        testMoves, 8, 0, 1, 0, &track, 0, 0, 0, 0  // trump=1 (spades)
    };
    
    WeightAllocTrump0(trumpContext);
    
    for (int i = 0; i < 8; i++) {
        std::cout << "Move " << i << " (♠♥♦♣"[testMoves[i].suit] 
                  << testMoves[i].rank << "): " << testMoves[i].weight << std::endl;
    }
    
    // Reset and test NT WeightAlloc
    for (int i = 0; i < 8; i++) {
        testMoves[i].weight = 0;
    }
    
    std::cout << "\nNO-TRUMP WEIGHTS (use_new_heuristic=" 
#ifdef DDS_USE_NEW_HEURISTIC
              << "true" 
#else
              << "false"
#endif
              << "):" << std::endl;
    
    HeuristicContext ntContext = {
        tpos, bestMove, bestMoveTT, thrp_rel,
        testMoves, 8, 0, 4, 0, &track, 0, 0, 0, 0  // trump=4 (no-trump)
    };
    
    WeightAllocNT0(ntContext);
    
    for (int i = 0; i < 8; i++) {
        std::cout << "Move " << i << " (♠♥♦♣"[testMoves[i].suit] 
                  << testMoves[i].rank << "): " << testMoves[i].weight << std::endl;
    }
    
    std::cout << "\nTo compare implementations:" << std::endl;
    std::cout << "1. Run with --define=use_new_heuristic=false" << std::endl;
    std::cout << "2. Run with --define=use_new_heuristic=true" << std::endl;
    std::cout << "3. Compare the weight outputs above" << std::endl;
}

TEST_F(OldVsNewComparisonTest, TestSpecificScenarios) {
    std::cout << "\n=== SPECIFIC SCENARIO WEIGHT TESTING ===" << std::endl;
    
    // Test different position characteristics
    for (int scenario = 0; scenario < 3; scenario++) {
        std::cout << "\n--- Scenario " << scenario << " ---" << std::endl;
        
        pos tpos = createStandardTestPosition();
        
        // Modify position based on scenario
        if (scenario == 1) {
            // Scenario 1: Unbalanced distribution
            tpos.length[0][0] = 5; // North has 5 spades
            tpos.length[1][0] = 1; // East has 1 spade
        } else if (scenario == 2) {
            // Scenario 2: Void in a suit
            tpos.length[1][1] = 0; // East void in hearts
            tpos.rankInSuit[1][1] = 0;
        }
        
        relRanksType thrp_rel[4] = {};
        moveType bestMove = {0, 14, 1, 0};
        moveType bestMoveTT = {0, 13, 1, 0};
        trackType track = {};
        
        // Test a single representative move
        moveType testMove = {0, 12, 0, 0}; // Queen of spades
        testMove.weight = 0;
        
        HeuristicContext context = {
            tpos, bestMove, bestMoveTT, thrp_rel,
            &testMove, 1, 0, 1, 0, &track, 0, 0, 0, 0
        };
        
        WeightAllocTrump0(context);
        
        std::cout << "Queen of spades trump weight: " << testMove.weight << std::endl;
        
        // Test NT
        testMove.weight = 0;
        context.trump = 4;
        WeightAllocNT0(context);
        
        std::cout << "Queen of spades NT weight: " << testMove.weight << std::endl;
    }
}
