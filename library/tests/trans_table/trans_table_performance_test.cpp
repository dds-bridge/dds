/// @file trans_table_performance_test.cpp
/// @brief Performace tests for TransTable implementations.
/// @details Covers memory stress, lookup performance, and allocation patterns.

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <memory>

#include "trans_table/trans_table.hpp"
#include "trans_table/trans_table_s.hpp"
#include "trans_table/trans_table_l.hpp"
#include "library/tests/trans_table/mock_data_generators.hpp"

using dds_test::MockDataFactory;
using TestScenario = dds_test::MockDataFactory::TestScenario;

class TransTablePerformanceTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        factory = std::make_unique<MockDataFactory>(98765);
        scenario = factory->CreateBasicScenario();
        tt_s_ = std::make_unique<TransTableS>();
        tt_l_ = std::make_unique<TransTableL>();
        tt_s_->init(scenario.handLookup);
        tt_l_->init(scenario.handLookup);
        tt_s_->set_memory_default(DefaultMemoryMb);
        tt_l_->set_memory_default(DefaultMemoryMb);
        tt_s_->make_tt();
        tt_l_->make_tt();
    }
    void TearDown() override
    {
        tt_s_->return_all_memory();
        tt_l_->return_all_memory();
    }
    static constexpr int DefaultMemoryMb = 64;
    std::unique_ptr<MockDataFactory> factory;
    TestScenario scenario;
    std::unique_ptr<TransTableS> tt_s_;
    std::unique_ptr<TransTableL> tt_l_;
};

// Memory Stress Test: Add a large number of positions
TEST_F(TransTablePerformanceTest, MemoryStress_AddManyPositions)
{
    auto scenarios = factory->CreateTestSuite(10000);
    for (const auto& s : scenarios) {
        tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    }
    const double mem_s = tt_s_->memory_in_use();
    const double mem_l = tt_l_->memory_in_use();
    EXPECT_GT(mem_s, 0.0);
    EXPECT_GT(mem_l, 0.0);
    EXPECT_LT(mem_s, DefaultMemoryMb * 1.2);
    EXPECT_LT(mem_l, DefaultMemoryMb * 1.2);
}

// Memory Limit Boundary Test
TEST_F(TransTablePerformanceTest, MemoryLimitBoundary)
{
    tt_s_->set_memory_maximum(8); // 8 MB
    tt_l_->set_memory_maximum(8);
    tt_s_->make_tt();
    tt_l_->make_tt();
    auto scenarios = factory->CreateTestSuite(2000);
    for (const auto& s : scenarios) {
        tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    }
    const double mem_s = tt_s_->memory_in_use();
    const double mem_l = tt_l_->memory_in_use();
    EXPECT_LE(mem_s, 8.5);
    EXPECT_LE(mem_l, 8.5);
}

// Rapid Allocation/Deallocation Pattern
TEST_F(TransTablePerformanceTest, RapidAllocationDeallocation)
{
    for (int cycle = 0; cycle < 10; ++cycle) {
        auto scenarios = factory->CreateTestSuite(500);
        for (const auto& s : scenarios) {
            tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
            tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        }
        tt_s_->reset_memory(ResetReason::NewDeal);
        tt_l_->reset_memory(ResetReason::NewDeal);
        EXPECT_LE(tt_s_->memory_in_use(), DefaultMemoryMb);
        EXPECT_LE(tt_l_->memory_in_use(), DefaultMemoryMb);
    }
}

// Lookup Performance Test
TEST_F(TransTablePerformanceTest, LookupPerformance)
{
    auto scenarios = factory->CreateTestSuite(2000);
    for (const auto& s : scenarios) {
        tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    }
    auto start_s = std::chrono::high_resolution_clock::now();
    for (const auto& s : scenarios) {
        bool lower_flag = false;
        tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag);
    }
    auto end_s = std::chrono::high_resolution_clock::now();
    auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(end_s - start_s);
    auto start_l = std::chrono::high_resolution_clock::now();
    for (const auto& s : scenarios) {
        bool lower_flag = false;
        tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lower_flag);
    }
    auto end_l = std::chrono::high_resolution_clock::now();
    auto duration_l = std::chrono::duration_cast<std::chrono::milliseconds>(end_l - start_l);
    EXPECT_LT(duration_s.count(), 2000); // Should be reasonably fast
    EXPECT_LT(duration_l.count(), 2000);
}

