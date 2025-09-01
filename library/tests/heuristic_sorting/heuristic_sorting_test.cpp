
#include "heuristic_sorting/internal.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include <gtest/gtest.h>
#include <cassert>
#include <iostream>

class HeuristicSortingUnitTest : public ::testing::Test {
 protected:
  HeuristicSortingUnitTest() = default;
  
  // Helper function to create a basic position
  pos createBasicPosition() {
    pos tpos = {};
    // Initialize with some basic data
    for (int hand = 0; hand < DDS_HANDS; hand++) {
      for (int suit = 0; suit < DDS_SUITS; suit++) {
        tpos.rankInSuit[hand][suit] = 0;
        tpos.length[hand][suit] = 0;
      }
    }
    return tpos;
  }
  
  // Helper function to create a basic context with modifiable position
  HeuristicContext createBasicContext(pos& tpos, moveType* mply, int numMoves) {
    static moveType bestMove = {};
    static moveType bestMoveTT = {};
    static relRanksType thrp_rel[1] = {};
    static trackType track = {};
    
    return HeuristicContext {
        tpos,
        bestMove,
        bestMoveTT,
        thrp_rel,
        mply,
        numMoves,
        0, // lastNumMoves
        0, // trump (spades)
        0, // suit (spades)
        &track,
        1, // currTrick
        0, // currHand
        0, // leadHand
        0  // leadSuit
    };
  }
};

// Test WeightAllocTrump0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrump0SetsWeight) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings (we can now modify the fields through const_cast)
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    
    // Call the function under test
    WeightAllocTrump0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocTrump0";
    
    std::cout << "TestWeightAllocTrump0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test SortMoves dispatcher
TEST_F(HeuristicSortingUnitTest, TestSortMovesDispatcher) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     
    mply[0].rank = 14;    
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Test leading hand (handRel = 0) with trump
    const_cast<int&>(context.currHand) = context.leadHand; // Same hand = leading
    const_cast<int&>(context.trump) = 1; // Hearts
    
    // Call the dispatcher
    SortMoves(context);
    
    // The weight should have been set by the appropriate function
    EXPECT_NE(mply[0].weight, 0) << "SortMoves should dispatch to appropriate function";
    
    std::cout << "TestSortMovesDispatcher passed. Weight: " << mply[0].weight << std::endl;
}

// Test CallHeuristic integration
TEST_F(HeuristicSortingUnitTest, TestCallHeuristicIntegration) {
    pos tpos = createBasicPosition();
    moveType bestMove = {};
    moveType bestMoveTT = {};
    relRanksType thrp_rel[1] = {};
    moveType mply[10];
    
    // Initialize a move
    mply[0].suit = 0;     
    mply[0].rank = 14;    
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    trackType track = {};
    
    // Call the integration function
    CallHeuristic(
        tpos,
        bestMove, 
        bestMoveTT,
        thrp_rel,
        mply,
        1,       // numMoves
        0,       // lastNumMoves
        1,       // trump (hearts)
        0,       // suit
        &track,  // trackp
        1,       // currTrick
        0,       // currHand
        0,       // leadHand
        0        // leadSuit
    );
    
    // The weight should have been set
    EXPECT_NE(mply[0].weight, 0) << "CallHeuristic should set move weights";
    
    std::cout << "TestCallHeuristicIntegration passed. Weight: " << mply[0].weight << std::endl;
}

// Test WeightAllocTrump0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrump0) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings (these are not const)
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    
    // Call the function under test
    WeightAllocTrump0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocTrump0";
    
    std::cout << "TestWeightAllocTrump0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test WeightAllocNT0 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocNT0) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades
    mply[0].rank = 14;    // Ace
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings
    const_cast<int&>(context.trump) = DDS_NOTRUMP; // No trump
    
    // Call the function under test
    WeightAllocNT0(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocNT0";
    
    std::cout << "TestWeightAllocNT0 passed. Weight: " << mply[0].weight << std::endl;
}

