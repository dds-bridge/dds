/*
   Unit tests for TransTable integration scenarios.
   
   Tests cross-implementation consistency, real game scenarios,
   interface compliance, and end-to-end integration.
*/


#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>

#include "library/src/trans_table/TransTable.h"
#include "library/src/trans_table/TransTableS.h"
#include "library/src/trans_table/TransTableL.h"
#include "library/tests/trans_table/mock_data_generators.h"

using dds_test::MockDataFactory;
using TestScenario = dds_test::MockDataFactory::TestScenario;


class TransTableIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory = std::make_unique<MockDataFactory>(12345);
        scenario = factory->CreateBasicScenario();
        ttS = std::make_unique<TransTableS>();
        ttL = std::make_unique<TransTableL>();
        ttS->Init(scenario.handLookup);
        ttL->Init(scenario.handLookup);
        ttS->SetMemoryDefault(DEFAULT_MEMORY_MB);
        ttL->SetMemoryDefault(DEFAULT_MEMORY_MB);
        ttS->MakeTT();
        ttL->MakeTT();
    }
    void TearDown() override {
        ttS->ReturnAllMemory();
        ttL->ReturnAllMemory();
    }
    static constexpr int DEFAULT_MEMORY_MB = 64;
    std::unique_ptr<MockDataFactory> factory;
    TestScenario scenario;
    std::unique_ptr<TransTableS> ttS;
    std::unique_ptr<TransTableL> ttL;
};

// ============================================================================
// Cross-Implementation Consistency Tests
// ============================================================================

// ============================================================================
// Interface Compliance Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, BothImplementationsInheritProperly) {
    TransTable* baseS = ttS.get();
    TransTable* baseL = ttL.get();
    ASSERT_NE(baseS, nullptr);
    ASSERT_NE(baseL, nullptr);
    EXPECT_NO_THROW(baseS->MemoryInUse());
    EXPECT_NO_THROW(baseL->MemoryInUse());
    EXPECT_NO_THROW(baseS->ResetMemory(TT_RESET_NEW_DEAL));
    EXPECT_NO_THROW(baseL->ResetMemory(TT_RESET_NEW_DEAL));
}

TEST_F(TransTableIntegrationTest, VirtualMethodDispatchWorksCorrectly) {
    std::vector<TransTable*> tables = {ttS.get(), ttL.get()};
    for (auto* table : tables) {
        double memory1 = table->MemoryInUse();
        auto s = factory->CreateBasicScenario();
        table->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        double memory2 = table->MemoryInUse();
        EXPECT_GE(memory2, memory1);
        bool lowerFlag = false;
        auto result = table->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
        EXPECT_NE(result, nullptr);
        EXPECT_NO_THROW(table->ResetMemory(TT_RESET_NEW_DEAL));
    }
}

TEST_F(TransTableIntegrationTest, PolymorphicUsageScenarios) {
    auto testTablePolymorphically = [this](TransTable* table) {
        table->SetMemoryDefault(32);
        table->MakeTT();
        auto s = factory->CreateBasicScenario();
        table->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        bool lowerFlag = false;
        auto result = table->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
        return result != nullptr;
    };
    EXPECT_TRUE(testTablePolymorphically(ttS.get()));
    EXPECT_TRUE(testTablePolymorphically(ttL.get()));
}

TEST_F(TransTableIntegrationTest, InterfaceContractsRespected) {
    double memS = ttS->MemoryInUse();
    double memL = ttL->MemoryInUse();
    EXPECT_GE(memS, 0.0);
    EXPECT_GE(memL, 0.0);
    auto s = factory->CreateBasicScenario();
    ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto resultL = ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
    EXPECT_NE(resultS, nullptr);
    EXPECT_NE(resultL, nullptr);
    ttS->ResetMemory(TT_RESET_NEW_DEAL);
    ttL->ResetMemory(TT_RESET_NEW_DEAL);
    auto emptyResultS = ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto emptyResultL = ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
    EXPECT_EQ(emptyResultS, nullptr);
    EXPECT_EQ(emptyResultL, nullptr);
}

