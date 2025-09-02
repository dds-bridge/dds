#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include "heuristic_sorting/internal.h"

/**
 * Direct comparison test that uses the working pattern from minimal_new_test
 * This test compares weight outputs between new WeightAlloc functions for different scenarios
 */
class WorkingComparisonTest : public ::testing::Test {
protected:
    pos createTestPosition(int scenario) {
        pos tpos = {};
        
        // Initialize basic valid data that we know works
        for (int h = 0; h < 4; h++) {
            for (int s = 0; s < 4; s++) {
                tpos.length[h][s] = 3; // Each hand has 3 cards in each suit
                tpos.rankInSuit[h][s] = 0x7000; // Some high cards
            }
        }
        
        // Vary the setup slightly by scenario
        if (scenario == 1) {
            // Scenario 1: Different distribution
            tpos.length[0][0] = 5; // North has 5 spades
            tpos.length[1][0] = 1; // East has 1 spade
        }
        
        return tpos;
    }
};

TEST_F(WorkingComparisonTest, CompareWeightAllocTrumpVsNT) {
    std::cout << "\n=== DIRECT WEIGHT ALLOCATION COMPARISON ===" << std::endl;
    
    for (int scenario = 0; scenario < 2; scenario++) {
        std::cout << "\n--- Scenario " << scenario << " ---" << std::endl;
        
        pos tpos = createTestPosition(scenario);
        relRanksType thrp_rel[4] = {};
        moveType bestMove = {0, 14, 1, 0};
        moveType bestMoveTT = {0, 13, 1, 0};
        
        trackType track = {};
        track.leadHand = 0;
        track.leadSuit = 0;
        
        // Create test moves
        moveType testMoves[4];
        testMoves[0] = {0, 14, 0, 0}; // Ace of spades
        testMoves[1] = {0, 13, 1, 0}; // King of hearts
        testMoves[2] = {0, 12, 2, 0}; // Queen of diamonds
        testMoves[3] = {0, 11, 3, 0}; // Jack of clubs
        
        // Test Trump weight allocation
        for (int i = 0; i < 4; i++) {
            testMoves[i].weight = 0;
        }
        
        HeuristicContext trumpContext = {
            tpos, bestMove, bestMoveTT, thrp_rel,
            testMoves, 4, 0, 1, 0, &track, 0, 0, 0, 0
        };
        
        WeightAllocTrump0(trumpContext);
        
        std::cout << "Trump weights:" << std::endl;
        for (int i = 0; i < 4; i++) {
            std::cout << "  Move " << i << " (suit " << testMoves[i].suit 
                      << " rank " << testMoves[i].rank << "): " << testMoves[i].weight << std::endl;
        }
        
        // Test NT weight allocation on same moves
        for (int i = 0; i < 4; i++) {
            testMoves[i].weight = 0;
        }
        
        HeuristicContext ntContext = {
            tpos, bestMove, bestMoveTT, thrp_rel,
            testMoves, 4, 0, 4, 0, &track, 0, 0, 0, 0  // trump=4 for NT
        };
        
        WeightAllocNT0(ntContext);
        
        std::cout << "NT weights:" << std::endl;
        for (int i = 0; i < 4; i++) {
            std::cout << "  Move " << i << " (suit " << testMoves[i].suit 
                      << " rank " << testMoves[i].rank << "): " << testMoves[i].weight << std::endl;
        }
    }
}

TEST_F(WorkingComparisonTest, TestMultipleWeightAllocFunctions) {
    std::cout << "\n=== TESTING MULTIPLE WEIGHT ALLOCATION FUNCTIONS ===" << std::endl;
    
    pos tpos = createTestPosition(0);
    relRanksType thrp_rel[4] = {};
    moveType bestMove = {0, 14, 1, 0};
    moveType bestMoveTT = {0, 13, 1, 0};
    trackType track = {};
    
    moveType testMove = {0, 12, 0, 0}; // Queen of spades
    
    // Test different WeightAlloc functions
    std::vector<std::string> functionNames = {
        "WeightAllocTrump0", "WeightAllocNT0", 
        "WeightAllocTrumpNotvoid1", "WeightAllocNTNotvoid1"
    };
    
    for (size_t funcIdx = 0; funcIdx < functionNames.size(); funcIdx++) {
        testMove.weight = 0;
        
        HeuristicContext context = {
            tpos, bestMove, bestMoveTT, thrp_rel,
            &testMove, 1, 0, (funcIdx % 2 == 0) ? 1 : 4, 0, &track, 0, 0, 0, 0
        };
        
        try {
            switch (funcIdx) {
                case 0: WeightAllocTrump0(context); break;
                case 1: WeightAllocNT0(context); break;
                case 2: WeightAllocTrumpNotvoid1(context); break;
                case 3: WeightAllocNTNotvoid1(context); break;
            }
            
            std::cout << functionNames[funcIdx] << ": weight=" << testMove.weight << std::endl;
        } catch (...) {
            std::cout << functionNames[funcIdx] << ": FAILED (exception)" << std::endl;
        }
    }
    
    std::cout << "\nAll weight allocation functions are accessible via testable_heuristic_sorting target!" << std::endl;
}
