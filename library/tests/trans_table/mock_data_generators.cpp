#include "mock_data_generators.h"
#include <iostream>
#include <algorithm>
#include "utility/Constants.h"

namespace dds_test {

// MockHandGenerator implementation
MockHandGenerator::MockHandGenerator(unsigned int seed) 
    : generator_(seed), cardDist_(0, 12), suitDist_(0, 3) {
}

void MockHandGenerator::GenerateRandomDistribution(int handDist[DDS_HANDS]) {
    // Generate random but valid distribution
    int remaining = 52;
    for (int hand = 0; hand < DDS_HANDS - 1; hand++) {
        int maxCards = std::min(13, remaining - (DDS_HANDS - hand - 1));
        std::uniform_int_distribution<int> dist(0, maxCards);
        handDist[hand] = dist(generator_);
        remaining -= handDist[hand];
    }
    handDist[DDS_HANDS - 1] = remaining; // Last hand gets remainder
    
    EnsureValidTotalCards(handDist);
}

void MockHandGenerator::GenerateBalancedDistribution(int handDist[DDS_HANDS]) {
    // Each hand gets approximately the same number of cards
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        handDist[hand] = 13; // Standard distribution
    }
}

void MockHandGenerator::GenerateUnbalancedDistribution(int handDist[DDS_HANDS]) {
    // Create an unbalanced distribution
    handDist[0] = 10;
    handDist[1] = 15;
    handDist[2] = 12;
    handDist[3] = 15;
    
    EnsureValidTotalCards(handDist);
}

void MockHandGenerator::GenerateVoidSuitDistribution(int handDist[DDS_HANDS], int suitToVoid) {
    GenerateBalancedDistribution(handDist);
    // This is hand distribution, not suit distribution
    // For now, just create a standard distribution
}

void MockHandGenerator::GenerateLongSuitDistribution(int handDist[DDS_HANDS], int suitToExtend) {
    GenerateBalancedDistribution(handDist);
    // This is hand distribution, not suit distribution
    // For now, just create a standard distribution
}

void MockHandGenerator::GenerateStandardHandLookup(int handLookup[][15]) {
    // Create standard hand lookup table
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        for (int rank = 0; rank < 15; rank++) {
            handLookup[hand][rank] = hand;
        }
    }
}

void MockHandGenerator::GenerateRandomHandLookup(int handLookup[][15]) {
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        for (int rank = 0; rank < 15; rank++) {
            std::uniform_int_distribution<int> dist(0, DDS_HANDS - 1);
            handLookup[hand][rank] = dist(generator_);
        }
    }
}

bool MockHandGenerator::IsValidDistribution(const int handDist[DDS_HANDS]) const {
    int total = 0;
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        if (handDist[hand] < 0 || handDist[hand] > 13) {
            return false;
        }
        total += handDist[hand];
    }
    return total <= 52;
}

void MockHandGenerator::PrintDistribution(const int handDist[DDS_HANDS]) const {
    std::cout << "Hand distribution: [";
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        if (hand > 0) std::cout << ", ";
        std::cout << handDist[hand];
    }
    std::cout << "]\n";
}

void MockHandGenerator::EnsureValidTotalCards(int handDist[DDS_HANDS], int targetTotal) {
    int total = 0;
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        total += handDist[hand];
    }
    
    // Adjust if total doesn't match target
    if (total != targetTotal) {
        int diff = targetTotal - total;
        // Distribute difference across hands
        for (int hand = 0; hand < DDS_HANDS && diff != 0; hand++) {
            if (diff > 0 && handDist[hand] < 13) {
                int add = std::min(diff, 13 - handDist[hand]);
                handDist[hand] += add;
                diff -= add;
            } else if (diff < 0 && handDist[hand] > 0) {
                int sub = std::min(-diff, handDist[hand]);
                handDist[hand] -= sub;
                diff += sub;
            }
        }
    }
}

// MockPositionGenerator implementation
MockPositionGenerator::MockPositionGenerator(unsigned int seed)
    : generator_(seed), trickDist_(1, 13), handDist_(0, DDS_HANDS - 1), aggrDist_(0, 0x1FFF) {
}