TEST_F(TransTableIntegrationTest, TransTableSAndLReturnEquivalentResults) {
    auto s = factory->CreateBasicScenario();
    ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto resultL = ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
    ASSERT_NE(resultS, nullptr);
    ASSERT_NE(resultL, nullptr);
    EXPECT_EQ(resultS->ubound, resultL->ubound);
    EXPECT_EQ(resultS->lbound, resultL->lbound);
    EXPECT_EQ(resultS->bestMoveSuit, resultL->bestMoveSuit);
    EXPECT_EQ(resultS->bestMoveRank, resultL->bestMoveRank);
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        EXPECT_EQ(resultS->leastWin[suit], resultL->leastWin[suit]);
    }
    EXPECT_EQ(lowerFlagS, lowerFlagL);
}

TEST_F(TransTableIntegrationTest, MemoryUsageDifferencesWithinExpectedRanges) {
    auto scenarios = factory->CreateTestSuite(10);
    for (const auto& s : scenarios) {
        ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    }
    double memoryS = ttS->MemoryInUse();
    double memoryL = ttL->MemoryInUse();
    EXPECT_GT(memoryS, 0.0);
    EXPECT_GT(memoryL, 0.0);
    EXPECT_LT(memoryL / memoryS, 10.0);
    EXPECT_LT(memoryS, DEFAULT_MEMORY_MB * 1.1);
    EXPECT_LT(memoryL, DEFAULT_MEMORY_MB * 1.1);
}

TEST_F(TransTableIntegrationTest, PerformanceCharacteristicsVerification) {
    const int NUM_OPERATIONS = 100;
    auto scenarios = factory->CreateTestSuite(NUM_OPERATIONS);
    auto startS = std::chrono::high_resolution_clock::now();
    for (const auto& s : scenarios) {
        ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        bool lowerFlag = false;
        ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
    }
    auto endS = std::chrono::high_resolution_clock::now();
    auto durationS = std::chrono::duration_cast<std::chrono::microseconds>(endS - startS);
    auto startL = std::chrono::high_resolution_clock::now();
    for (const auto& s : scenarios) {
        ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        bool lowerFlag = false;
        ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
    }
    auto endL = std::chrono::high_resolution_clock::now();
    auto durationL = std::chrono::duration_cast<std::chrono::microseconds>(endL - startL);
    double ratio = static_cast<double>(durationL.count()) / durationS.count();
    EXPECT_LT(ratio, 10.0);
    EXPECT_GT(ratio, 0.1);
}

TEST_F(TransTableIntegrationTest, BothImplementationsHandleSameEdgeCases) {
    // Test case 1: Maximum trick number
    auto mockData = MockPositionGenerator::createGamePosition();
    int maxTrick = 12;
    int hand = 0;
    
    // Both should handle without crashing
    EXPECT_NO_THROW(ttS->Add(maxTrick, hand, mockData.aggrTarget, 
                            mockData.winRanks, mockData.nodeCard, false));
    EXPECT_NO_THROW(ttL->Add(maxTrick, hand, mockData.aggrTarget, 
                            mockData.winRanks, mockData.nodeCard, false));
    
    // Test case 2: All hands
    for (int testHand = 0; testHand < DDS_HANDS; testHand++) {
        auto data = MockPositionGenerator::createVariablePosition(testHand);
        
        EXPECT_NO_THROW(ttS->Add(4, testHand, data.aggrTarget, 
                                data.winRanks, data.nodeCard, false));
        EXPECT_NO_THROW(ttL->Add(4, testHand, data.aggrTarget, 
                                data.winRanks, data.nodeCard, false));
    }
    
    // Test case 3: Empty lookup after reset
    ttS->ResetMemory(TT_RESET_NEW_DEAL);
    ttL->ResetMemory(TT_RESET_NEW_DEAL);
    
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->Lookup(4, 0, mockData.aggrTarget, 
                               mockData.handDist, 10, lowerFlagS);
    auto resultL = ttL->Lookup(4, 0, mockData.aggrTarget, 
                               mockData.handDist, 10, lowerFlagL);
    
    // Both should return null for empty table
    EXPECT_EQ(resultS, nullptr);
    EXPECT_EQ(resultL, nullptr);
}

