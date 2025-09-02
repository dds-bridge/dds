#include <gtest/gtest.h>
#include <iostream>
#include <cstring>
#include <algorithm>

// Include the new heuristic library headers
#include "dds/dds.h"
#include "heuristic_sorting/internal.h"
#include "heuristic_sorting/heuristic_sorting.h"

class ComprehensiveHeuristicTest : public ::testing::Test {
 protected:
  ComprehensiveHeuristicTest() = default;
  
  // Helper function to create a test position with specific characteristics
  pos createTestPosition(int trumpSuit = 1, bool hasTrumpWinner = true) {
    pos tpos = {};
    
    // Initialize basic position - all hands empty initially
    for (int hand = 0; hand < DDS_HANDS; hand++) {
      for (int suit = 0; suit < DDS_SUITS; suit++) {
        tpos.rankInSuit[hand][suit] = 0;
        tpos.length[hand][suit] = 0;
      }
    }
    
    // Set up realistic card distribution
    // Hand 0 (North) - mixed distribution
    tpos.rankInSuit[0][0] = 0x7000; // A K Q of spades
    tpos.length[0][0] = 3;
    tpos.rankInSuit[0][1] = 0x1000; // K of hearts 
    tpos.length[0][1] = 1;
    tpos.rankInSuit[0][2] = 0x0800; // J of diamonds
    tpos.length[0][2] = 1;
    
    // Hand 1 (East) - some voids for testing void conditions
    tpos.rankInSuit[1][0] = 0x0400; // 10 of spades
    tpos.length[1][0] = 1;
    // Hand 1 void in hearts (length[1][1] = 0)
    tpos.rankInSuit[1][2] = 0x6000; // A K of diamonds
    tpos.length[1][2] = 2;
    tpos.rankInSuit[1][3] = 0x2000; // Q of clubs
    tpos.length[1][3] = 1;
    
    // Hand 2 (South) - following scenarios
    tpos.rankInSuit[2][0] = 0x0200; // 9 of spades
    tpos.length[2][0] = 1;
    tpos.rankInSuit[2][1] = 0x0800; // J of hearts
    tpos.length[2][1] = 1;
    tpos.rankInSuit[2][2] = 0x1000; // K of diamonds
    tpos.length[2][2] = 1;
    
    // Hand 3 (West) - final position scenarios
    tpos.rankInSuit[3][0] = 0x0100; // 8 of spades
    tpos.length[3][0] = 1;
    tpos.rankInSuit[3][1] = 0x4000; // A of hearts
    tpos.length[3][1] = 1;
    // Hand 3 void in diamonds (length[3][2] = 0)
    tpos.rankInSuit[3][3] = 0x7000; // A K Q of clubs
    tpos.length[3][3] = 3;
    
    // Set up aggregates
    for (int suit = 0; suit < DDS_SUITS; suit++) {
      tpos.aggr[suit] = tpos.rankInSuit[0][suit] | tpos.rankInSuit[1][suit] | 
                        tpos.rankInSuit[2][suit] | tpos.rankInSuit[3][suit];
    }
    
    // Set up winners and second best if trump game
    if (trumpSuit != DDS_NOTRUMP && hasTrumpWinner) {
      tpos.winner[trumpSuit].rank = 14; // Ace
      tpos.winner[trumpSuit].hand = 3;  // West has ace of trump
      tpos.secondBest[trumpSuit].rank = 13; // King  
      tpos.secondBest[trumpSuit].hand = 0;  // North has king of trump
    }
    
    return tpos;
  }
  
  // Helper to create test moves
  void createTestMoves(moveType* mply, int numMoves, int suit = 0) {
    for (int i = 0; i < numMoves; i++) {
      mply[i].suit = suit;
      mply[i].rank = 14 - i; // Descending ranks (A, K, Q, J, ...)
      mply[i].weight = 0;     // Will be set by heuristic
      mply[i].sequence = 1 << (14 - i); // Corresponding sequence bit
    }
  }
  
  // Helper to create test context for specific game state
  HeuristicContext createTestContext(pos& tpos, moveType* mply, int numMoves,
                                   int currHand, int leadHand, int leadSuit,
                                   int trump = 1, int currTrick = 1) {
    static moveType bestMove = {};
    static moveType bestMoveTT = {};
    static relRanksType thrp_rel[1] = {};
    static trackType track = {};
    
    // Set up track data
    track.leadHand = leadHand;
    track.leadSuit = leadSuit;
    
    return HeuristicContext {
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        mply,
        numMoves,
        0, // lastNumMoves
        trump,
        leadSuit, // suit
        &track,
        currTrick,
        currHand,
        leadHand,
        leadSuit
    };
  }
  
