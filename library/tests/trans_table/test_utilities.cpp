#include "test_utilities.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include "utility/Constants.h"

namespace dds_test {

// TransTableTestBase implementation
void TransTableTestBase::SetUp() {
    CreateStandardHandLookup(standardHandLookup);
    setupComplete = true;
}

void TransTableTestBase::TearDown() {
    setupComplete = false;
}

void TransTableTestBase::CreateStandardHandLookup(int handLookup[][15]) {
    // Initialize with standard hand configuration
    // This creates a mapping from relative ranks to hand positions
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        for (int rank = 0; rank < 15; rank++) {
            handLookup[hand][rank] = hand; // Simple mapping for testing
        }
    }
}

void TransTableTestBase::CreateTestPositionData(
    int trick, 
    int hand, 
    unsigned short aggrTarget[DDS_SUITS],
    int handDist[DDS_HANDS],
    unsigned short winRanks[DDS_SUITS],
    nodeCardsType& nodeData) {
    
    // Create simple test position data
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        aggrTarget[suit] = static_cast<unsigned short>(0x1000 + suit * 0x100);
        winRanks[suit] = static_cast<unsigned short>(0x2000 + suit * 0x200);
    }
    
    for (int h = 0; h < DDS_HANDS; h++) {
        handDist[h] = 13 - trick; // Decreasing cards as tricks progress
    }
    
    // Initialize node data
    nodeData.ubound = static_cast<char>(13 - trick);
    nodeData.lbound = static_cast<char>(trick > 0 ? trick - 1 : 0);
    nodeData.bestMoveSuit = static_cast<char>(0);
    nodeData.bestMoveRank = static_cast<char>(14); // Ace
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        nodeData.leastWin[suit] = static_cast<char>(2 + suit); // 2, 3, 4, 5
    }
}

double TransTableTestBase::GetInitialMemoryUsage(TransTable* table) {
    return table ? table->MemoryInUse() : 0.0;
}

// MemoryTracker implementation
MemoryTracker::MemoryTracker(TransTable* table) 
    : table_(table), initialUsage_(0.0), peakUsage_(0.0) {
    if (table_) {
        initialUsage_ = table_->MemoryInUse();
        peakUsage_ = initialUsage_;
    }
}

MemoryTracker::~MemoryTracker() {
    // Destructor can log final memory state if needed
}

double MemoryTracker::GetCurrentUsage() const {
    return table_ ? table_->MemoryInUse() : 0.0;
}

void MemoryTracker::UpdatePeak() {
    if (table_) {
        double current = table_->MemoryInUse();
        if (current > peakUsage_) {
            peakUsage_ = current;
        }
    }
}

bool MemoryTracker::HasMemoryLeak() const {
    // Simple leak detection - current usage should not exceed peak by much
    double current = GetCurrentUsage();
    return current > (peakUsage_ * 1.1); // 10% tolerance
}

// PerformanceTimer implementation
PerformanceTimer::PerformanceTimer() : isRunning_(false) {
    Reset();
}

void PerformanceTimer::Start() {
    startTime_ = std::chrono::high_resolution_clock::now();
    isRunning_ = true;
}

void PerformanceTimer::Stop() {
    if (isRunning_) {
        endTime_ = std::chrono::high_resolution_clock::now();
        isRunning_ = false;
    }
}

void PerformanceTimer::Reset() {
    startTime_ = std::chrono::high_resolution_clock::time_point();
    endTime_ = std::chrono::high_resolution_clock::time_point();
    isRunning_ = false;
    operations_.clear();
}

std::chrono::milliseconds PerformanceTimer::GetElapsed() const {
    if (isRunning_) {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_);
    } else {
        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime_ - startTime_);
    }
}

double PerformanceTimer::GetElapsedSeconds() const {
    auto ms = GetElapsed();
    return ms.count() / 1000.0;
}

void PerformanceTimer::StartOperation(const std::string& name) {
    operations_[name].count++;
    // Store start time for this operation
    startTime_ = std::chrono::high_resolution_clock::now();
    isRunning_ = true;
}

void PerformanceTimer::EndOperation(const std::string& name) {
    if (isRunning_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - startTime_);
        
        auto& stats = operations_[name];
        stats.totalTime += elapsed;
        stats.minTime = std::min(stats.minTime, elapsed);
        stats.maxTime = std::max(stats.maxTime, elapsed);
        
        isRunning_ = false;
    }
}