// ============================================================================
// Real Game Scenario Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, ActualBridgeHandData) {
    // Simulate a realistic bridge hand scenario
    // North: S:AKQ H:AKQ D:AKQ C:A (13 HCP, strong hand)
    auto strongHand = MockPositionGenerator::createStrongHandPosition();
    
    // Add position for trick 4 (early in game)
    ttS->Add(4, 0, strongHand.aggrTarget, strongHand.winRanks, 
             strongHand.nodeCard, false);
    ttL->Add(4, 0, strongHand.aggrTarget, strongHand.winRanks, 
             strongHand.nodeCard, false);
    
    // Lookup should find the position
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->Lookup(4, 0, strongHand.aggrTarget, 
                               strongHand.handDist, 10, lowerFlagS);
    auto resultL = ttL->Lookup(4, 0, strongHand.aggrTarget, 
                               strongHand.handDist, 10, lowerFlagL);
    
    ASSERT_NE(resultS, nullptr);
    ASSERT_NE(resultL, nullptr);
    
    // Results should be consistent
    EXPECT_EQ(resultS->ubound, resultL->ubound);
    EXPECT_EQ(resultS->lbound, resultL->lbound);
}

TEST_F(TransTableIntegrationTest, MultiTrickSequences) {
    // Simulate a sequence of tricks being played
    std::vector<MockPositionGenerator::GamePosition> trickSequence;
    
    // Generate positions for tricks 4, 8, 12 (typical storage points)
    for (int trick : {4, 8, 12}) {
        trickSequence.push_back(MockPositionGenerator::createTrickPosition(trick));
    }
    
    // Add all positions to both tables
    for (size_t i = 0; i < trickSequence.size(); i++) {
        const auto& pos = trickSequence[i];
        int trick = 4 + static_cast<int>(i) * 4;
        
        ttS->Add(trick, 0, pos.aggrTarget, pos.winRanks, pos.nodeCard, false);
        ttL->Add(trick, 0, pos.aggrTarget, pos.winRanks, pos.nodeCard, false);
    }
    
    // Verify all positions can be retrieved
    for (size_t i = 0; i < trickSequence.size(); i++) {
        const auto& pos = trickSequence[i];
        int trick = 4 + static_cast<int>(i) * 4;
        
        bool lowerFlagS = false, lowerFlagL = false;
        auto resultS = ttS->Lookup(trick, 0, pos.aggrTarget, 
                                   pos.handDist, 10, lowerFlagS);
        auto resultL = ttL->Lookup(trick, 0, pos.aggrTarget, 
                                   pos.handDist, 10, lowerFlagL);
        
        ASSERT_NE(resultS, nullptr) << "Trick " << trick << " not found in TransTableS";
        ASSERT_NE(resultL, nullptr) << "Trick " << trick << " not found in TransTableL";
        
        // Verify consistency between implementations
        EXPECT_EQ(resultS->ubound, resultL->ubound);
        EXPECT_EQ(resultS->lbound, resultL->lbound);
    }
}

TEST_F(TransTableIntegrationTest, DifferentTrumpSuits) {
    // Test with different trump configurations
    for (int trump = 0; trump < DDS_SUITS; trump++) {
        auto trumpData = MockPositionGenerator::createTrumpPosition(trump);
        
        ttS->Add(4, 0, trumpData.aggrTarget, trumpData.winRanks, 
                trumpData.nodeCard, false);
        ttL->Add(4, 0, trumpData.aggrTarget, trumpData.winRanks, 
                trumpData.nodeCard, false);
        
        bool lowerFlagS = false, lowerFlagL = false;
        auto resultS = ttS->Lookup(4, 0, trumpData.aggrTarget, 
                                   trumpData.handDist, 10, lowerFlagS);
        auto resultL = ttL->Lookup(4, 0, trumpData.aggrTarget, 
                                   trumpData.handDist, 10, lowerFlagL);
        
        ASSERT_NE(resultS, nullptr) << "Trump suit " << trump << " not found in TransTableS";
        ASSERT_NE(resultL, nullptr) << "Trump suit " << trump << " not found in TransTableL";
        
        EXPECT_EQ(resultS->ubound, resultL->ubound);
        EXPECT_EQ(resultS->lbound, resultL->lbound);
    }
}

