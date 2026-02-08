/*
   Unit tests for TransTable integration scenarios.
   
   Tests cross-implementation consistency, real game scenarios,
   interface compliance, and end-to-end integration.
*/


#include <gtest/gtest.h>
#include <memory>

#include "trans_table/trans_table.hpp"
#include "trans_table/trans_table_s.hpp"
#include "trans_table/trans_table_l.hpp"
#include "mock_data_generators.hpp"

namespace dds_test {

using TestScenario = MockDataFactory::TestScenario;


class TransTableIntegrationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        factory = std::make_unique<MockDataFactory>(12345);
        scenario = factory->CreateBasicScenario();
        tt_s_ = std::make_unique<TransTableS>();
        tt_l_ = std::make_unique<TransTableL>();
        // Initialize with basic scenario data
        tt_s_->init(scenario.handLookup);
        tt_l_->init(scenario.handLookup);
        tt_s_->set_memory_default(DefaultMemoryMb);
        tt_l_->set_memory_default(DefaultMemoryMb);
        tt_s_->make_tt();
        tt_l_->make_tt();
    }
    void TearDown() override
    {
        if (tt_s_) {
            tt_s_->return_all_memory();
        }
        if (tt_l_) {
            tt_l_->return_all_memory();
        }
    }
    static constexpr int DefaultMemoryMb = 64;
    std::unique_ptr<MockDataFactory> factory;
    TestScenario scenario;
    std::unique_ptr<TransTableS> tt_s_;
    std::unique_ptr<TransTableL> tt_l_;
};

// ============================================================================
// Cross-Implementation Consistency Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, BothImplementationsCanBeCreated)
{
    EXPECT_NE(tt_s_, nullptr);
    EXPECT_NE(tt_l_, nullptr);
}

TEST_F(TransTableIntegrationTest, ObjectsCanBeDestroyed)
{
    // Test that objects can be destroyed without issues
    auto temp_s = std::make_unique<TransTableS>();
    auto temp_l = std::make_unique<TransTableL>();
    // Destruction happens automatically
}

TEST_F(TransTableIntegrationTest, BasicMethodsExist)
{
    // Test that basic methods can be called without crashing
    EXPECT_NO_THROW(tt_s_->memory_in_use());
    EXPECT_NO_THROW(tt_l_->memory_in_use());
}

// ============================================================================
// Interface Compliance Tests  
// ============================================================================

TEST_F(TransTableIntegrationTest, BothImplementationsInheritFromTransTable)
{
    // Verify both implementations can be used polymorphically
    const double mem_s = tt_s_->memory_in_use();
    const double mem_l = tt_l_->memory_in_use();

    EXPECT_GE(mem_s, 0.0);
    EXPECT_GE(mem_l, 0.0);

    // Should be able to reset through interface
    EXPECT_NO_THROW(tt_s_->reset_memory(ResetReason::NewDeal));
    EXPECT_NO_THROW(tt_l_->reset_memory(ResetReason::NewDeal));
}

TEST_F(TransTableIntegrationTest, VirtualMethodsWorkCorrectly)
{
    // Test virtual method dispatch works
    const double memory1_s = tt_s_->memory_in_use();
    const double memory1_l = tt_l_->memory_in_use();

    // Create test data using available factory methods
    auto s = factory->CreateBasicScenario();
    tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);

    const double memory2_s = tt_s_->memory_in_use();
    const double memory2_l = tt_l_->memory_in_use();

    // Memory usage should increase (or at least not decrease)
    EXPECT_GE(memory2_s, memory1_s);
    EXPECT_GE(memory2_l, memory1_l);

    // Should be able to lookup
    bool lower_flag_s = false, lower_flag_l = false;
    auto result_s = tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_s);
    auto result_l = tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_l);
    
    EXPECT_NE(resultS, nullptr);
    EXPECT_NE(resultL, nullptr);
}

// ============================================================================
// Real Game Scenario Tests
// ============================================================================

TEST_F(TransTableIntegrationTest, BasicDataOperations)
{
    // Test basic add and lookup operations
    auto s = factory->CreateBasicScenario();

    // Add data to both tables
    tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);

    // Lookup should find the data
    bool lower_flag_s = false, lower_flag_l = false;
    auto result_s = tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_s);
    auto result_l = tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_l);
    
    EXPECT_NE(resultS, nullptr);
    EXPECT_NE(resultL, nullptr);
}

TEST_F(TransTableIntegrationTest, MultipleScenarios)
{
    // Test with multiple different scenarios
    for (int i = 0; i < 5; ++i) {
        auto s = factory->CreateBasicScenario();
        s.trick = i + 1;  // Vary the trick number

        tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);

        bool lower_flag_s = false, lower_flag_l = false;
        auto result_s = tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_s);
        auto result_l = tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_l);

        EXPECT_NE(result_s, nullptr) << "Failed for scenario " << i;
        EXPECT_NE(result_l, nullptr) << "Failed for scenario " << i;
    }
}

TEST_F(TransTableIntegrationTest, ResultConsistency)
{
    // Test that both implementations return consistent results
    auto s = factory->CreateBasicScenario();

    // Add identical data to both tables
    tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);

    // Lookup should return consistent results
    bool lower_flag_s = false, lower_flag_l = false;
    auto result_s = tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_s);
    auto result_l = tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag_l);

    ASSERT_NE(result_s, nullptr);
    ASSERT_NE(result_l, nullptr);

    // Results should be equivalent
    EXPECT_EQ(result_s->upper_bound, result_l->upper_bound);
    EXPECT_EQ(result_s->lower_bound, result_l->lower_bound);
    EXPECT_EQ(lower_flag_s, lower_flag_l);
}

} // namespace dds_test