void MockPositionGenerator::GenerateEarlyGamePosition(
    int& trick, int& hand,
    unsigned short aggrTarget[DDS_SUITS],
    int handDist[DDS_HANDS]) {
    
    trick = std::uniform_int_distribution<int>(1, 4)(generator_);
    hand = handDist_(generator_);
    
    GenerateSimpleAggrTarget(aggrTarget);
    
    for (int h = 0; h < DDS_HANDS; h++) {
        handDist[h] = 13 - trick + 1;
    }
}

void MockPositionGenerator::GenerateMiddleGamePosition(
    int& trick, int& hand,
    unsigned short aggrTarget[DDS_SUITS],
    int handDist[DDS_HANDS]) {
    
    trick = std::uniform_int_distribution<int>(5, 9)(generator_);
    hand = handDist_(generator_);
    
    GenerateAggrTarget(aggrTarget, 2);
    
    for (int h = 0; h < DDS_HANDS; h++) {
        handDist[h] = 13 - trick + 1;
    }
}

void MockPositionGenerator::GenerateEndGamePosition(
    int& trick, int& hand,
    unsigned short aggrTarget[DDS_SUITS],
    int handDist[DDS_HANDS]) {
    
    trick = std::uniform_int_distribution<int>(10, 13)(generator_);
    hand = handDist_(generator_);
    
    GenerateComplexAggrTarget(aggrTarget);
    
    for (int h = 0; h < DDS_HANDS; h++) {
        handDist[h] = 13 - trick + 1;
    }
}

void MockPositionGenerator::GenerateAggrTarget(unsigned short aggrTarget[DDS_SUITS], int complexity) {
    if (complexity == 1) {
        GenerateSimpleAggrTarget(aggrTarget);
    } else {
        GenerateComplexAggrTarget(aggrTarget);
    }
}

void MockPositionGenerator::GenerateSimpleAggrTarget(unsigned short aggrTarget[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        aggrTarget[suit] = static_cast<unsigned short>(0x1000 + suit * 0x100);
    }
}

void MockPositionGenerator::GenerateComplexAggrTarget(unsigned short aggrTarget[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        aggrTarget[suit] = static_cast<unsigned short>(aggrDist_(generator_));
    }
}

void MockPositionGenerator::GenerateNodeCardsType(NodeCards& node, int tricksRemaining) {
    node.ubound = static_cast<char>(tricksRemaining);
    node.lbound = static_cast<char>(std::max(0, tricksRemaining - 3));
    node.bestMoveSuit = static_cast<char>(std::uniform_int_distribution<int>(0, DDS_SUITS - 1)(generator_));
    node.bestMoveRank = static_cast<char>(std::uniform_int_distribution<int>(2, 14)(generator_));
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        node.leastWin[suit] = static_cast<char>(std::uniform_int_distribution<int>(2, 14)(generator_));
    }
}

MockPositionGenerator::GameSequence MockPositionGenerator::GenerateGameSequence(int startTrick, int endTrick) {
    GameSequence sequence;
    
    for (int trick = startTrick; trick <= endTrick; trick++) {
        sequence.tricks.push_back(trick);
        sequence.hands.push_back(handDist_(generator_));
        
        std::array<unsigned short, DDS_SUITS> aggrTarget;
        GenerateAggrTarget(aggrTarget.data(), 1);
        sequence.aggrTargets.push_back(aggrTarget);
        
        std::array<int, DDS_HANDS> handDist;
        for (int hand = 0; hand < DDS_HANDS; hand++) {
            handDist[hand] = 13 - trick + 1;
        }
        sequence.handDists.push_back(handDist);
    }
    
    return sequence;
}

void MockPositionGenerator::AdjustForTrickNumber(int trick, int handDist[DDS_HANDS]) {
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        handDist[hand] = std::max(0, 13 - trick);
    }
}

// MockWinRankGenerator implementation
MockWinRankGenerator::MockWinRankGenerator(unsigned int seed)
    : generator_(seed), rankDist_(0, 0x1FFF), coinFlip_(0.5) {
}