TEST_F(TransTableIntegrationTest, ComplexWinningRankPatterns) {
    // Test with various winning rank patterns
    std::vector<std::vector<int>> winPatterns = {
        {14, 13, 12, 11}, // A-K-Q-J sequence
        {14, 12, 10, 8},  // A-Q-T-8 gaps
        {9, 8, 7, 6},     // Middle cards
        {5, 4, 3, 2}      // Low cards
    };
    
    for (size_t pattern = 0; pattern < winPatterns.size(); pattern++) {
        auto patternData = MockPositionGenerator::createWinRankPattern(winPatterns[pattern]);
        
        ttS->Add(4, 0, patternData.aggrTarget, patternData.winRanks, 
                patternData.nodeCard, false);
        ttL->Add(4, 0, patternData.aggrTarget, patternData.winRanks, 
                patternData.nodeCard, false);
        
        bool lowerFlagS = false, lowerFlagL = false;
        auto resultS = ttS->Lookup(4, 0, patternData.aggrTarget, 
                                   patternData.handDist, 10, lowerFlagS);
        auto resultL = ttL->Lookup(4, 0, patternData.aggrTarget, 
                                   patternData.handDist, 10, lowerFlagL);
        
        ASSERT_NE(resultS, nullptr) << "Pattern " << pattern << " not found in TransTableS";
        ASSERT_NE(resultL, nullptr) << "Pattern " << pattern << " not found in TransTableL";
        
        EXPECT_EQ(resultS->ubound, resultL->ubound);
        EXPECT_EQ(resultS->lbound, resultL->lbound);
    }
}

// ============================================================================
// Interface Compliance Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, BothImplementationsInheritProperly) {
    // Verify both implementations inherit from TransTable
    TransTable* baseS = ttS.get();
    TransTable* baseL = ttL.get();
    
    ASSERT_NE(baseS, nullptr);
    ASSERT_NE(baseL, nullptr);
    
    // Should be able to call virtual methods through base pointer
    EXPECT_NO_THROW(baseS->MemoryInUse());
    EXPECT_NO_THROW(baseL->MemoryInUse());
    
    // Should be able to reset through base pointer
    EXPECT_NO_THROW(baseS->ResetMemory(TT_RESET_NEW_DEAL));
    EXPECT_NO_THROW(baseL->ResetMemory(TT_RESET_NEW_DEAL));
}

TEST_F(TransTableIntegrationTest, VirtualMethodDispatchWorksCorrectly) {
    std::vector<TransTable*> tables = {ttS.get(), ttL.get()};
    
    for (auto* table : tables) {
        // Test virtual method dispatch
        double memory1 = table->MemoryInUse();
        
        // Add some data
        auto mockData = MockPositionGenerator::createGamePosition();
        table->Add(4, 0, mockData.aggrTarget, mockData.winRanks, 
                  mockData.nodeCard, false);
        
        double memory2 = table->MemoryInUse();
        
        // Memory usage should increase (or at least not decrease)
        EXPECT_GE(memory2, memory1);
        
        // Should be able to lookup
        bool lowerFlag = false;
        auto result = table->Lookup(4, 0, mockData.aggrTarget, 
                                   mockData.handDist, 10, lowerFlag);
        EXPECT_NE(result, nullptr);
        
        // Should be able to reset
        EXPECT_NO_THROW(table->ResetMemory(TT_RESET_NEW_DEAL));
    }
}