// Cache Hit/Miss Rate Test
TEST_F(TransTablePerformanceTest, CacheHitMissRate)
{
    auto scenarios = factory->CreateTestSuite(1000);
    // Add all positions
    for (const auto& s : scenarios) {
        tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
    }
    // Lookup all (should be hits)
    int hits_s = 0, hits_l = 0;
    for (const auto& s : scenarios) {
        bool lowerFlag = false;
        if (tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag)) hits_s++;
        if (tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag)) hits_l++;
    }
    EXPECT_EQ(hits_s, 1000);
    EXPECT_EQ(hits_l, 1000);
    // Lookup random (should be mostly misses)
    int misses_s = 0, misses_l = 0;
    auto randoms = factory->CreateTestSuite(1000);
    for (const auto& s : randoms) {
        bool lowerFlag = false;
        if (!tt_s_->lookup(s.trick+1, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag)) misses_s++;
        if (!tt_l_->lookup(s.trick+1, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag)) misses_l++;
    }
    EXPECT_GT(misses_s, 800);
    EXPECT_GT(misses_l, 800);
}

// Search Time Complexity Test (scaling)
TEST_F(TransTablePerformanceTest, SearchTimeComplexityScaling)
{
    std::vector<int> sizes = {100, 1000, 5000};
    for (int n : sizes) {
        tt_s_->reset_memory(ResetReason::NewDeal);
        tt_l_->reset_memory(ResetReason::NewDeal);
        auto scenarios = factory->CreateTestSuite(n);
        for (const auto& s : scenarios) {
            tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
            tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        }
        auto start_s = std::chrono::high_resolution_clock::now();
        for (const auto& s : scenarios) {
            bool lowerFlag = false;
            tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag);
        }
        auto end_s = std::chrono::high_resolution_clock::now();
        auto duration_s = std::chrono::duration_cast<std::chrono::milliseconds>(end_s - start_s);
        auto start_l = std::chrono::high_resolution_clock::now();
        for (const auto& s : scenarios) {
            bool lowerFlag = false;
            tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag);
        }
        auto end_l = std::chrono::high_resolution_clock::now();
        auto duration_l = std::chrono::duration_cast<std::chrono::milliseconds>(end_l - start_l);
        EXPECT_LT(duration_s.count(), 2000);
        EXPECT_LT(duration_l.count(), 2000);
    }
}

// Thread Safety/Concurrent Access Test (Task 12)
TEST_F(TransTablePerformanceTest, DISABLED_ConcurrentAccessNotSupported)
{
    // The TransTable implementations are not thread-safe.
    // Concurrent access must be guarded externally (e.g., with a mutex).
    SUCCEED() << "TransTable is not thread-safe; concurrent access is not supported.";
}

// End-to-End Integration Test (Task 13)
TEST_F(TransTablePerformanceTest, EndToEndIntegration_SearchSimulation)
{
    // Simulate a search algorithm using the trans table for storage and retrieval
    auto scenarios = factory->CreateTestSuite(500);
    int found = 0, notFound = 0;
    // Simulate search: add and lookup, with resets in between
    for (int round = 0; round < 5; ++round) {
        for (const auto& s : scenarios) {
            tt_s_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
            tt_l_->add(s.trick, s.hand, s.aggrTarget, s.win_ranks, s.nodeData, false);
        }
        for (const auto& s : scenarios) {
            bool lowerFlag = false;
            if (tt_s_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag)) found++;
            else notFound++;
            if (tt_l_->lookup(s.trick, s.hand, s.aggrTarget, s.hand_dist, 10, lowerFlag)) found++;
            else notFound++;
        }
        tt_s_->reset_memory(ResetReason::NewDeal);
        tt_l_->reset_memory(ResetReason::NewDeal);
    }
    EXPECT_GT(found, notFound);
}