// Test WeightAllocTrumpNotvoid1 function
TEST_F(HeuristicSortingUnitTest, TestWeightAllocTrumpNotvoid1) {
    moveType mply[10];
    pos tpos = createBasicPosition();
    
    // Initialize a move
    mply[0].suit = 0;     // Spades (lead suit)
    mply[0].rank = 12;    // Queen
    mply[0].weight = 0;
    mply[0].sequence = 1;
    
    auto context = createBasicContext(tpos, mply, 1);
    
    // Modify context settings
    const_cast<int&>(context.trump) = 1; // Hearts are trump
    const_cast<int&>(context.leadSuit) = 0; // Spades led
    const_cast<int&>(context.currHand) = 1; // Second hand to play
    
    // Call the function under test
    WeightAllocTrumpNotvoid1(context);
    
    // The weight should have been modified
    EXPECT_NE(mply[0].weight, 0) << "Weight should be set by WeightAllocTrumpNotvoid1";
    
    std::cout << "TestWeightAllocTrumpNotvoid1 passed. Weight: " << mply[0].weight << std::endl;
}

// Test all missing WeightAlloc functions for complete coverage
TEST_F(HeuristicSortingUnitTest, TestAllMissingWeightAllocFunctions) {
    moveType mply[5];
    pos tpos = createBasicPosition();
    
    // Initialize multiple moves for better testing
    for (int i = 0; i < 5; i++) {
        mply[i].suit = i % 4;     
        mply[i].rank = 14 - i;    
        mply[i].weight = 0;
        mply[i].sequence = 1 << (14 - i);
    }
    
    auto context = createBasicContext(tpos, mply, 5);
    
    // Test Position 1 functions
    std::cout << "Testing Position 1 functions..." << std::endl;
    
    // WeightAllocNTNotvoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    const_cast<int&>(context.currHand) = 1;
    const_cast<int&>(context.leadSuit) = 0;
    WeightAllocNTNotvoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTNotvoid1 should assign weights";
    
    // WeightAllocTrumpVoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpVoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpVoid1 should assign weights";
    
    // WeightAllocNTVoid1
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTVoid1(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTVoid1 should assign weights";
    
    // Test Position 2 functions
    std::cout << "Testing Position 2 functions..." << std::endl;
    const_cast<int&>(context.currHand) = 2;
    
    // WeightAllocTrumpNotvoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpNotvoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpNotvoid2 should assign weights";
    
    // WeightAllocNTNotvoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTNotvoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTNotvoid2 should assign weights";
    
    // WeightAllocTrumpVoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpVoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpVoid2 should assign weights";
    
    // WeightAllocNTVoid2
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTVoid2(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTVoid2 should assign weights";
    
    // Test Position 3 functions
    std::cout << "Testing Position 3 functions..." << std::endl;
    const_cast<int&>(context.currHand) = 3;
    
    // WeightAllocCombinedNotvoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocCombinedNotvoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocCombinedNotvoid3 should assign weights";
    
    // WeightAllocTrumpVoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = 1;
    WeightAllocTrumpVoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocTrumpVoid3 should assign weights";
    
    // WeightAllocNTVoid3
    for (int i = 0; i < 5; i++) mply[i].weight = 0;
    const_cast<int&>(context.trump) = DDS_NOTRUMP;
    WeightAllocNTVoid3(context);
    EXPECT_TRUE(std::any_of(mply, mply + 5, [](const moveType& m) { return m.weight != 0; }))
        << "WeightAllocNTVoid3 should assign weights";
    
    std::cout << "All 13 WeightAlloc functions tested successfully!" << std::endl;
}

// Test MergeSort functionality with different move counts
TEST_F(HeuristicSortingUnitTest, TestMergeSortComprehensive) {
    std::cout << "Testing MergeSort for all cases 1-13 moves..." << std::endl;
    
    // Test each case from 1 to 13 moves
    for (int numMoves = 1; numMoves <= 13; numMoves++) {
        moveType mply[13];
        
        // Create moves with weights in reverse order to test sorting
        for (int i = 0; i < numMoves; i++) {
            mply[i].suit = i % 4;
            mply[i].rank = 14 - i;
            mply[i].weight = numMoves - i; // Reverse order: 13, 12, 11, ..., 1
            mply[i].sequence = 1 << (14 - i);
        }
        
        // Sort using MergeSort
        MergeSort(mply, numMoves);
        
        // Verify moves are sorted in descending order by weight
        bool properlyorted = true;
        for (int i = 1; i < numMoves; i++) {
            if (mply[i-1].weight < mply[i].weight) {
                properlyorted = false;
                break;
            }
        }
        
        EXPECT_TRUE(properlyorted) << "MergeSort failed for " << numMoves << " moves";
        
        // Verify highest weight (numMoves) is first
        EXPECT_EQ(mply[0].weight, numMoves) 
            << "Highest weight move should be first for " << numMoves << " moves";
        
        // Verify lowest weight (1) is last
        EXPECT_EQ(mply[numMoves-1].weight, 1)
            << "Lowest weight move should be last for " << numMoves << " moves";
    }
    
    std::cout << "MergeSort tested successfully for all cases 1-13!" << std::endl;
}

// Test function dispatch logic to ensure correct WeightAlloc function is called
TEST_F(HeuristicSortingUnitTest, TestFunctionDispatchLogic) {
    std::cout << "Testing function dispatch logic..." << std::endl;
    
    moveType mply[3];
    pos tpos = createBasicPosition();
    
    // Initialize moves
    for (int i = 0; i < 3; i++) {
        mply[i].suit = 0;
        mply[i].rank = 14 - i;
        mply[i].weight = 0;
        mply[i].sequence = 1 << (14 - i);
    }
    
    // Test leading hand (handRel = 0) scenarios
    {
        auto context = createBasicContext(tpos, mply, 3);
        const_cast<int&>(context.currHand) = 0; // Same as leadHand = leading
        const_cast<int&>(context.leadHand) = 0;
        
        // Trump game leading
        for (int i = 0; i < 3; i++) mply[i].weight = 0;
        const_cast<int&>(context.trump) = 1;
        SortMoves(context);
        EXPECT_TRUE(std::any_of(mply, mply + 3, [](const moveType& m) { return m.weight != 0; }))
            << "Leading trump should dispatch correctly";
        
        // No-trump leading
        for (int i = 0; i < 3; i++) mply[i].weight = 0;
        const_cast<int&>(context.trump) = DDS_NOTRUMP;
        SortMoves(context);
        EXPECT_TRUE(std::any_of(mply, mply + 3, [](const moveType& m) { return m.weight != 0; }))
            << "Leading NT should dispatch correctly";
    }
    
    // Test following hand scenarios (handRel = 1, 2, 3)
    for (int handRel = 1; handRel <= 3; handRel++) {
        auto context = createBasicContext(tpos, mply, 3);
        const_cast<int&>(context.currHand) = handRel;
        const_cast<int&>(context.leadHand) = 0;
        const_cast<int&>(context.leadSuit) = 0;
        
        // Test trump game
        for (int i = 0; i < 3; i++) mply[i].weight = 0;
        const_cast<int&>(context.trump) = 1;
        SortMoves(context);
        EXPECT_TRUE(std::any_of(mply, mply + 3, [](const moveType& m) { return m.weight != 0; }))
            << "Following hand " << handRel << " trump should dispatch correctly";
        
        // Test no-trump game
        for (int i = 0; i < 3; i++) mply[i].weight = 0;
        const_cast<int&>(context.trump) = DDS_NOTRUMP;
        SortMoves(context);
        EXPECT_TRUE(std::any_of(mply, mply + 3, [](const moveType& m) { return m.weight != 0; }))
            << "Following hand " << handRel << " NT should dispatch correctly";
    }
    
    std::cout << "Function dispatch logic tested successfully!" << std::endl;
}

// Test comprehensive game scenarios
TEST_F(HeuristicSortingUnitTest, TestComprehensiveGameScenarios) {
    std::cout << "Testing comprehensive game scenarios..." << std::endl;
    
    const int numMoves = 4;
    moveType mply[numMoves];
    pos tpos = createBasicPosition();
    
    // Initialize test moves
    for (int i = 0; i < numMoves; i++) {
        mply[i].suit = i % 4;
        mply[i].rank = 14 - i;
        mply[i].weight = 0;
        mply[i].sequence = 1 << (14 - i);
    }
    
    // Scenario 1: Early game trump contract, leading hand
    {
        auto context = createBasicContext(tpos, mply, numMoves);
        const_cast<int&>(context.currTrick) = 1; // Early game
        const_cast<int&>(context.trump) = 1;     // Hearts trump
        const_cast<int&>(context.currHand) = 0;  // Leading
        const_cast<int&>(context.leadHand) = 0;
        
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        SortMoves(context);
        
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "Early trump game should assign weights";
    }
    
    // Scenario 2: Late game no-trump, following hand
    {
        auto context = createBasicContext(tpos, mply, numMoves);
        const_cast<int&>(context.currTrick) = 10;        // Late game
        const_cast<int&>(context.trump) = DDS_NOTRUMP;   // No trump
        const_cast<int&>(context.currHand) = 2;          // Third to play
        const_cast<int&>(context.leadHand) = 0;
        
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        SortMoves(context);
        
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "Late NT game should assign weights";
    }
    
    // Scenario 3: Different trump suits
    for (int trump = 0; trump < DDS_SUITS; trump++) {
        auto context = createBasicContext(tpos, mply, numMoves);
        const_cast<int&>(context.trump) = trump;
        const_cast<int&>(context.currHand) = 0;
        const_cast<int&>(context.leadHand) = 0;
        
        for (int i = 0; i < numMoves; i++) mply[i].weight = 0;
        SortMoves(context);
        
        EXPECT_TRUE(std::any_of(mply, mply + numMoves, [](const moveType& m) { return m.weight != 0; }))
            << "Trump suit " << trump << " should assign weights";
    }
    
    std::cout << "Comprehensive game scenarios tested successfully!" << std::endl;
}

// Test edge cases and boundary conditions
TEST_F(HeuristicSortingUnitTest, TestEdgeCases) {
    std::cout << "Testing edge cases..." << std::endl;
    
    pos tpos = createBasicPosition();
    
    // Test single move
    {
        moveType singleMove;
        singleMove.suit = 0;
        singleMove.rank = 14;
        singleMove.weight = 0;
        singleMove.sequence = 1;
        
        auto context = createBasicContext(tpos, &singleMove, 1);
        SortMoves(context);
        
        EXPECT_NE(singleMove.weight, 0) << "Single move should get weight assigned";
    }
    
    // Test maximum moves (13)
    {
        moveType maxMoves[13];
        for (int i = 0; i < 13; i++) {
            maxMoves[i].suit = i % 4;
            maxMoves[i].rank = 14 - (i % 13);
            maxMoves[i].weight = 0;
            maxMoves[i].sequence = 1 << (14 - (i % 13));
        }
        
        auto context = createBasicContext(tpos, maxMoves, 13);
        SortMoves(context);
        
        EXPECT_TRUE(std::any_of(maxMoves, maxMoves + 13, [](const moveType& m) { return m.weight != 0; }))
            << "Maximum moves should get weights assigned";
            
        // Check that moves are sorted
        bool sorted = true;
        for (int i = 1; i < 13; i++) {
            if (maxMoves[i-1].weight < maxMoves[i].weight) {
                sorted = false;
                break;
            }
        }
        EXPECT_TRUE(sorted) << "Maximum moves should be properly sorted";
    }
    
    std::cout << "Edge cases tested successfully!" << std::endl;
}