void MockWinRankGenerator::GenerateSimpleWinRanks(unsigned short winRanks[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        winRanks[suit] = static_cast<unsigned short>(0x1000 + suit * 0x100);
    }
}

void MockWinRankGenerator::GenerateComplexWinRanks(unsigned short winRanks[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        winRanks[suit] = rankDist_(generator_);
        EnsureValidRankPattern(winRanks[suit]);
    }
}

void MockWinRankGenerator::GenerateEquivalentWinRanks(
    unsigned short winRanks1[DDS_SUITS],
    unsigned short winRanks2[DDS_SUITS]) {
    
    // Generate first set
    GenerateSimpleWinRanks(winRanks1);
    
    // Make second set equivalent in relative terms
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        winRanks2[suit] = winRanks1[suit];
    }
}

void MockWinRankGenerator::GenerateRelativeRankScenario(
    unsigned short absoluteRanks[DDS_SUITS],
    unsigned short relativeRanks[DDS_SUITS],
    unsigned short winMask[DDS_SUITS]) {
    
    GenerateSimpleWinRanks(absoluteRanks);
    ConvertToRelativeRanks(absoluteRanks, relativeRanks);
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        winMask[suit] = 0x1FFF; // Full mask for simplicity
    }
}

void MockWinRankGenerator::GenerateSingleSuitWin(unsigned short winRanks[DDS_SUITS], int suit) {
    for (int s = 0; s < DDS_SUITS; s++) {
        winRanks[s] = (s == suit) ? static_cast<unsigned short>(0x1000) : 0;
    }
}

void MockWinRankGenerator::GenerateMultiSuitWin(unsigned short winRanks[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        winRanks[suit] = coinFlip_(generator_) ? static_cast<unsigned short>(0x1000 + suit * 0x200) : 0;
    }
}

void MockWinRankGenerator::GenerateNoWinRanks(unsigned short winRanks[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        winRanks[suit] = 0;
    }
}

void MockWinRankGenerator::GenerateHighCardPattern(unsigned short& suitRanks) {
    suitRanks = 0x1C00; // A, K, Q
}

void MockWinRankGenerator::GenerateLowCardPattern(unsigned short& suitRanks) {
    suitRanks = 0x000E; // 2, 3, 4
}

void MockWinRankGenerator::GenerateSequencePattern(unsigned short& suitRanks) {
    suitRanks = 0x01F0; // 5, 6, 7, 8, 9
}

void MockWinRankGenerator::GenerateGappedPattern(unsigned short& suitRanks) {
    suitRanks = 0x1401; // A, 7, 2
}

bool MockWinRankGenerator::AreEquivalentRelativeRanks(
    const unsigned short ranks1[DDS_SUITS],
    const unsigned short ranks2[DDS_SUITS]) {
    
    // Simple equivalence check for now
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (ranks1[suit] != ranks2[suit]) {
            return false;
        }
    }
    return true;
}

void MockWinRankGenerator::ConvertToRelativeRanks(
    const unsigned short absolute[DDS_SUITS],
    unsigned short relative[DDS_SUITS]) {
    
    // Simple conversion - for testing purposes
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        relative[suit] = absolute[suit];
    }
}

void MockWinRankGenerator::PrintWinRanks(const unsigned short winRanks[DDS_SUITS]) const {
    std::cout << "Win ranks: [";
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (suit > 0) std::cout << ", ";
        std::cout << "0x" << std::hex << winRanks[suit];
    }
    std::cout << std::dec << "]\n";
}

void MockWinRankGenerator::EnsureValidRankPattern(unsigned short& ranks) {
    ranks &= 0x1FFF; // Ensure only 13 bits are set
}

unsigned short MockWinRankGenerator::CreateRankSequence(int start, int length) {
    unsigned short result = 0;
    for (int i = 0; i < length && (start + i) <= 14; i++) {
        result |= (1 << (start + i - 2)); // Bit positions for ranks 2-14
    }
    return result;
}

// MockDataFactory implementation
MockDataFactory::MockDataFactory(unsigned int baseSeed)
    : handGen_(baseSeed), posGen_(baseSeed + 1), rankGen_(baseSeed + 2), currentSeed_(baseSeed) {
}