void PerformanceTimer::PrintResults() const {
    std::cout << "Performance Results:\n";
    std::cout << std::setw(20) << "Operation" 
              << std::setw(10) << "Count"
              << std::setw(12) << "Total(ms)"
              << std::setw(12) << "Avg(ms)"
              << std::setw(10) << "Min(ms)"
              << std::setw(10) << "Max(ms)" << "\n";
    std::cout << std::string(74, '-') << "\n";
    
    for (const auto& op : operations_) {
        const auto& stats = op.second;
        double avgTime = stats.count > 0 ? 
            static_cast<double>(stats.totalTime.count()) / stats.count : 0.0;
        
        std::cout << std::setw(20) << op.first
                  << std::setw(10) << stats.count
                  << std::setw(12) << stats.totalTime.count()
                  << std::setw(12) << std::fixed << std::setprecision(2) << avgTime
                  << std::setw(10) << stats.minTime.count()
                  << std::setw(10) << stats.maxTime.count() << "\n";
    }
}

// PositionComparator implementation
bool PositionComparator::AreEqual(const nodeCardsType& a, const nodeCardsType& b) {
    if (a.ubound != b.ubound || a.lbound != b.lbound) {
        return false;
    }
    
    if (a.bestMoveSuit != b.bestMoveSuit || a.bestMoveRank != b.bestMoveRank) {
        return false;
    }
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (a.leastWin[suit] != b.leastWin[suit]) {
            return false;
        }
    }
    
    return true;
}

bool PositionComparator::BoundsAreEquivalent(
    const nodeCardsType& a, 
    const nodeCardsType& b,
    int tolerance) {
    
    return (abs(a.ubound - b.ubound) <= tolerance) &&
           (abs(a.lbound - b.lbound) <= tolerance);
}

bool PositionComparator::RelativeRanksMatch(
    const unsigned short ranks1[DDS_SUITS],
    const unsigned short ranks2[DDS_SUITS],
    const unsigned short winMask[DDS_SUITS]) {
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        // Apply mask and compare
        unsigned short masked1 = ranks1[suit] & winMask[suit];
        unsigned short masked2 = ranks2[suit] & winMask[suit];
        
        if (masked1 != masked2) {
            return false;
        }
    }
    
    return true;
}

std::string PositionComparator::PositionToString(const nodeCardsType& node) {
    std::ostringstream oss;
    oss << "NodeData{ubound:" << static_cast<int>(node.ubound)
        << ", lbound:" << static_cast<int>(node.lbound)
        << ", bestMove:" << static_cast<int>(node.bestMoveSuit) 
        << "/" << static_cast<int>(node.bestMoveRank)
        << ", leastWin:[";
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (suit > 0) oss << ",";
        oss << static_cast<int>(node.leastWin[suit]);
    }
    oss << "]}";
    
    return oss.str();
}

std::string PositionComparator::RanksToString(const unsigned short ranks[DDS_SUITS]) {
    std::ostringstream oss;
    oss << "Ranks[";
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (suit > 0) oss << ",";
        oss << "0x" << std::hex << ranks[suit];
    }
    oss << "]";
    return oss.str();
}

// TestDataValidator implementation
bool TestDataValidator::IsValidHandDistribution(const int handDist[DDS_HANDS]) {
    int total = 0;
    for (int hand = 0; hand < DDS_HANDS; hand++) {
        if (handDist[hand] < 0 || handDist[hand] > 13) {
            return false;
        }
        total += handDist[hand];
    }
    
    // Total should be reasonable for remaining cards
    return total >= 0 && total <= 52;
}

bool TestDataValidator::IsValidAggrTarget(const unsigned short aggrTarget[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        // Check if the aggregate value is within reasonable bounds
        if (aggrTarget[suit] > 0x1FFF) { // 13 bits max for card combinations
            return false;
        }
    }
    return true;
}

bool TestDataValidator::IsValidWinRanks(const unsigned short winRanks[DDS_SUITS]) {
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        // Check if winning ranks are within valid range
        if (winRanks[suit] > 0x1FFF) { // 13 bits max
            return false;
        }
    }
    return true;
}

bool TestDataValidator::IsValidNodeData(const nodeCardsType& node) {
    // Check bounds are reasonable
    if (node.ubound < 0 || node.ubound > 13 ||
        node.lbound < 0 || node.lbound > 13 ||
        node.lbound > node.ubound) {
        return false;
    }
    
    // Check suit/rank are in valid range
    if (node.bestMoveSuit < 0 || node.bestMoveSuit >= DDS_SUITS ||
        node.bestMoveRank < 2 || node.bestMoveRank > 14) {
        return false;
    }
    
    // Check leastWin values
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (node.leastWin[suit] < 0 || node.leastWin[suit] > 14) {
            return false;
        }
    }
    
    return true;
}

bool TestDataValidator::IsValidTrickHand(int trick, int hand) {
    return (trick >= 1 && trick <= 13) && 
           (hand >= 0 && hand < DDS_HANDS);
}

} // namespace dds_test
