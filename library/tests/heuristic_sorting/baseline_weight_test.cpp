#include <gtest/gtest.h>
#include <iostream>
#include "dds/dds.h"
#include "dds/Moves.h"

/**
 * Baseline weight comparison test - establishes what the old implementation does
 * This test captures the current weight outputs for various scenarios to establish baseline
 */
class BaselineWeightTest : public ::testing::Test {
protected:
    pos createRealisticTestPosition(int scenario) {
        pos tpos;
        memset(&tpos, 0, sizeof(pos));
        
        // Create more realistic hand distributions
        switch (scenario) {
            case 0: // Trump contract, opening lead
                // North hand: 4 spades, 3 hearts, 3 diamonds, 3 clubs
                tpos.length[0][0] = 4; tpos.length[0][1] = 3; tpos.length[0][2] = 3; tpos.length[0][3] = 3;
                // East hand: 3 spades, 4 hearts, 3 diamonds, 3 clubs  
                tpos.length[1][0] = 3; tpos.length[1][1] = 4; tpos.length[1][2] = 3; tpos.length[1][3] = 3;
                // South hand: 5 spades, 2 hearts, 3 diamonds, 3 clubs
                tpos.length[2][0] = 5; tpos.length[2][1] = 2; tpos.length[2][2] = 3; tpos.length[2][3] = 3;
                // West hand: 1 spade, 4 hearts, 4 diamonds, 4 clubs
                tpos.length[3][0] = 1; tpos.length[3][1] = 4; tpos.length[3][2] = 4; tpos.length[3][3] = 4;
                break;
                
            case 1: // No-trump contract, opening lead  
                // More balanced hands for NT
                for (int hand = 0; hand < 4; hand++) {
                    for (int suit = 0; suit < 4; suit++) {
                        tpos.length[hand][suit] = 3 + (hand % 2); // 3 or 4 cards per suit
                    }
                    tpos.length[hand][hand] = 4; // Each hand has one 4-card suit
                }
                break;
        }
        
        // Set available ranks (all cards initially available)
        tpos.aggr[0] = 0x7FFF;  // All spades
        tpos.aggr[1] = 0x7FFF;  // All hearts  
        tpos.aggr[2] = 0x7FFF;  // All diamonds
        tpos.aggr[3] = 0x7FFF;  // All clubs
        
        // Set rank distributions more realistically
        for (int hand = 0; hand < 4; hand++) {
            for (int suit = 0; suit < 4; suit++) {
                // Give each hand some high cards in their long suits
                int length = tpos.length[hand][suit];
                if (length >= 4) {
                    // Long suit gets some high cards
                    tpos.rankInSuit[hand][suit] = 0x7800; // A,K,Q,J
                } else if (length == 3) {
                    // Medium suit gets medium cards
                    tpos.rankInSuit[hand][suit] = 0x1C00; // Q,J,10
                } else {
                    // Short suit gets low cards
                    tpos.rankInSuit[hand][suit] = 0x0380; // 9,8,7
                }
            }
        }
        
        return tpos;
    }
};

