#include <gtest/gtest.h>
#include <iostream>
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"

TEST(MinimalNewTest, TestWeightAllocTrump0) {
    std::cout << "Testing minimal WeightAllocTrump0 call..." << std::endl;
    
    // Create minimal pos structure
    pos tpos = {};
    // Note: trump is passed separately in the context
    
    // Initialize some basic data
    for (int h = 0; h < 4; h++) {
        for (int s = 0; s < 4; s++) {
            tpos.length[h][s] = 3; // Each hand has 3 cards in each suit
            tpos.rankInSuit[h][s] = 0x7000; // Some high cards
        }
    }
    
    // Create moves
    moveType moves[3];
    moves[0] = {0, 14, 0, 0}; // Ace of spades
    moves[1] = {0, 13, 0, 0}; // King of spades  
    moves[2] = {0, 12, 0, 0}; // Queen of spades
    
    for (int i = 0; i < 3; i++) {
        moves[i].weight = 0;
    }
    
    moveType bestMove = {0, 14, 1, 0};
    moveType bestMoveTT = {0, 13, 1, 0};
    relRanksType thrp_rel = {};
    
    trackType track = {};
    track.leadHand = 0;
    track.leadSuit = 0;
    
    HeuristicContext context = {
        tpos,           // pos
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
    
    std::cout << "About to call WeightAllocTrump0..." << std::endl;
    
    // This is where the segfault likely occurs
    WeightAllocTrump0(context);
    
    std::cout << "WeightAllocTrump0 completed successfully!" << std::endl;
    
    for (int i = 0; i < 3; i++) {
        std::cout << "Move " << i << " weight: " << moves[i].weight << std::endl;
    }
    
    EXPECT_TRUE(true); // If we get here, no segfault occurred
}
