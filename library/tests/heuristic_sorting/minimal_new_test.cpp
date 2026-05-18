/**
 * @file minimal_new_test.cpp
 * @brief Minimal test for weight_alloc_trump0 function
 *
 * Tests basic functionality of weight allocation for trump contracts
 * to verify the function executes successfully without errors.
 */

#include <gtest/gtest.h>
#include <iostream>
#include "heuristic_sorting/heuristic_sorting.hpp"
#include "heuristic_sorting/internal.hpp"

TEST(MinimalNewTest, TestWeightAllocTrump0) {
    std::cout << "Testing minimal weight_alloc_trump0 call..." << std::endl;
    
    // Create minimal Pos structure
    Pos tpos = {};
    // Note: trump is passed separately in the context
    
    // Initialize some basic data
    for (int h = 0; h < 4; h++) {
        for (int s = 0; s < 4; s++) {
            tpos.length[h][s] = 3; // Each hand has 3 cards in each suit
            tpos.rank_in_suit[h][s] = 0x7000; // Some high cards
        }
    }
    
    // Create moves
    MoveType moves[3];
    moves[0] = {0, 14, 0, 0}; // Ace of spades
    moves[1] = {0, 13, 0, 0}; // King of spades  
    moves[2] = {0, 12, 0, 0}; // Queen of spades
    
    for (int i = 0; i < 3; i++) {
        moves[i].weight = 0;
    }
    
    MoveType bestMove = {0, 14, 1, 0};
    MoveType bestMoveTT = {0, 13, 1, 0};
    RelRanksType thrp_rel = {};
    
    TrackType track = {};
    track.lead_hand = 0;
    track.lead_suit = 0;
    
    HeuristicContext context = {
        tpos,           // Pos
        bestMove,       // bestMove  
        bestMoveTT,     // bestMoveTT
        &thrp_rel,      // thrp_rel
        moves,          // mply
        3,              // numMoves
        0,              // lastNumMoves
        1,              // trump (spades)
        0,              // suit (spades)
        &track,         // trackp
        0,              // currTrick
        0,              // currHand
        0,              // leadHand
        0               // leadSuit (spades)
    };
    
    std::cout << "About to call weight_alloc_trump0..." << std::endl;
    
    // This is where the segfault likely occurs
    weight_alloc_trump0(context);
    
    std::cout << "weight_alloc_trump0 completed successfully!" << std::endl;
    
    for (int i = 0; i < 3; i++) {
        std::cout << "Move " << i << " weight: " << moves[i].weight << std::endl;
    }
    
    EXPECT_TRUE(true); // If we get here, no segfault occurred
}
