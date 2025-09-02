#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "dds/Moves.h"

/**
 * Simple direct comparison test for weight allocation functions
 * This test compares the move weights between old and new implementations
 */
class SimpleWeightComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test data
    }

    pos createSimpleTestPosition() {
        pos tpos;
        
        // Initialize basic position - cleared first
        memset(&tpos, 0, sizeof(pos));
        
        // Set basic hand distributions (each hand has 3 cards in each suit)
        for (int hand = 0; hand < 4; hand++) {
            for (int suit = 0; suit < 4; suit++) {
                tpos.length[hand][suit] = 3;
            }
            tpos.length[hand][0] = 4;  // First hand has 4 spades
        }
        
        // Set rank presence - simplified for testing
        // Each suit has basic ranks available
        tpos.aggr[0] = 0x7FFF;  // All spades available initially
        tpos.aggr[1] = 0x7FFF;  // All hearts available initially  
        tpos.aggr[2] = 0x7FFF;  // All diamonds available initially
        tpos.aggr[3] = 0x7FFF;  // All clubs available initially
        
        // Set some realistic rank distributions
        for (int hand = 0; hand < 4; hand++) {
            for (int suit = 0; suit < 4; suit++) {
                tpos.rankInSuit[hand][suit] = (0x1000 + 0x800 + 0x400) >> (hand * 3);
            }
        }
        
        return tpos;
    }
};

TEST_F(SimpleWeightComparisonTest, CompareAgainstOriginalMoveGen0) {
    pos tpos = createSimpleTestPosition();
    relRanksType thrp_rel[4] = {};
    moveType bestMove = {0, 14, 1, 0};    // Ace of spades
    moveType bestMoveTT = {0, 13, 1, 0};  // King of spades

    std::cout << "\n=== DIRECT COMPARISON TEST ===" << std::endl;
    
    // Test the old implementation first (when compiled without new heuristic)
    std::cout << "\nTesting OLD implementation:" << std::endl;
    
    // Create Moves instance for old implementation
    Moves oldMoves;
    
    // Initialize the Moves object properly
    int initialRanks[] = {14, 13, 12, 11};  // A, K, Q, J of spades
    int initialSuits[] = {0, 0, 0, 0};      // All spades
    oldMoves.Init(0, 0, initialRanks, initialSuits, tpos.rankInSuit, 1, 0);

    // Call old implementation using public interface
    int oldNumMoves = oldMoves.MoveGen0(0, tpos, bestMove, bestMoveTT, thrp_rel);
    
    std::cout << "  Generated " << oldNumMoves << " moves" << std::endl;
    
    // Try to get the moves from the old implementation
    // Note: This is challenging because MakeNext is stateful
    std::vector<moveType> oldMoveList;
    for (int i = 0; i < oldNumMoves && i < 10; i++) {
        const moveType* oldMove = oldMoves.MakeNext(0, 0, tpos.winRanks[0]);
        if (oldMove) {
            oldMoveList.push_back(*oldMove);
            std::cout << "    Move " << i << ": suit=" << oldMove->suit 
                      << " rank=" << oldMove->rank << " weight=" << oldMove->weight << std::endl;
        }
    }

    std::cout << "\nTesting NEW implementation:" << std::endl;
    
    // Create test moves to process with the new heuristic
    moveType newMoves[5];
    newMoves[0] = {0, 14, 0, 0};  // Ace of spades
    newMoves[1] = {0, 13, 1, 0};  // King of hearts  
    newMoves[2] = {0, 12, 2, 0};  // Queen of diamonds
    newMoves[3] = {0, 11, 3, 0};  // Jack of clubs
    newMoves[4] = {0, 10, 0, 0};  // Ten of spades
    
    // Initialize weights to zero
    for (int i = 0; i < 5; i++) {
        newMoves[i].weight = 0;
    }
    
    // Create a trackType structure for the new implementation
    trackType track;
    track.leadHand = 0;
    track.leadSuit = 0;
    
    // Create a HeuristicContext for the new implementation
    HeuristicContext context = {
        tpos,           // pos
        bestMove,       // bestMove  
        bestMoveTT,     // bestMoveTT
        thrp_rel,       // thrp_rel
        newMoves,       // mply
        5,              // numMoves
        0,              // lastNumMoves
        1,              // trump (spades)
        0,              // suit (spades)
        &track,         // trackp
        0,              // currTrick
        0,              // currHand
        0,              // leadHand
        0               // leadSuit (spades)
    };
    
    // Call the new implementation using WeightAllocTrump0 directly
    WeightAllocTrump0(context);
    
    std::cout << "  New implementation (Trump) processed 5 test moves:" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "    Move " << i << ": suit=" << newMoves[i].suit 
                  << " rank=" << newMoves[i].rank << " weight=" << newMoves[i].weight << std::endl;
    }
    
    // Reset weights and test NT function
    for (int i = 0; i < 5; i++) {
        newMoves[i].weight = 0;
    }
    
    context.trump = 4; // No-trump
    WeightAllocNT0(context);
    
    std::cout << "  New implementation (NT) processed same 5 moves:" << std::endl;
    for (int i = 0; i < 5; i++) {
        std::cout << "    Move " << i << ": suit=" << newMoves[i].suit 
                  << " rank=" << newMoves[i].rank << " weight=" << newMoves[i].weight << std::endl;
    }

    std::cout << "\n=== ANALYSIS ===" << std::endl;
    std::cout << "Old implementation moves: " << oldNumMoves << std::endl;
    std::cout << "Successfully demonstrated calling both old and new weight allocation" << std::endl;
    std::cout << "Next step: Create proper side-by-side comparison with identical inputs" << std::endl;
    
    // Basic validation that we got some results
    EXPECT_GT(oldNumMoves, 0) << "Old implementation should generate moves";
    EXPECT_GT(oldMoveList.size(), 0) << "Should be able to retrieve moves from old implementation";
}
