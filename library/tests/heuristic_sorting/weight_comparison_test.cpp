#include <gtest/gtest.h>
#include <iostream>
#include "heuristic_sorting/heuristic_sorting.h"
#include "dds/Moves.h"

// This test compares weights produced by the old and new heuristic implementations
// It must be compiled twice - once with each implementation - to capture outputs

class WeightComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize common test data
    }
    
    // Create a representative test position
    pos createTestPosition(bool trumpGame, int leadHand) {
        pos tpos = {};
        
        // Set up a realistic bridge position
        // North (0): ♠AKQ ♥J32 ♦A54 ♣876
        tpos.rankInSuit[0][0] = 0b11100000000000;  // Spades: A,K,Q
        tpos.rankInSuit[0][1] = 0b00000001100000;  // Hearts: J,3,2  
        tpos.rankInSuit[0][2] = 0b01000000010100;  // Diamonds: A,5,4
        tpos.rankInSuit[0][3] = 0b00000001110000;  // Clubs: 8,7,6
        
        // Set suit lengths
        tpos.length[0][0] = 3; tpos.length[0][1] = 3; 
        tpos.length[0][2] = 3; tpos.length[0][3] = 3;
        
        // East (1): ♠J987 ♥AKQ ♦32 ♣AKQ
        tpos.rankInSuit[1][0] = 0b00000010111000;  // Spades: J,9,8,7
        tpos.rankInSuit[1][1] = 0b11100000000000;  // Hearts: A,K,Q
        tpos.rankInSuit[1][2] = 0b00000001100000;  // Diamonds: 3,2
        tpos.rankInSuit[1][3] = 0b11100000000000;  // Clubs: A,K,Q
        
        tpos.length[1][0] = 4; tpos.length[1][1] = 3;
        tpos.length[1][2] = 2; tpos.length[1][3] = 3;
        
        // South (2): ♠654 ♥987 ♦KQJ ♣432
        tpos.rankInSuit[2][0] = 0b00000000010100;  // Spades: 6,5,4
        tpos.rankInSuit[2][1] = 0b00000010111000;  // Hearts: 9,8,7
        tpos.rankInSuit[2][2] = 0b01110000000000;  // Diamonds: K,Q,J
        tpos.rankInSuit[2][3] = 0b00000001110000;  // Clubs: 4,3,2
        
        tpos.length[2][0] = 3; tpos.length[2][1] = 3;
        tpos.length[2][2] = 3; tpos.length[2][3] = 3;
        
        // West (3): ♠T32 ♥T654 ♦T987 ♣JT9
        tpos.rankInSuit[3][0] = 0b00000100001100;  // Spades: T,3,2
        tpos.rankInSuit[3][1] = 0b00000100111100;  // Hearts: T,6,5,4
        tpos.rankInSuit[3][2] = 0b00000100111000;  // Diamonds: T,9,8,7
        tpos.rankInSuit[3][3] = 0b00000010100000;  // Clubs: J,T,9
        
        tpos.length[3][0] = 3; tpos.length[3][1] = 4;
        tpos.length[3][2] = 4; tpos.length[3][3] = 3;
        
        // Set trump winner based on test scenario
        if (trumpGame) {
            tpos.winner[1].rank = 14; // Ace of Hearts available
            tpos.winner[1].hand = 1;
        } else {
            tpos.winner[1].rank = 0;  // No trump winner
        }
        
        return tpos;
    }
    
    // Create test moves for a given suit
    void createTestMoves(moveType* moves, int numMoves, int suit) {
        for (int i = 0; i < numMoves; i++) {
            moves[i].suit = suit;
            moves[i].rank = 14 - i;  // Start from Ace and go down
            moves[i].sequence = 1 << (14 - i);
            moves[i].weight = 0;  // Will be set by heuristic
        }
    }
    
    void createTrackData(trackType& track, int leadHand, int leadSuit) {
        track.leadHand = leadHand;
        track.leadSuit = leadSuit;
        
        // Initialize other track data
        for (int i = 0; i < 4; i++) {
            track.playSuits[i] = -1;
            track.playRanks[i] = -1;
            track.high[i] = -1;
        }
        
        for (int s = 0; s < 4; s++) {
            track.removedRanks[s] = 0;
        }
    }
};

