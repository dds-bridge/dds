#include <gtest/gtest.h>
#include <iostream>
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "dds/dll.h"

/**
 * Minimal test to isolate the segfault issue
 */
class MinimalWeightTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize the DDS system
        SetMaxThreads(0);
    }
};

TEST_F(MinimalWeightTest, BasicWeightAllocCall) {
    std::cout << "Creating basic position..." << std::endl;
    
    // Create a basic position structure
    pos tpos;
    memset(&tpos, 0, sizeof(pos));
    
    // Set minimal required data
    tpos.length[0][0] = 4;  // North has 4 spades
    tpos.length[1][0] = 3;  // East has 3 spades
    tpos.length[2][0] = 3;  // South has 3 spades
    tpos.length[3][0] = 3;  // West has 3 spades
    
    // Create some test moves
    moveType moves[3] = {
        {0, 14, 0, 0},  // Ace of spades
        {0, 13, 0, 0},  // King of spades
        {0, 12, 0, 0}   // Queen of spades
    };
    
    // Create other required structures
    moveType bestMove = {0, 14, 1, 0};
    moveType bestMoveTT = {0, 13, 1, 0};
    relRanksType thrp_rel[4] = {};
    
    std::cout << "Creating HeuristicContext..." << std::endl;
    
    // Create HeuristicContext using constructor syntax
    HeuristicContext context = {
        tpos,          // const pos& tpos
        bestMove,      // const moveType& bestMove  
        bestMoveTT,    // const moveType& bestMoveTT
        thrp_rel,      // const relRanksType* thrp_rel
        moves,         // moveType* mply
        3,             // numMoves
        0,             // lastNumMoves
        0,             // trump (spades)
        0,             // suit
        nullptr,       // trackp
        0,             // currTrick
        0,             // currHand
        0,             // leadHand
        0              // leadSuit
    };
    
    std::cout << "Calling WeightAllocTrump0..." << std::endl;
    
    try {
        WeightAllocTrump0(context);
        std::cout << "SUCCESS: WeightAllocTrump0 completed without crash!" << std::endl;
        
        // Print results
        for (int i = 0; i < 3; i++) {
            std::cout << "Move " << i << ": suit=" << moves[i].suit 
                      << " rank=" << moves[i].rank 
                      << " weight=" << moves[i].weight << std::endl;
        }
        
        SUCCEED();
    } catch (...) {
        FAIL() << "WeightAllocTrump0 threw an exception";
    }
}
