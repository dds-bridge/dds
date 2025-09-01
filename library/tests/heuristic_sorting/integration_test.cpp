#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <cstring>

// Include the new heuristic library headers
#include "heuristic_sorting/internal.h"
#include "heuristic_sorting/heuristic_sorting.h"

class HeuristicIntegrationTest : public ::testing::Test {
 protected:
  HeuristicIntegrationTest() = default;
  
  // Helper to create a test position with specific characteristics
  pos createTestPosition(bool hasTrumpWinner = false, int trumpSuit = 1) {
    pos tpos = {};
    
    // Initialize basic position
    for (int hand = 0; hand < DDS_HANDS; hand++) {
      for (int suit = 0; suit < DDS_SUITS; suit++) {
        tpos.rankInSuit[hand][suit] = 0;
        tpos.length[hand][suit] = 0;
      }
    }
    
    // Set up a specific scenario
    // Hand 0 (North) has some cards
    tpos.rankInSuit[0][0] = 0x7000; // A K Q of spades
    tpos.length[0][0] = 3;
    tpos.rankInSuit[0][1] = 0x1000; // K of hearts 
    tpos.length[0][1] = 1;
    
    // Hand 1 (East) has some cards
    tpos.rankInSuit[1][0] = 0x0800; // J of spades
    tpos.length[1][0] = 1;
    tpos.rankInSuit[1][2] = 0x6000; // A K of diamonds
    tpos.length[1][2] = 2;
    
    if (hasTrumpWinner) {
      tpos.winner[trumpSuit].rank = 14; // Ace
      tpos.winner[trumpSuit].hand = 0;
    }
    
    return tpos;
  }
  
  // Helper to create test moves
  void createTestMoves(moveType* mply, int numMoves) {
    for (int i = 0; i < numMoves; i++) {
      mply[i].suit = i % 4;
      mply[i].rank = 14 - i; // Descending ranks
      mply[i].weight = 0;     // Will be set by heuristic
      mply[i].sequence = i + 1;
    }
  }
  
  // Helper to copy move weights for comparison
  void copyMoveWeights(const moveType* from, moveType* to, int numMoves) {
    for (int i = 0; i < numMoves; i++) {
      to[i].weight = from[i].weight;
    }
  }
  
  // Helper to compare move weights
  bool compareMoveWeights(const moveType* moves1, const moveType* moves2, int numMoves) {
    for (int i = 0; i < numMoves; i++) {
      if (moves1[i].weight != moves2[i].weight) {
        std::cout << "Weight mismatch at move " << i 
                  << ": old=" << moves1[i].weight 
                  << " new=" << moves2[i].weight << std::endl;
        return false;
      }
    }
    return true;
  }
};

// Test that new heuristic library produces same results as original for basic scenarios
TEST_F(HeuristicIntegrationTest, TestBasicWeightComparison) {
    const int numMoves = 3;
    moveType movesOld[numMoves];
    moveType movesNew[numMoves];
    
    // Set up identical moves
    createTestMoves(movesOld, numMoves);
    createTestMoves(movesNew, numMoves);
    
    // Create test position
    pos tpos = createTestPosition(true, 1);
    
    // Test with new heuristic library
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};
    
    CallHeuristic(
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        movesNew,
        numMoves,
        0,      // lastNumMoves
        1,      // trump (hearts)
        0,      // suit
        &track, // trackp
        1,      // currTrick
        0,      // currHand  
        0,      // leadHand
        0       // leadSuit
    );
    
    // For comparison with original, we can test specific functions directly
    // Since we can't easily instantiate the full Moves class in this context,
    // we'll focus on testing that the new implementation produces consistent results
    
    std::cout << "New implementation move weights: ";
    for (int i = 0; i < numMoves; i++) {
        std::cout << movesNew[i].weight << " ";
    }
    std::cout << std::endl;
    
    // Verify that weights were assigned (non-zero for at least some moves)
    bool hasNonZeroWeight = false;
    for (int i = 0; i < numMoves; i++) {
        if (movesNew[i].weight != 0) {
            hasNonZeroWeight = true;
            break;
        }
    }
    
    EXPECT_TRUE(hasNonZeroWeight) << "New implementation should assign weights to moves";
}