TEST_F(TransTableIntegrationTest, PolymorphicUsageScenarios) {
    // Create a function that uses TransTable polymorphically
    auto testTablePolymorphically = [](TransTable* table) {
        table->SetMemoryDefault(32);
        table->MakeTT();
        
        auto mockData = MockPositionGenerator::createGamePosition();
        table->Add(4, 0, mockData.aggrTarget, mockData.winRanks, 
                  mockData.nodeCard, false);
        
        bool lowerFlag = false;
        auto result = table->Lookup(4, 0, mockData.aggrTarget, 
                                   mockData.handDist, 10, lowerFlag);
        return result != nullptr;
    };
    
    // Test both implementations polymorphically
    EXPECT_TRUE(testTablePolymorphically(ttS.get()));
    EXPECT_TRUE(testTablePolymorphically(ttL.get()));
}

TEST_F(TransTableIntegrationTest, InterfaceContractsRespected) {
    // Test that both implementations respect the base interface contracts
    
    // 1. Memory methods should return consistent values
    double memS = ttS->MemoryInUse();
    double memL = ttL->MemoryInUse();
    EXPECT_GE(memS, 0.0);
    EXPECT_GE(memL, 0.0);
    
    // 2. Add/Lookup contract: what you add should be retrievable
    auto testData = MockPositionGenerator::createGamePosition();
    
    ttS->Add(4, 0, testData.aggrTarget, testData.winRanks, testData.nodeCard, false);
    ttL->Add(4, 0, testData.aggrTarget, testData.winRanks, testData.nodeCard, false);
    
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->Lookup(4, 0, testData.aggrTarget, testData.handDist, 10, lowerFlagS);
    auto resultL = ttL->Lookup(4, 0, testData.aggrTarget, testData.handDist, 10, lowerFlagL);
    
    EXPECT_NE(resultS, nullptr);
    EXPECT_NE(resultL, nullptr);
    
    // 3. Reset contract: after reset, lookups should fail
    ttS->ResetMemory(TT_RESET_NEW_DEAL);
    ttL->ResetMemory(TT_RESET_NEW_DEAL);
    
    auto emptyResultS = ttS->Lookup(4, 0, testData.aggrTarget, testData.handDist, 10, lowerFlagS);
    auto emptyResultL = ttL->Lookup(4, 0, testData.aggrTarget, testData.handDist, 10, lowerFlagL);
    
    EXPECT_EQ(emptyResultS, nullptr);
    EXPECT_EQ(emptyResultL, nullptr);
}

// ============================================================================
// End-to-End Integration Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, FullGameSequenceHandling) {
    // Simulate a complete game sequence
    const int TOTAL_TRICKS = 13;
    const int STORAGE_INTERVAL = 4; // Store at tricks 4, 8, 12
    
    std::vector<MockPositionGenerator::GamePosition> gamePositions;
    
    // Generate positions for each storage point
    for (int trick = STORAGE_INTERVAL; trick <= TOTAL_TRICKS - 1; trick += STORAGE_INTERVAL) {
        gamePositions.push_back(MockPositionGenerator::createTrickPosition(trick));
    }
    
    // Add all positions to both tables
    for (size_t i = 0; i < gamePositions.size(); i++) {
        int trick = STORAGE_INTERVAL + static_cast<int>(i) * STORAGE_INTERVAL;
        const auto& pos = gamePositions[i];
        
        ttS->Add(trick, 0, pos.aggrTarget, pos.winRanks, pos.nodeCard, false);
        ttL->Add(trick, 0, pos.aggrTarget, pos.winRanks, pos.nodeCard, false);
    }
    
    // Verify all positions are retrievable and consistent
    for (size_t i = 0; i < gamePositions.size(); i++) {
        int trick = STORAGE_INTERVAL + static_cast<int>(i) * STORAGE_INTERVAL;
        const auto& pos = gamePositions[i];
        
        bool lowerFlagS = false, lowerFlagL = false;
        auto resultS = ttS->Lookup(trick, 0, pos.aggrTarget, pos.handDist, 10, lowerFlagS);
        auto resultL = ttL->Lookup(trick, 0, pos.aggrTarget, pos.handDist, 10, lowerFlagL);
        
        ASSERT_NE(resultS, nullptr) << "Game sequence trick " << trick << " failed in TransTableS";
        ASSERT_NE(resultL, nullptr) << "Game sequence trick " << trick << " failed in TransTableL";
        
        EXPECT_EQ(resultS->ubound, resultL->ubound) << "Inconsistent bounds at trick " << trick;
        EXPECT_EQ(resultS->lbound, resultL->lbound) << "Inconsistent bounds at trick " << trick;
    }
}

