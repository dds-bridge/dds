#include "test_utilities.hpp"
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
    NodeCards& nodeData) {
    
    // Create simple test position data
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        aggrTarget[suit] = static_cast<unsigned short>(0x1000 + suit * 0x100);
        winRanks[suit] = static_cast<unsigned short>(0x2000 + suit * 0x200);
    }
    
    for (int h = 0; h < DDS_HANDS; h++) {
        handDist[h] = 13 - trick; // Decreasing cards as tricks progress
    }
    
    // Initialize node data
    nodeData.upper_bound = static_cast<char>(13 - trick);
    nodeData.lower_bound = static_cast<char>(trick > 0 ? trick - 1 : 0);
    nodeData.best_move_suit = static_cast<char>(0);
    nodeData.best_move_rank = static_cast<char>(14); // Ace
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        nodeData.least_win[suit] = static_cast<char>(2 + suit); // 2, 3, 4, 5
    }
}

double TransTableTestBase::GetInitialMemoryUsage(TransTable* table) {
    return table ? table->memory_in_use() : 0.0;
}

// MemoryTracker implementation
MemoryTracker::MemoryTracker(TransTable* table) 
    : table_(table), initialUsage_(0.0), peakUsage_(0.0) {
    if (table_) {
        initialUsage_ = table_->memory_in_use();
        peakUsage_ = initialUsage_;
    }
}

MemoryTracker::~MemoryTracker() {
    // Destructor can log final memory state if needed
}

double MemoryTracker::GetCurrentUsage() const {
    return table_ ? table_->memory_in_use() : 0.0;
}

void MemoryTracker::UpdatePeak() {
    if (table_) {
        double current = table_->memory_in_use();
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
bool PositionComparator::AreEqual(const NodeCards& a, const NodeCards& b) {
    if (a.upper_bound != b.upper_bound || a.lower_bound != b.lower_bound) {
        return false;
    }
    
    if (a.best_move_suit != b.best_move_suit || a.best_move_rank != b.best_move_rank) {
        return false;
    }
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (a.least_win[suit] != b.least_win[suit]) {
            return false;
        }
    }
    
    return true;
}

bool PositionComparator::BoundsAreEquivalent(
    const NodeCards& a, 
    const NodeCards& b,
    int tolerance) {
    
    return (abs(a.upper_bound - b.upper_bound) <= tolerance) &&
           (abs(a.lower_bound - b.lower_bound) <= tolerance);
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

std::string PositionComparator::PositionToString(const NodeCards& node) {
    std::ostringstream oss;
    oss << "NodeData{ubound:" << static_cast<int>(node.upper_bound)
        << ", lbound:" << static_cast<int>(node.lower_bound)
        << ", bestMove:" << static_cast<int>(node.best_move_suit) 
        << "/" << static_cast<int>(node.best_move_rank)
        << ", least_win:[";
    
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (suit > 0) oss << ",";
        oss << static_cast<int>(node.least_win[suit]);
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

bool TestDataValidator::IsValidNodeData(const NodeCards& node) {
    // Check bounds are reasonable
    if (node.upper_bound < 0 || node.upper_bound > 13 ||
        node.lower_bound < 0 || node.lower_bound > 13 ||
        node.lower_bound > node.upper_bound) {
        return false;
    }
    
    // Check suit/rank are in valid range
    if (node.best_move_suit < 0 || node.best_move_suit >= DDS_SUITS ||
        node.best_move_rank < 2 || node.best_move_rank > 14) {
        return false;
    }
    
    // Check least_win values
    for (int suit = 0; suit < DDS_SUITS; suit++) {
        if (node.least_win[suit] < 0 || node.least_win[suit] > 14) {
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