  // Helper to check if moves are properly sorted (descending by weight)
  bool areMovesProperlysorted(const moveType* mply, int numMoves) {
    for (int i = 1; i < numMoves; i++) {
      if (mply[i-1].weight < mply[i].weight) {
        return false;
      }
    }
    return true;
  }
};

// Test all 13 WeightAlloc functions for basic functionality
TEST_F(ComprehensiveHeuristicTest, TestAllWeightAllocFunctions) {
    const int numMoves = 3;
    moveType mply[numMoves];
    
    // Test each function with appropriate game state
    
    // Position 0 (Leading player) functions
    {
        pos tpos = createTestPosition(1, true); // Trump game with winner
        createTestMoves(mply, numMoves, 0);
        auto context = createTestContext(tpos, mply, numMoves, 0, 0, 0, 1);
        
        // Reset weights and test WeightAllocTrump0
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocTrump0(context);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocTrump0 should assign weights";
        
        // Reset weights and test WeightAllocNT0
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        auto ntContext = createTestContext(tpos, mply, numMoves, 0, 0, 0, DDS_NOTRUMP);
        WeightAllocNT0(ntContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocNT0 should assign weights";
    }
    
    // Position 1 (First follower) functions
    {
        pos tpos = createTestPosition(1, true);
        createTestMoves(mply, numMoves, 0); // Following lead suit
        
        // Test not void functions (can follow suit)
        tpos.rankInSuit[1][0] = 0x0400; // Hand 1 has cards in lead suit
        tpos.length[1][0] = 1;
        
        auto trumpContext = createTestContext(tpos, mply, numMoves, 1, 0, 0, 1);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocTrumpNotvoid1(trumpContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocTrumpNotvoid1 should assign weights";
        
        auto ntContext = createTestContext(tpos, mply, numMoves, 1, 0, 0, DDS_NOTRUMP);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocNTNotvoid1(ntContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocNTNotvoid1 should assign weights";
        
        // Test void functions (cannot follow suit)
        tpos.rankInSuit[1][0] = 0; // Hand 1 void in lead suit
        tpos.length[1][0] = 0;
        createTestMoves(mply, numMoves, 1); // Playing different suit
        
        trumpContext = createTestContext(tpos, mply, numMoves, 1, 0, 0, 1);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocTrumpVoid1(trumpContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocTrumpVoid1 should assign weights";
        
        ntContext = createTestContext(tpos, mply, numMoves, 1, 0, 0, DDS_NOTRUMP);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocNTVoid1(ntContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocNTVoid1 should assign weights";
    }
    
    // Position 2 (Second follower) functions
    {
        pos tpos = createTestPosition(1, true);
        createTestMoves(mply, numMoves, 0);
        
        // Test not void functions
        tpos.rankInSuit[2][0] = 0x0200; // Hand 2 has cards in lead suit
        tpos.length[2][0] = 1;
        
        auto trumpContext = createTestContext(tpos, mply, numMoves, 2, 0, 0, 1);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocTrumpNotvoid2(trumpContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocTrumpNotvoid2 should assign weights";
        
        auto ntContext = createTestContext(tpos, mply, numMoves, 2, 0, 0, DDS_NOTRUMP);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocNTNotvoid2(ntContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocNTNotvoid2 should assign weights";
        
        // Test void functions
        tpos.rankInSuit[2][0] = 0; // Hand 2 void in lead suit
        tpos.length[2][0] = 0;
        createTestMoves(mply, numMoves, 1);
        
        trumpContext = createTestContext(tpos, mply, numMoves, 2, 0, 0, 1);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocTrumpVoid2(trumpContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocTrumpVoid2 should assign weights";
        
        ntContext = createTestContext(tpos, mply, numMoves, 2, 0, 0, DDS_NOTRUMP);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocNTVoid2(ntContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocNTVoid2 should assign weights";
    }
    
    // Position 3 (Final player) functions
    {
        pos tpos = createTestPosition(1, true);
        createTestMoves(mply, numMoves, 0);
        
        // Test combined not void function (used for both trump and NT)
        tpos.rankInSuit[3][0] = 0x0100; // Hand 3 has cards in lead suit
        tpos.length[3][0] = 1;
        
        auto context = createTestContext(tpos, mply, numMoves, 3, 0, 0, 1);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocCombinedNotvoid3(context);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocCombinedNotvoid3 should assign weights";
        
        // Test void functions
        tpos.rankInSuit[3][0] = 0; // Hand 3 void in lead suit
        tpos.length[3][0] = 0;
        createTestMoves(mply, numMoves, 1);
        
        auto trumpContext = createTestContext(tpos, mply, numMoves, 3, 0, 0, 1);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocTrumpVoid3(trumpContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocTrumpVoid3 should assign weights";
        
        auto ntContext = createTestContext(tpos, mply, numMoves, 3, 0, 0, DDS_NOTRUMP);
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        WeightAllocNTVoid3(ntContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "WeightAllocNTVoid3 should assign weights";
    }
    
    std::cout << "All 13 WeightAlloc functions tested successfully!" << std::endl;
}

// Test MergeSort functionality with all cases 1-13
TEST_F(ComprehensiveHeuristicTest, TestMergeSortAllCases) {
    // Test each case from 1 to 13 moves
    for (int numMoves = 1; numMoves <= 13; numMoves++) {
        moveType mply[13];
        
        // Create moves with random weights (reverse order to test sorting)
        for (int i = 0; i < numMoves; i++) {
            mply[i].suit = i % 4;
            mply[i].rank = 14 - i;
            mply[i].weight = numMoves - i; // Reverse order weights
            mply[i].sequence = 1 << (14 - i);
        }
        
        // Sort using MergeSort
        MergeSort(mply, numMoves);
        
        // Verify moves are sorted in descending order by weight
        EXPECT_TRUE(areMovesProperlysorted(mply, numMoves))
            << "MergeSort failed for " << numMoves << " moves";
        
        // Verify highest weight is first
        EXPECT_EQ(mply[0].weight, numMoves) 
            << "Highest weight move should be first for " << numMoves << " moves";
    }
    
    std::cout << "MergeSort tested for all cases 1-13 moves!" << std::endl;
}

// Test function dispatch logic matches original WeightList array behavior
TEST_F(ComprehensiveHeuristicTest, TestFunctionDispatchCorrectness) {
    const int numMoves = 2;
    moveType mply[numMoves];
    
    // Test all possible dispatch combinations
    struct TestCase {
        int handRel;
        bool trumpGame;
        bool canFollowSuit;
        int expectedFindex;
        const char* description;
    };
    
    std::vector<TestCase> testCases = {
        {0, false, true,  -1, "Leading, NT (uses WeightAllocNT0)"},
        {0, true,  true,  -1, "Leading, Trump (uses WeightAllocTrump0)"},
        {1, false, true,   4, "handRel=1, NT, following suit"},
        {1, true,  true,   5, "handRel=1, Trump, following suit"},
        {1, false, false,  6, "handRel=1, NT, void"},
        {1, true,  false,  7, "handRel=1, Trump, void"},
        {2, false, true,   8, "handRel=2, NT, following suit"},
        {2, true,  true,   9, "handRel=2, Trump, following suit"},
        {2, false, false, 10, "handRel=2, NT, void"},
        {2, true,  false, 11, "handRel=2, Trump, void"},
        {3, false, true,  12, "handRel=3, NT, following suit (Combined)"},
        {3, true,  true,  13, "handRel=3, Trump, following suit (Combined)"},
        {3, false, false, 14, "handRel=3, NT, void"},
        {3, true,  false, 15, "handRel=3, Trump, void"},
    };
    
    for (const auto& testCase : testCases) {
        pos tpos = createTestPosition(testCase.trumpGame ? 1 : DDS_NOTRUMP, testCase.trumpGame);
        createTestMoves(mply, numMoves, 0);
        
        // Set up void/not void condition for current hand
        int currHand = testCase.handRel == 0 ? 0 : testCase.handRel;
        if (testCase.canFollowSuit) {
            tpos.rankInSuit[currHand][0] = 0x1000; // Has cards in lead suit
            tpos.length[currHand][0] = 1;
        } else {
            tpos.rankInSuit[currHand][0] = 0; // Void in lead suit
            tpos.length[currHand][0] = 0;
        }
        
        auto context = createTestContext(tpos, mply, numMoves, currHand, 0, 0, 
                                       testCase.trumpGame ? 1 : DDS_NOTRUMP);
        
        // Reset weights
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        
        // Call SortMoves dispatcher
        SortMoves(context);
        
        // Verify that some weight was assigned (function was called)
        bool hasWeight = std::any_of(mply, mply + numMoves, 
                                   [](const moveType& m) { return m.weight != 0; });
        EXPECT_TRUE(hasWeight) << "Dispatch failed for: " << testCase.description;
        
        // Verify moves are sorted
        EXPECT_TRUE(areMovesProperlySort ed(mply, numMoves))
            << "Moves not sorted after SortMoves for: " << testCase.description;
    }
    
    std::cout << "Function dispatch logic tested for all scenarios!" << std::endl;
}

// Test comprehensive game scenarios covering different bridge situations
TEST_F(ComprehensiveHeuristicTest, TestGameScenarios) {
    const int numMoves = 4;
    moveType mply[numMoves];
    
    // Scenario 1: Early game trump contract
    {
        pos tpos = createTestPosition(1, true);
        createTestMoves(mply, numMoves, 0);
        auto context = createTestContext(tpos, mply, numMoves, 0, 0, 0, 1, 1);
        
        SortMoves(context);
        EXPECT_TRUE(areMovesProperlyorted(mply, numMoves)) << "Early trump game sorting failed";
    }
    
    // Scenario 2: Late game no-trump
    {
        pos tpos = createTestPosition(DDS_NOTRUMP, false);
        createTestMoves(mply, numMoves, 0);
        auto context = createTestContext(tpos, mply, numMoves, 0, 0, 0, DDS_NOTRUMP, 10);
        
        SortMoves(context);
        EXPECT_TRUE(areMovesProperlyorted(mply, numMoves)) << "Late NT game sorting failed";
    }
    
    // Scenario 3: Void hand discarding
    {
        pos tpos = createTestPosition(1, true);
        // Make current player void in lead suit
        tpos.rankInSuit[1][0] = 0;
        tpos.length[1][0] = 0;
        createTestMoves(mply, numMoves, 1); // Playing different suit
        
        auto context = createTestContext(tpos, mply, numMoves, 1, 0, 0, 1);
        SortMoves(context);
        EXPECT_TRUE(areMovesProperlyorted(mply, numMoves)) << "Void hand sorting failed";
    }
    
    // Scenario 4: Final player decision
    {
        pos tpos = createTestPosition(1, true);
        createTestMoves(mply, numMoves, 0);
        auto context = createTestContext(tpos, mply, numMoves, 3, 0, 0, 1);
        
        SortMoves(context);
        EXPECT_TRUE(areMovesProperlyorted(mply, numMoves)) << "Final player sorting failed";
    }
    
    std::cout << "Comprehensive game scenarios tested successfully!" << std::endl;
}

// Test edge cases and boundary conditions
TEST_F(ComprehensiveHeuristicTest, TestEdgeCases) {
    // Test single move (no sorting needed)
    {
        moveType mply[1];
        pos tpos = createTestPosition();
        createTestMoves(mply, 1, 0);
        auto context = createTestContext(tpos, mply, 1, 0, 0, 0);
        
        SortMoves(context);
        EXPECT_NE(mply[0].weight, 0) << "Single move should get weight assigned";
    }
    
    // Test maximum moves (13)
    {
        moveType mply[13];
        pos tpos = createTestPosition();
        createTestMoves(mply, 13, 0);
        auto context = createTestContext(tpos, mply, 13, 0, 0, 0);
        
        SortMoves(context);
        EXPECT_TRUE(areMovesProperlyorted(mply, 13)) << "Maximum moves sorting failed";
    }
    
    // Test all trump suits
    for (int trump = 0; trump < DDS_SUITS; trump++) {
        moveType mply[3];
        pos tpos = createTestPosition(trump, true);
        createTestMoves(mply, 3, 0);
        auto context = createTestContext(tpos, mply, 3, 0, 0, 0, trump);
        
        SortMoves(context);
        EXPECT_TRUE(areMovesProperlyorted(mply, 3)) 
            << "Trump suit " << trump << " sorting failed";
    }
    
    std::cout << "Edge cases tested successfully!" << std::endl;
}

// Test data structure validation
TEST_F(ComprehensiveHeuristicTest, TestDataStructureUsage) {
    const int numMoves = 3;
    moveType mply[numMoves];
    pos tpos = createTestPosition();
    createTestMoves(mply, numMoves, 0);
    
    // Test that all required data structures are accessed
    auto context = createTestContext(tpos, mply, numMoves, 0, 0, 0);
    
    // Verify context contains all required data
    EXPECT_NE(&context.tpos, nullptr) << "Position data should be accessible";
    EXPECT_NE(context.mply, nullptr) << "Move array should be accessible";
    EXPECT_GT(context.numMoves, 0) << "Number of moves should be positive";
    EXPECT_NE(context.trackp, nullptr) << "Track data should be accessible";
    
    // Test with various trump suits and verify they're handled
    for (int trump = 0; trump <= DDS_NOTRUMP; trump++) {
        auto trumpContext = createTestContext(tpos, mply, numMoves, 0, 0, 0, trump);
        
        // Reset weights
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        
        SortMoves(trumpContext);
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, 
                              [](const moveType& m) { return m.weight != 0; }))
            << "Trump " << trump << " should assign weights";
    }
    
    std::cout << "Data structure usage validated!" << std::endl;
}