TEST_F(TransTableIntegrationTest, MemoryManagementAcrossGameStates) {
    // Test memory management across multiple game states
    double initialMemoryS = ttS->MemoryInUse();
    double initialMemoryL = ttL->MemoryInUse();
    
    // Fill tables with data from first game
    for (int i = 0; i < 20; i++) {
        auto gameData = MockPositionGenerator::createVariablePosition(i);
        int trick = 4 + (i % 3) * 4;
        int hand = i % DDS_HANDS;
        
        ttS->Add(trick, hand, gameData.aggrTarget, gameData.winRanks, gameData.nodeCard, false);
        ttL->Add(trick, hand, gameData.aggrTarget, gameData.winRanks, gameData.nodeCard, false);
    }
    
    double midGameMemoryS = ttS->MemoryInUse();
    double midGameMemoryL = ttL->MemoryInUse();
    
    // Memory should have increased
    EXPECT_GT(midGameMemoryS, initialMemoryS);
    EXPECT_GT(midGameMemoryL, initialMemoryL);
    
    // Reset for new game
    ttS->ResetMemory(TT_RESET_NEW_DEAL);
    ttL->ResetMemory(TT_RESET_NEW_DEAL);
    
    double postResetMemoryS = ttS->MemoryInUse();
    double postResetMemoryL = ttL->MemoryInUse();
    
    // Memory should decrease after reset (may not be exactly initial due to allocation patterns)
    EXPECT_LE(postResetMemoryS, midGameMemoryS);
    EXPECT_LE(postResetMemoryL, midGameMemoryL);
    
    // Add data from second game
    for (int i = 20; i < 40; i++) {
        auto gameData = MockPositionGenerator::createVariablePosition(i);
        int trick = 4 + (i % 3) * 4;
        int hand = i % DDS_HANDS;
        
        ttS->Add(trick, hand, gameData.aggrTarget, gameData.winRanks, gameData.nodeCard, false);
        ttL->Add(trick, hand, gameData.aggrTarget, gameData.winRanks, gameData.nodeCard, false);
    }
    
    // Memory management should still work correctly
    double finalMemoryS = ttS->MemoryInUse();
    double finalMemoryL = ttL->MemoryInUse();
    
    EXPECT_GT(finalMemoryS, postResetMemoryS);
    EXPECT_GT(finalMemoryL, postResetMemoryL);
}