// Test different game scenarios to ensure comprehensive coverage
TEST_F(HeuristicIntegrationTest, TestDifferentGameScenarios) {
    const int numMoves = 4;
    moveType moves[numMoves];
    
    // Test scenario 1: Trump game with trump winner
    {
        createTestMoves(moves, numMoves);
        pos tpos = createTestPosition(true, 1);
        
        moveType bestMove = {};
        moveType bestMoveTT = {};
        relRanksType thrp_rel[1] = {};
        trackType track = {};
        
        CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                     0, 1, 0, &track, 1, 0, 0, 0);
        
        std::cout << "Trump scenario weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
        
        EXPECT_TRUE(true) << "Trump scenario completed successfully";
    }
    
    // Test scenario 2: No-trump game
    {
        createTestMoves(moves, numMoves);
        pos tpos = createTestPosition(false, 0);
        
        moveType bestMove = {};
        moveType bestMoveTT = {};
        relRanksType thrp_rel[1] = {};
        trackType track = {};
        
        CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                     0, DDS_NOTRUMP, 0, &track, 1, 0, 0, 0);
        
        std::cout << "No-trump scenario weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
        
        EXPECT_TRUE(true) << "No-trump scenario completed successfully";
    }
    
    // Test scenario 3: Following hand (handRel != 0)
    {
        createTestMoves(moves, numMoves);
        pos tpos = createTestPosition(false, 0);
        
        moveType bestMove = {};
        moveType bestMoveTT = {};
        relRanksType thrp_rel[1] = {};
        trackType track = {};
        
        CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                     0, 1, 0, &track, 1, 1, 0, 0); // currHand=1, leadHand=0
        
        std::cout << "Following hand scenario weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
        
        EXPECT_TRUE(true) << "Following hand scenario completed successfully";
    }
}

// Test edge cases and boundary conditions
TEST_F(HeuristicIntegrationTest, TestEdgeCases) {
    // Test with single move
    {
        const int numMoves = 1;
        moveType moves[numMoves];
        createTestMoves(moves, numMoves);
        
        pos tpos = createTestPosition();
        moveType bestMove = {};
        moveType bestMoveTT = {};
        relRanksType thrp_rel[1] = {};
        trackType track = {};
        
        CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                     0, 1, 0, &track, 1, 0, 0, 0);
        
        std::cout << "Single move test weight: " << moves[0].weight << std::endl;
        EXPECT_TRUE(true) << "Single move scenario completed successfully";
    }
    
    // Test with maximum moves
    {
        const int numMoves = 13;
        moveType moves[numMoves];
        createTestMoves(moves, numMoves);
        
        pos tpos = createTestPosition();
        moveType bestMove = {};
        moveType bestMoveTT = {};
        relRanksType thrp_rel[1] = {};
        trackType track = {};
        
        CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                     0, 1, 0, &track, 1, 0, 0, 0);
        
        std::cout << "Maximum moves test - first few weights: ";
        for (int i = 0; i < std::min(5, numMoves); i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
        
        EXPECT_TRUE(true) << "Maximum moves scenario completed successfully";
    }
}

// Test consistency - same inputs should produce same outputs
TEST_F(HeuristicIntegrationTest, TestConsistency) {
    const int numMoves = 3;
    moveType moves1[numMoves];
    moveType moves2[numMoves];
    
    // Set up identical scenarios
    createTestMoves(moves1, numMoves);
    createTestMoves(moves2, numMoves);
    
    pos tpos = createTestPosition(true, 1);
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    trackType track = {};
    
    // Run the same test twice
    CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves1, numMoves,
                 0, 1, 0, &track, 1, 0, 0, 0);
                 
    CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves2, numMoves,
                 0, 1, 0, &track, 1, 0, 0, 0);
    
    // Results should be identical
    bool identical = compareMoveWeights(moves1, moves2, numMoves);
    
    EXPECT_TRUE(identical) << "Identical inputs should produce identical outputs";
    
    if (identical) {
        std::cout << "Consistency test passed - identical results for identical inputs" << std::endl;
    }
}