#ifdef DDS_USE_NEW_HEURISTIC
TEST_F(WeightComparisonTest, CaptureNewImplementationWeights) {
    std::cout << "\n=== NEW HEURISTIC IMPLEMENTATION WEIGHTS ===" << std::endl;
#else  
TEST_F(WeightComparisonTest, CaptureOriginalImplementationWeights) {
    std::cout << "\n=== ORIGINAL IMPLEMENTATION WEIGHTS ===" << std::endl;
#endif

    const int numMoves = 5;
    moveType moves[numMoves];
    
    // Test scenario 1: Leading hand, trump game, spades
    {
        std::cout << "\nScenario 1: Leading hand, trump game, spades" << std::endl;
        pos tpos = createTestPosition(true, 0);
        createTestMoves(moves, numMoves, 0);  // Spades
        
        trackType track;
        createTrackData(track, 0, 0);
        
        relRanksType thrp_rel = {};
        moveType bestMove = {0, 14, 1, 0};  // Ace of spades
        moveType bestMoveTT = {0, 13, 1, 0}; // King of spades
        
#ifdef DDS_USE_NEW_HEURISTIC
        CallHeuristic(tpos, bestMove, bestMoveTT, &thrp_rel,
                     moves, numMoves, 0, 1, 0, &track,
                     0, 0, 0, 0);
#else
        // For original implementation, we'd need to set up Moves class
        // This is more complex and will be implemented in next iteration
        std::cout << "Original implementation testing requires Moves class setup" << std::endl;
        return;
#endif
        
        std::cout << "Weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
    }
    
    // Test scenario 2: Following hand position 1, no-trump, following suit
    {
        std::cout << "\nScenario 2: Following hand pos 1, no-trump, following suit" << std::endl;
        pos tpos = createTestPosition(false, 0);
        createTestMoves(moves, numMoves, 1);  // Hearts
        
        trackType track;
        createTrackData(track, 0, 1);  // Hearts led
        
        relRanksType thrp_rel = {};
        moveType bestMove = {};
        moveType bestMoveTT = {};
        
#ifdef DDS_USE_NEW_HEURISTIC
        CallHeuristic(tpos, bestMove, bestMoveTT, &thrp_rel,
                     moves, numMoves, 0, 4, 1, &track,
                     0, 1, 0, 1);  // currHand=1, leadSuit=1
#else
        std::cout << "Original implementation testing requires Moves class setup" << std::endl;
        return;
#endif
        
        std::cout << "Weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
    }
    
    // Test scenario 3: Following hand position 2, trump game, void
    {
        std::cout << "\nScenario 3: Following hand pos 2, trump game, void" << std::endl;
        pos tpos = createTestPosition(true, 0);
        
        // Make position 2 void in spades
        tpos.rankInSuit[2][0] = 0;
        tpos.length[2][0] = 0;
        
        createTestMoves(moves, numMoves, 1);  // Playing hearts (trump)
        
        trackType track;
        createTrackData(track, 0, 0);  // Spades led, but pos 2 void
        
        relRanksType thrp_rel = {};
        moveType bestMove = {};
        moveType bestMoveTT = {};
        
#ifdef DDS_USE_NEW_HEURISTIC
        CallHeuristic(tpos, bestMove, bestMoveTT, &thrp_rel,
                     moves, numMoves, 0, 1, 1, &track,
                     0, 2, 0, 0);  // currHand=2, leadSuit=0 (spades)
#else
        std::cout << "Original implementation testing requires Moves class setup" << std::endl;
        return;
#endif
        
        std::cout << "Weights: ";
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n=============================================" << std::endl;
}

// Additional test to capture systematic differences
#ifdef DDS_USE_NEW_HEURISTIC
TEST_F(WeightComparisonTest, SystematicComparisonNew) {
    std::cout << "\n=== SYSTEMATIC COMPARISON (NEW) ===" << std::endl;
#else
TEST_F(WeightComparisonTest, SystematicComparisonOriginal) {
    std::cout << "\n=== SYSTEMATIC COMPARISON (ORIGINAL) ===" << std::endl;
#endif

    // Test all major function paths with consistent inputs
    const int numScenarios = 4;
    const char* scenarioNames[] = {
        "MoveGen0_Trump",
        "MoveGen0_NoTrump", 
        "MoveGen123_Following",
        "MoveGen123_Void"
    };
    
    for (int scenario = 0; scenario < numScenarios; scenario++) {
        std::cout << "\n" << scenarioNames[scenario] << ": ";
        
        const int numMoves = 3;
        moveType moves[numMoves];
        createTestMoves(moves, numMoves, 0);
        
        pos tpos = createTestPosition(scenario < 2, 0);  // Trump for first 2 scenarios
        trackType track;
        createTrackData(track, 0, 0);
        
        relRanksType thrp_rel = {};
        moveType bestMove = {0, 14, 1, 0};
        moveType bestMoveTT = {0, 13, 1, 0};
        
        int currHand = (scenario < 2) ? 0 : 1;  // Leading hand for first 2
        int trump = (scenario < 2) ? 1 : ((scenario == 2) ? 4 : 1);  // No-trump for scenario 2
        
        if (scenario == 3) {  // Void scenario
            tpos.rankInSuit[1][0] = 0;  // Make hand 1 void in spades
            tpos.length[1][0] = 0;
        }
        
#ifdef DDS_USE_NEW_HEURISTIC
        CallHeuristic(tpos, bestMove, bestMoveTT, &thrp_rel,
                     moves, numMoves, 0, trump, 0, &track,
                     0, currHand, 0, 0);
        
        for (int i = 0; i < numMoves; i++) {
            std::cout << moves[i].weight << " ";
        }
#else
        std::cout << "ORIG_WEIGHTS_HERE ";
#endif
    }
    std::cout << std::endl;
}
