/// @file mock_data_generators.hpp
/// @brief Mock data generators for trans_table testing.
/// @details Provides generators for hands, positions, and game sequences.

#pragma once

// C++ standard library headers
#include <array>
#include <random>
#include <vector>

// Project headers
#include <api/dll.h>
#include "trans_table/trans_table.hpp"

namespace dds_test {

// Mock hand distribution generator
class MockHandGenerator {
public:
    MockHandGenerator(unsigned int seed = 12345);
    
    // Generate realistic hand distributions
    void GenerateRandomDistribution(int hand_dist[DDS_HANDS]);
    void GenerateBalancedDistribution(int hand_dist[DDS_HANDS]);
    void GenerateUnbalancedDistribution(int hand_dist[DDS_HANDS]);
    
    // Generate specific distribution patterns
    void GenerateVoidSuitDistribution(int hand_dist[DDS_HANDS], int suitToVoid);
    void GenerateLongSuitDistribution(int hand_dist[DDS_HANDS], int suitToExtend);
    
    // Generate hand lookup tables
    void GenerateStandardHandLookup(int handLookup[][15]);
    void GenerateRandomHandLookup(int handLookup[][15]);
    
    // Utilities
    bool IsValidDistribution(const int hand_dist[DDS_HANDS]) const;
    void PrintDistribution(const int hand_dist[DDS_HANDS]) const;
    
private:
    std::mt19937 generator_;
    std::uniform_int_distribution<int> cardDist_;
    std::uniform_int_distribution<int> suitDist_;
    
    void EnsureValidTotalCards(int hand_dist[DDS_HANDS], int targetTotal = 13);
};

// Mock position generator for different game scenarios
class MockPositionGenerator {
public:
    MockPositionGenerator(unsigned int seed = 54321);
    
    // Generate position data for different trick numbers
    void GenerateEarlyGamePosition(
        int& trick, int& hand,
        unsigned short aggrTarget[DDS_SUITS],
        int hand_dist[DDS_HANDS]
    );
    
    void GenerateMiddleGamePosition(
        int& trick, int& hand,
        unsigned short aggrTarget[DDS_SUITS],
        int hand_dist[DDS_HANDS]
    );
    
    void GenerateEndGamePosition(
        int& trick, int& hand,
        unsigned short aggrTarget[DDS_SUITS],
        int hand_dist[DDS_HANDS]
    );
    
    // Generate aggregate target data
    void GenerateAggrTarget(unsigned short aggrTarget[DDS_SUITS], int complexity = 1);
    void GenerateSimpleAggrTarget(unsigned short aggrTarget[DDS_SUITS]);
    void GenerateComplexAggrTarget(unsigned short aggrTarget[DDS_SUITS]);
    
    // Generate node card data
    void GenerateNodeCardsType(NodeCards& node, int tricksRemaining);
    
    // Generate realistic game sequences
    struct GameSequence {
        std::vector<int> tricks;
        std::vector<int> hands;
        std::vector<std::array<unsigned short, DDS_SUITS>> aggrTargets;
        std::vector<std::array<int, DDS_HANDS>> hand_dists;
    };
    
    GameSequence GenerateGameSequence(int startTrick, int endTrick);
    
private:
    std::mt19937 generator_;
    std::uniform_int_distribution<int> trickDist_;
    std::uniform_int_distribution<int> hand_dist_;
    std::uniform_int_distribution<unsigned short> aggrDist_;
    
    void AdjustForTrickNumber(int trick, int hand_dist[DDS_HANDS]);
};

// Mock winning rank pattern generator
class MockWinRankGenerator {
public:
    MockWinRankGenerator(unsigned int seed = 98765);
    
    // Generate winning rank patterns
    void GenerateSimpleWinRanks(unsigned short win_ranks[DDS_SUITS]);
    void GenerateComplexWinRanks(unsigned short win_ranks[DDS_SUITS]);
    void GenerateEquivalentWinRanks(
        unsigned short win_ranks1[DDS_SUITS],
        unsigned short win_ranks2[DDS_SUITS]
    );
    
    // Generate relative rank scenarios
    void GenerateRelativeRankScenario(
        unsigned short absoluteRanks[DDS_SUITS],
        unsigned short relativeRanks[DDS_SUITS],
        unsigned short winMask[DDS_SUITS]
    );
    
    // Generate patterns for specific test scenarios
    void GenerateSingleSuitWin(unsigned short win_ranks[DDS_SUITS], int suit);
    void GenerateMultiSuitWin(unsigned short win_ranks[DDS_SUITS]);
    void GenerateNoWinRanks(unsigned short win_ranks[DDS_SUITS]);
    
    // Generate suit-specific patterns
    void GenerateHighCardPattern(unsigned short& suitRanks);
    void GenerateLowCardPattern(unsigned short& suitRanks);
    void GenerateSequencePattern(unsigned short& suitRanks);
    void GenerateGappedPattern(unsigned short& suitRanks);
    
    // Utilities for rank manipulation
    static bool AreEquivalentRelativeRanks(
        const unsigned short ranks1[DDS_SUITS],
        const unsigned short ranks2[DDS_SUITS]
    );
    
    static void ConvertToRelativeRanks(
        const unsigned short absolute[DDS_SUITS],
        unsigned short relative[DDS_SUITS]
    );
    
    void PrintWinRanks(const unsigned short win_ranks[DDS_SUITS]) const;
    
private:
    std::mt19937 generator_;
    std::uniform_int_distribution<unsigned short> rankDist_;
    std::bernoulli_distribution coinFlip_;
    
    // Helper to ensure valid rank patterns
    void EnsureValidRankPattern(unsigned short& ranks);
    unsigned short CreateRankSequence(int start, int length);
};

// Combined mock data factory
class MockDataFactory {
public:
    MockDataFactory(unsigned int baseSeed = 11111);
    
    // Create complete test scenarios
    struct TestScenario {
        int trick;
        int hand;
        unsigned short aggrTarget[DDS_SUITS];
        int hand_dist[DDS_HANDS];
        unsigned short win_ranks[DDS_SUITS];
    NodeCards nodeData;
        int handLookup[DDS_HANDS][15];
    };
    
    TestScenario CreateBasicScenario();
    TestScenario CreateComplexScenario();
    TestScenario CreateEdgeCaseScenario();
    TestScenario CreatePerformanceScenario();
    
    // Create matching scenarios for equivalence testing
    std::pair<TestScenario, TestScenario> CreateEquivalentScenarios();
    std::pair<TestScenario, TestScenario> CreateNonEquivalentScenarios();
    
    // Create test data sets
    std::vector<TestScenario> CreateTestSuite(int count);
    std::vector<TestScenario> CreateRegressionTestSuite();
    
private:
    MockHandGenerator handGen_;
    MockPositionGenerator posGen_;
    MockWinRankGenerator rankGen_;
    unsigned int currentSeed_;
    
    void IncrementSeed() { currentSeed_++; }
};

} // namespace dds_test