MockDataFactory::TestScenario MockDataFactory::CreateBasicScenario() {
    TestScenario scenario;
    
    posGen_.GenerateEarlyGamePosition(
        scenario.trick, scenario.hand, 
        scenario.aggrTarget, scenario.handDist);
    
    rankGen_.GenerateSimpleWinRanks(scenario.winRanks);
    posGen_.GenerateNodeCardsType(scenario.nodeData, 13 - scenario.trick);
    handGen_.GenerateStandardHandLookup(scenario.handLookup);
    
    IncrementSeed();
    return scenario;
}

MockDataFactory::TestScenario MockDataFactory::CreateComplexScenario() {
    TestScenario scenario;
    
    posGen_.GenerateMiddleGamePosition(
        scenario.trick, scenario.hand,
        scenario.aggrTarget, scenario.handDist);
    
    rankGen_.GenerateComplexWinRanks(scenario.winRanks);
    posGen_.GenerateNodeCardsType(scenario.nodeData, 13 - scenario.trick);
    handGen_.GenerateRandomHandLookup(scenario.handLookup);
    
    IncrementSeed();
    return scenario;
}

MockDataFactory::TestScenario MockDataFactory::CreateEdgeCaseScenario() {
    TestScenario scenario;
    
    posGen_.GenerateEndGamePosition(
        scenario.trick, scenario.hand,
        scenario.aggrTarget, scenario.handDist);
    
    rankGen_.GenerateNoWinRanks(scenario.winRanks);
    posGen_.GenerateNodeCardsType(scenario.nodeData, 13 - scenario.trick);
    handGen_.GenerateStandardHandLookup(scenario.handLookup);
    
    IncrementSeed();
    return scenario;
}

MockDataFactory::TestScenario MockDataFactory::CreatePerformanceScenario() {
    TestScenario scenario;
    
    posGen_.GenerateMiddleGamePosition(
        scenario.trick, scenario.hand,
        scenario.aggrTarget, scenario.handDist);
    
    rankGen_.GenerateMultiSuitWin(scenario.winRanks);
    posGen_.GenerateNodeCardsType(scenario.nodeData, 13 - scenario.trick);
    handGen_.GenerateStandardHandLookup(scenario.handLookup);
    
    IncrementSeed();
    return scenario;
}

std::pair<MockDataFactory::TestScenario, MockDataFactory::TestScenario> 
MockDataFactory::CreateEquivalentScenarios() {
    TestScenario scenario1 = CreateBasicScenario();
    TestScenario scenario2 = scenario1; // Copy for equivalence
    
    // Make them equivalent in relative rank terms
    rankGen_.GenerateEquivalentWinRanks(scenario1.winRanks, scenario2.winRanks);
    
    return std::make_pair(scenario1, scenario2);
}

std::pair<MockDataFactory::TestScenario, MockDataFactory::TestScenario>
MockDataFactory::CreateNonEquivalentScenarios() {
    TestScenario scenario1 = CreateBasicScenario();
    TestScenario scenario2 = CreateComplexScenario();
    
    return std::make_pair(scenario1, scenario2);
}

std::vector<MockDataFactory::TestScenario> MockDataFactory::CreateTestSuite(int count) {
    std::vector<TestScenario> scenarios;
    scenarios.reserve(count);
    
    for (int i = 0; i < count; i++) {
        if (i % 4 == 0) {
            scenarios.push_back(CreateBasicScenario());
        } else if (i % 4 == 1) {
            scenarios.push_back(CreateComplexScenario());
        } else if (i % 4 == 2) {
            scenarios.push_back(CreateEdgeCaseScenario());
        } else {
            scenarios.push_back(CreatePerformanceScenario());
        }
    }
    
    return scenarios;
}

std::vector<MockDataFactory::TestScenario> MockDataFactory::CreateRegressionTestSuite() {
    std::vector<TestScenario> scenarios;
    
    // Add specific scenarios for regression testing
    scenarios.push_back(CreateBasicScenario());
    scenarios.push_back(CreateComplexScenario());
    scenarios.push_back(CreateEdgeCaseScenario());
    
    return scenarios;
}

} // namespace dds_test