TEST_F(TransTableIntegrationTest, PositionStorageAndRetrievalConsistency) {
    // Test that positions stored and retrieved maintain consistency
    const int NUM_POSITIONS = 50;
    std::vector<MockPositionGenerator::GamePosition> storedPositions;
    
    // Store positions with known data
    for (int i = 0; i < NUM_POSITIONS; i++) {
        auto pos = MockPositionGenerator::createKnownPosition(i);
        storedPositions.push_back(pos);
        
        int trick = 4 + (i % 3) * 4;
        int hand = i % DDS_HANDS;
        
        ttS->Add(trick, hand, pos.aggrTarget, pos.winRanks, pos.nodeCard, false);
        ttL->Add(trick, hand, pos.aggrTarget, pos.winRanks, pos.nodeCard, false);
    }
    
    // Retrieve and verify all positions
    for (int i = 0; i < NUM_POSITIONS; i++) {
        const auto& originalPos = storedPositions[i];
        int trick = 4 + (i % 3) * 4;
        int hand = i % DDS_HANDS;
        
        bool lowerFlagS = false, lowerFlagL = false;
        auto resultS = ttS->Lookup(trick, hand, originalPos.aggrTarget, 
                                   originalPos.handDist, 10, lowerFlagS);
        auto resultL = ttL->Lookup(trick, hand, originalPos.aggrTarget, 
                                   originalPos.handDist, 10, lowerFlagL);
        
        ASSERT_NE(resultS, nullptr) << "Position " << i << " not found in TransTableS";
        ASSERT_NE(resultL, nullptr) << "Position " << i << " not found in TransTableL";
        
        // Verify data integrity
        EXPECT_EQ(resultS->ubound, originalPos.nodeCard.ubound) 
            << "Upper bound mismatch for position " << i << " in TransTableS";
        EXPECT_EQ(resultL->ubound, originalPos.nodeCard.ubound) 
            << "Upper bound mismatch for position " << i << " in TransTableL";
            
        EXPECT_EQ(resultS->lbound, originalPos.nodeCard.lbound) 
            << "Lower bound mismatch for position " << i << " in TransTableS";
        EXPECT_EQ(resultL->lbound, originalPos.nodeCard.lbound) 
            << "Lower bound mismatch for position " << i << " in TransTableL";
            
        // Verify consistency between implementations
        EXPECT_EQ(resultS->ubound, resultL->ubound) 
            << "Inconsistent upper bound between implementations for position " << i;
        EXPECT_EQ(resultS->lbound, resultL->lbound) 
            << "Inconsistent lower bound between implementations for position " << i;
    }
}

TEST_F(TransTableIntegrationTest, SearchOptimizationIntegration) {
    // Test integration with search optimization scenarios
    
    // Scenario 1: Alpha-beta cutoff simulation
    auto cutoffPosition = MockPositionGenerator::createAlphaBetaCutoffPosition();
    ttS->Add(4, 0, cutoffPosition.aggrTarget, cutoffPosition.winRanks, 
             cutoffPosition.nodeCard, true); // flag=true for cutoff
    ttL->Add(4, 0, cutoffPosition.aggrTarget, cutoffPosition.winRanks, 
             cutoffPosition.nodeCard, true);
    
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->Lookup(4, 0, cutoffPosition.aggrTarget, 
                               cutoffPosition.handDist, 8, lowerFlagS); // limit < stored ubound
    auto resultL = ttL->Lookup(4, 0, cutoffPosition.aggrTarget, 
                               cutoffPosition.handDist, 8, lowerFlagL);
    
    ASSERT_NE(resultS, nullptr);
    ASSERT_NE(resultL, nullptr);
    
    // Should handle search limits correctly
    EXPECT_EQ(lowerFlagS, lowerFlagL);
    
    // Scenario 2: Exact value storage
    auto exactPosition = MockPositionGenerator::createExactValuePosition();
    ttS->Add(8, 1, exactPosition.aggrTarget, exactPosition.winRanks, 
             exactPosition.nodeCard, false); // exact value
    ttL->Add(8, 1, exactPosition.aggrTarget, exactPosition.winRanks, 
             exactPosition.nodeCard, false);
    
    auto exactResultS = ttS->Lookup(8, 1, exactPosition.aggrTarget, 
                                    exactPosition.handDist, 15, lowerFlagS);
    auto exactResultL = ttL->Lookup(8, 1, exactPosition.aggrTarget, 
                                    exactPosition.handDist, 15, lowerFlagL);
    
    ASSERT_NE(exactResultS, nullptr);
    ASSERT_NE(exactResultL, nullptr);
    
    // Exact values should match
    EXPECT_EQ(exactResultS->ubound, exactResultS->lbound);
    EXPECT_EQ(exactResultL->ubound, exactResultL->lbound);
    EXPECT_EQ(exactResultS->ubound, exactResultL->ubound);
}
