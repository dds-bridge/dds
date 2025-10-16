/*
   Unit tests for TransTable integration scenarios.
   
   Tests cross-implementation consistency, real game scenarios,
   interface compliance, and end-to-end integration.
*/


#include <gtest/gtest.h>
#include <memory>

#include "trans_table/TransTable.h"
#include "trans_table/TransTableS.h"
#include "trans_table/TransTableL.h"
#include "mock_data_generators.h"

namespace dds_test {

using TestScenario = MockDataFactory::TestScenario;


class TransTableIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory = std::make_unique<MockDataFactory>(12345);
        scenario = factory->CreateBasicScenario();
        ttS = std::make_unique<TransTableS>();
        ttL = std::make_unique<TransTableL>();
        // Initialize with basic scenario data
    ttS->init(scenario.handLookup);
    ttL->init(scenario.handLookup);
    ttS->set_memory_default(DEFAULT_MEMORY_MB);
    ttL->set_memory_default(DEFAULT_MEMORY_MB);
    ttS->make_tt();
    ttL->make_tt();
    }
    void TearDown() override {
    if (ttS) ttS->return_all_memory();
    if (ttL) ttL->return_all_memory();
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

TEST_F(TransTableIntegrationTest, BothImplementationsCanBeCreated) {
    EXPECT_NE(ttS, nullptr);
    EXPECT_NE(ttL, nullptr);
}

TEST_F(TransTableIntegrationTest, ObjectsCanBeDestroyed) {
    // Test that objects can be destroyed without issues
    auto tempS = std::make_unique<TransTableS>();
    auto tempL = std::make_unique<TransTableL>();
    // Destruction happens automatically
}

TEST_F(TransTableIntegrationTest, BasicMethodsExist) {
    // Test that basic methods can be called without crashing
    EXPECT_NO_THROW(ttS->memory_in_use());
    EXPECT_NO_THROW(ttL->memory_in_use());
}

// ============================================================================
// Interface Compliance Tests  
// ============================================================================

TEST_F(TransTableIntegrationTest, BothImplementationsInheritFromTransTable) {
    // Verify both implementations can be used polymorphically
    double memS = ttS->memory_in_use();
    double memL = ttL->memory_in_use();
    
    EXPECT_GE(memS, 0.0);
    EXPECT_GE(memL, 0.0);
    
    // Should be able to reset through interface
    EXPECT_NO_THROW(ttS->reset_memory(ResetReason::NewDeal));
    EXPECT_NO_THROW(ttL->reset_memory(ResetReason::NewDeal));
}

TEST_F(TransTableIntegrationTest, VirtualMethodsWorkCorrectly) {
    // Test virtual method dispatch works
    double memory1S = ttS->memory_in_use();
    double memory1L = ttL->memory_in_use();
    
    // Create test data using available factory methods
    auto s = factory->CreateBasicScenario();
    ttS->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    ttL->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    
    double memory2S = ttS->memory_in_use();
    double memory2L = ttL->memory_in_use();
    
    // Memory usage should increase (or at least not decrease)
    EXPECT_GE(memory2S, memory1S);
    EXPECT_GE(memory2L, memory1L);
    
    // Should be able to lookup
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto resultL = ttL->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
    
    EXPECT_NE(resultS, nullptr);
    EXPECT_NE(resultL, nullptr);
}

// ============================================================================
// Real Game Scenario Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, BasicDataOperations) {
    // Test basic add and lookup operations
    auto s = factory->CreateBasicScenario();
    
    // Add data to both tables
    ttS->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    ttL->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    
    // Lookup should find the data
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto resultL = ttL->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
    
    EXPECT_NE(resultS, nullptr);
    EXPECT_NE(resultL, nullptr);
}

TEST_F(TransTableIntegrationTest, MultipleScenarios) {
    // Test with multiple different scenarios
    for (int i = 0; i < 5; ++i) {
        auto s = factory->CreateBasicScenario();
        s.trick = i + 1;  // Vary the trick number
        
    ttS->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    ttL->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        
        bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto resultL = ttL->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
        
        EXPECT_NE(resultS, nullptr) << "Failed for scenario " << i;
        EXPECT_NE(resultL, nullptr) << "Failed for scenario " << i;
    }
}

TEST_F(TransTableIntegrationTest, ResultConsistency) {
    // Test that both implementations return consistent results
    auto s = factory->CreateBasicScenario();
    
    // Add identical data to both tables
    ttS->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    ttL->add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    
    // Lookup should return consistent results
    bool lowerFlagS = false, lowerFlagL = false;
    auto resultS = ttS->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagS);
    auto resultL = ttL->lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlagL);
    
    ASSERT_NE(resultS, nullptr);
    ASSERT_NE(resultL, nullptr);
    
    // Results should be equivalent
    EXPECT_EQ(resultS->ubound, resultL->ubound);
    EXPECT_EQ(resultS->lbound, resultL->lbound);
    EXPECT_EQ(lowerFlagS, lowerFlagL);
}

} // namespace dds_test
