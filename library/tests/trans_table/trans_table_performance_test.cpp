/*
   Performance and stress tests for TransTable implementations.
   Covers memory stress, lookup performance, and allocation patterns.
*/

#include <gtest/gtest.h>
#include <chrono>
#include <vector>
#include <memory>

#include "library/src/trans_table/TransTable.h"
#include "library/src/trans_table/TransTableS.h"
#include "library/src/trans_table/TransTableL.h"
#include "library/tests/trans_table/mock_data_generators.h"

using dds_test::MockDataFactory;
using TestScenario = dds_test::MockDataFactory::TestScenario;

class TransTablePerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        factory = std::make_unique<MockDataFactory>(98765);
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

// Memory Stress Test: Add a large number of positions
TEST_F(TransTablePerformanceTest, MemoryStress_AddManyPositions) {
    auto scenarios = factory->CreateTestSuite(10000);
    for (const auto& s : scenarios) {
        ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    }
    double memS = ttS->MemoryInUse();
    double memL = ttL->MemoryInUse();
    EXPECT_GT(memS, 0.0);
    EXPECT_GT(memL, 0.0);
    EXPECT_LT(memS, DEFAULT_MEMORY_MB * 1.2);
    EXPECT_LT(memL, DEFAULT_MEMORY_MB * 1.2);
}

// Memory Limit Boundary Test
TEST_F(TransTablePerformanceTest, MemoryLimitBoundary) {
    ttS->SetMemoryMaximum(8); // 8 MB
    ttL->SetMemoryMaximum(8);
    ttS->MakeTT();
    ttL->MakeTT();
    auto scenarios = factory->CreateTestSuite(2000);
    for (const auto& s : scenarios) {
        ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    }
    double memS = ttS->MemoryInUse();
    double memL = ttL->MemoryInUse();
    EXPECT_LE(memS, 8.5);
    EXPECT_LE(memL, 8.5);
}

// Rapid Allocation/Deallocation Pattern
TEST_F(TransTablePerformanceTest, RapidAllocationDeallocation) {
    for (int cycle = 0; cycle < 10; ++cycle) {
        auto scenarios = factory->CreateTestSuite(500);
        for (const auto& s : scenarios) {
            ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
            ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        }
        ttS->ResetMemory(TT_RESET_NEW_DEAL);
        ttL->ResetMemory(TT_RESET_NEW_DEAL);
        EXPECT_LE(ttS->MemoryInUse(), DEFAULT_MEMORY_MB);
        EXPECT_LE(ttL->MemoryInUse(), DEFAULT_MEMORY_MB);
    }
}

// Lookup Performance Test
TEST_F(TransTablePerformanceTest, LookupPerformance) {
    auto scenarios = factory->CreateTestSuite(2000);
    for (const auto& s : scenarios) {
        ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    }
    auto startS = std::chrono::high_resolution_clock::now();
    for (const auto& s : scenarios) {
        bool lowerFlag = false;
        ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
    }
    auto endS = std::chrono::high_resolution_clock::now();
    auto durationS = std::chrono::duration_cast<std::chrono::milliseconds>(endS - startS);
    auto startL = std::chrono::high_resolution_clock::now();
    for (const auto& s : scenarios) {
        bool lowerFlag = false;
        ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
    }
    auto endL = std::chrono::high_resolution_clock::now();
    auto durationL = std::chrono::duration_cast<std::chrono::milliseconds>(endL - startL);
    EXPECT_LT(durationS.count(), 2000); // Should be reasonably fast
    EXPECT_LT(durationL.count(), 2000);
}

// Cache Hit/Miss Rate Test
TEST_F(TransTablePerformanceTest, CacheHitMissRate) {
    auto scenarios = factory->CreateTestSuite(1000);
    // Add all positions
    for (const auto& s : scenarios) {
        ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
    }
    // Lookup all (should be hits)
    int hitsS = 0, hitsL = 0;
    for (const auto& s : scenarios) {
        bool lowerFlag = false;
        if (ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag)) hitsS++;
        if (ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag)) hitsL++;
    }
    EXPECT_EQ(hitsS, 1000);
    EXPECT_EQ(hitsL, 1000);
    // Lookup random (should be mostly misses)
    int missesS = 0, missesL = 0;
    auto randoms = factory->CreateTestSuite(1000);
    for (const auto& s : randoms) {
        bool lowerFlag = false;
        if (!ttS->Lookup(s.trick+1, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag)) missesS++;
        if (!ttL->Lookup(s.trick+1, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag)) missesL++;
    }
    EXPECT_GT(missesS, 800);
    EXPECT_GT(missesL, 800);
}

// Search Time Complexity Test (scaling)
TEST_F(TransTablePerformanceTest, SearchTimeComplexityScaling) {
    std::vector<int> sizes = {100, 1000, 5000};
    for (int n : sizes) {
        ttS->ResetMemory(TT_RESET_NEW_DEAL);
        ttL->ResetMemory(TT_RESET_NEW_DEAL);
        auto scenarios = factory->CreateTestSuite(n);
        for (const auto& s : scenarios) {
            ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
            ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        }
        auto startS = std::chrono::high_resolution_clock::now();
        for (const auto& s : scenarios) {
            bool lowerFlag = false;
            ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
        }
        auto endS = std::chrono::high_resolution_clock::now();
        auto durationS = std::chrono::duration_cast<std::chrono::milliseconds>(endS - startS);
        auto startL = std::chrono::high_resolution_clock::now();
        for (const auto& s : scenarios) {
            bool lowerFlag = false;
            ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag);
        }
        auto endL = std::chrono::high_resolution_clock::now();
        auto durationL = std::chrono::duration_cast<std::chrono::milliseconds>(endL - startL);
        EXPECT_LT(durationS.count(), 2000);
        EXPECT_LT(durationL.count(), 2000);
    }
}

// Thread Safety/Concurrent Access Test (Task 12)
TEST_F(TransTablePerformanceTest, DISABLED_ConcurrentAccessNotSupported) {
    // The TransTable implementations are not thread-safe.
    // Concurrent access must be guarded externally (e.g., with a mutex).
    SUCCEED() << "TransTable is not thread-safe; concurrent access is not supported.";
}

// End-to-End Integration Test (Task 13)
TEST_F(TransTablePerformanceTest, EndToEndIntegration_SearchSimulation) {
    // Simulate a search algorithm using the trans table for storage and retrieval
    auto scenarios = factory->CreateTestSuite(500);
    int found = 0, notFound = 0;
    // Simulate search: add and lookup, with resets in between
    for (int round = 0; round < 5; ++round) {
        for (const auto& s : scenarios) {
            ttS->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
            ttL->Add(s.trick, s.hand, s.aggrTarget, s.winRanks, s.nodeData, false);
        }
        for (const auto& s : scenarios) {
            bool lowerFlag = false;
            if (ttS->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag)) found++;
            else notFound++;
            if (ttL->Lookup(s.trick, s.hand, s.aggrTarget, s.handDist, 10, lowerFlag)) found++;
            else notFound++;
        }
        ttS->ResetMemory(TT_RESET_NEW_DEAL);
        ttL->ResetMemory(TT_RESET_NEW_DEAL);
    }
    EXPECT_GT(found, notFound);
}