TEST_F(BaselineWeightTest, CaptureBaselineWeights_TrumpScenarios) {
    std::cout << "\n=== BASELINE WEIGHT CAPTURE - TRUMP SCENARIOS ===" << std::endl;
    
    for (int scenario = 0; scenario < 2; scenario++) {
        std::cout << "\n--- Scenario " << scenario << " ---" << std::endl;
        
        pos tpos = createRealisticTestPosition(scenario);
        relRanksType thrp_rel[4] = {};
        moveType bestMove = {0, 14, 0, 0};    // Ace of spades
        moveType bestMoveTT = {0, 13, 0, 0};  // King of spades

        Moves moves;
        
        // Initialize properly for trump contract (spades = trump)
        int initialRanks[] = {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2};
        int initialSuits[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        moves.Init(0, 0, initialRanks, initialSuits, tpos.rankInSuit, 0, 0); // trump=0 (spades)

        // Generate moves with trump heuristic
        int numMoves = moves.MoveGen0(0, tpos, bestMove, bestMoveTT, thrp_rel);
        
        std::cout << "Generated " << numMoves << " moves:" << std::endl;
        
        // Collect all moves and their weights
        std::vector<moveType> moveList;
        for (int i = 0; i < numMoves && i < 20; i++) { // Limit to 20 moves max
            const moveType* move = moves.MakeNext(0, 0, tpos.winRanks[0]);
            if (move) {
                moveList.push_back(*move);
                std::cout << "  Move " << i << ": ";
                std::cout << "suit=" << move->suit << " rank=" << move->rank;
                std::cout << " sequence=" << move->sequence << " weight=" << move->weight;
                std::cout << std::endl;
            } else {
                break;
            }
        }
        
        // Summary statistics
        if (!moveList.empty()) {
            int minWeight = moveList[0].weight;
            int maxWeight = moveList[0].weight;
            double avgWeight = 0;
            
            for (const auto& move : moveList) {
                minWeight = std::min(minWeight, move.weight);
                maxWeight = std::max(maxWeight, move.weight);
                avgWeight += move.weight;
            }
            avgWeight /= moveList.size();
            
            std::cout << "  Summary: " << moveList.size() << " moves, ";
            std::cout << "weights [" << minWeight << " to " << maxWeight << "], ";
            std::cout << "avg=" << avgWeight << std::endl;
        }
    }
}

TEST_F(BaselineWeightTest, CaptureBaselineWeights_NoTrumpScenarios) {
    std::cout << "\n=== BASELINE WEIGHT CAPTURE - NO TRUMP SCENARIOS ===" << std::endl;
    
    for (int scenario = 0; scenario < 2; scenario++) {
        std::cout << "\n--- NT Scenario " << scenario << " ---" << std::endl;
        
        pos tpos = createRealisticTestPosition(scenario);
        relRanksType thrp_rel[4] = {};
        moveType bestMove = {0, 14, 0, 0};    // Ace of spades
        moveType bestMoveTT = {0, 13, 0, 0};  // King of spades

        Moves moves;
        
        // Initialize for no-trump contract
        int initialRanks[] = {14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2};
        int initialSuits[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        moves.Init(0, 0, initialRanks, initialSuits, tpos.rankInSuit, 4, 0); // trump=4 (no-trump)

        // Generate moves with NT heuristic
        int numMoves = moves.MoveGen0(0, tpos, bestMove, bestMoveTT, thrp_rel);
        
        std::cout << "Generated " << numMoves << " moves:" << std::endl;
        
        // Collect all moves and their weights
        std::vector<moveType> moveList;
        for (int i = 0; i < numMoves && i < 20; i++) {
            const moveType* move = moves.MakeNext(0, 0, tpos.winRanks[0]);
            if (move) {
                moveList.push_back(*move);
                std::cout << "  Move " << i << ": ";
                std::cout << "suit=" << move->suit << " rank=" << move->rank;
                std::cout << " sequence=" << move->sequence << " weight=" << move->weight;
                std::cout << std::endl;
            } else {
                break;
            }
        }
        
        // Summary statistics
        if (!moveList.empty()) {
            int minWeight = moveList[0].weight;
            int maxWeight = moveList[0].weight;
            double avgWeight = 0;
            
            for (const auto& move : moveList) {
                minWeight = std::min(minWeight, move.weight);
                maxWeight = std::max(maxWeight, move.weight);
                avgWeight += move.weight;
            }
            avgWeight /= moveList.size();
            
            std::cout << "  Summary: " << moveList.size() << " moves, ";
            std::cout << "weights [" << minWeight << " to " << maxWeight << "], ";
            std::cout << "avg=" << avgWeight << std::endl;
        }
    }
}
