#ifndef DDS_MOCK_DATA_GENERATORS_H
#define DDS_MOCK_DATA_GENERATORS_H

#include <vector>
#include <random>
#include <array>
#include "trans_table/TransTable.h"
#include <api/dll.h>

namespace dds_test {

// Mock hand distribution generator
class MockHandGenerator {
public:
    MockHandGenerator(unsigned int seed = 12345);
    
    // Generate realistic hand distributions
    void GenerateRandomDistribution(int handDist[DDS_HANDS]);
    void GenerateBalancedDistribution(int handDist[DDS_HANDS]);
    void GenerateUnbalancedDistribution(int handDist[DDS_HANDS]);
    
    // Generate specific distribution patterns
    void GenerateVoidSuitDistribution(int handDist[DDS_HANDS], int suitToVoid);
    void GenerateLongSuitDistribution(int handDist[DDS_HANDS], int suitToExtend);
    
    // Generate hand lookup tables
    void GenerateStandardHandLookup(int handLookup[][15]);
    void GenerateRandomHandLookup(int handLookup[][15]);
    
    // Utilities
    bool IsValidDistribution(const int handDist[DDS_HANDS]) const;
    void PrintDistribution(const int handDist[DDS_HANDS]) const;
    
private:
    std::mt19937 generator_;
    std::uniform_int_distribution<int> cardDist_;
    std::uniform_int_distribution<int> suitDist_;
    
    void EnsureValidTotalCards(int handDist[DDS_HANDS], int targetTotal = 13);
};

// Mock position generator for different game scenarios
class MockPositionGenerator {
public:
    MockPositionGenerator(unsigned int seed = 54321);
    
    // Generate position data for different trick numbers
    void GenerateEarlyGamePosition(
        int& trick, int& hand,
        unsigned short aggrTarget[DDS_SUITS],
        int handDist[DDS_HANDS]
    );
    
    void GenerateMiddleGamePosition(
        int& trick, int& hand,
        unsigned short aggrTarget[DDS_SUITS],
        int handDist[DDS_HANDS]
    );
    
    void GenerateEndGamePosition(
        int& trick, int& hand,
        unsigned short aggrTarget[DDS_SUITS],
        int handDist[DDS_HANDS]
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
        std::vector<std::array<int, DDS_HANDS>> handDists;
    };
    
    GameSequence GenerateGameSequence(int startTrick, int endTrick);
    
private:
    std::mt19937 generator_;
    std::uniform_int_distribution<int> trickDist_;
    std::uniform_int_distribution<int> handDist_;
    std::uniform_int_distribution<unsigned short> aggrDist_;
    
    void AdjustForTrickNumber(int trick, int handDist[DDS_HANDS]);
};

// Mock winning rank pattern generator
class MockWinRankGenerator {
public:
    MockWinRankGenerator(unsigned int seed = 98765);
    
    // Generate winning rank patterns
    void GenerateSimpleWinRanks(unsigned short winRanks[DDS_SUITS]);
    void GenerateComplexWinRanks(unsigned short winRanks[DDS_SUITS]);
    void GenerateEquivalentWinRanks(
        unsigned short winRanks1[DDS_SUITS],
        unsigned short winRanks2[DDS_SUITS]
    );
    
    // Generate relative rank scenarios
    void GenerateRelativeRankScenario(
        unsigned short absoluteRanks[DDS_SUITS],
        unsigned short relativeRanks[DDS_SUITS],
        unsigned short winMask[DDS_SUITS]
    );
    
    // Generate patterns for specific test scenarios
    void GenerateSingleSuitWin(unsigned short winRanks[DDS_SUITS], int suit);
    void GenerateMultiSuitWin(unsigned short winRanks[DDS_SUITS]);
    void GenerateNoWinRanks(unsigned short winRanks[DDS_SUITS]);
    
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
    
    void PrintWinRanks(const unsigned short winRanks[DDS_SUITS]) const;
    
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
        int handDist[DDS_HANDS];
        unsigned short winRanks[DDS_SUITS];
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

#endif // DDS_MOCK_DATA_GENERATORS_H
