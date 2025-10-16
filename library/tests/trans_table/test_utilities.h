#ifndef DDS_TEST_UTILITIES_H
#define DDS_TEST_UTILITIES_H

#include <gtest/gtest.h>
#include <memory>
#include <chrono>
#include <string>
#include <array>
#include <map>
#include "trans_table/TransTable.h"
#include "dds/dll.h"

namespace dds_test {

// Base test fixture for all trans table tests
class TransTableTestBase {
protected:
    virtual void SetUp();
    virtual void TearDown();
    
    // Helper to create standard hand lookup table
    void CreateStandardHandLookup(int handLookup[][15]);
    
    // Helper to create test position data
    void CreateTestPositionData(
        int trick, 
        int hand, 
        unsigned short aggrTarget[DDS_SUITS],
        int handDist[DDS_HANDS],
        unsigned short winRanks[DDS_SUITS],
    NodeCards& nodeData
    );
    
    // Helper to verify memory usage patterns
    double GetInitialMemoryUsage(TransTable* table);
    
    // Standard test data
    int standardHandLookup[DDS_HANDS][15];
    bool setupComplete;
};

// Memory tracking utilities
class MemoryTracker {
public:
    MemoryTracker(TransTable* table);
    ~MemoryTracker();
    
    double GetInitialUsage() const { return initialUsage_; }
    double GetCurrentUsage() const;
    double GetPeakUsage() const { return peakUsage_; }
    void UpdatePeak();
    
    // Check for memory leaks
    bool HasMemoryLeak() const;
    
private:
    TransTable* table_;
    double initialUsage_;
    double peakUsage_;
};

// Performance timing helpers
class PerformanceTimer {
public:
    PerformanceTimer();
    
    void Start();
    void Stop();
    void Reset();
    
    std::chrono::milliseconds GetElapsed() const;
    double GetElapsedSeconds() const;
    
    // For benchmark operations
    void StartOperation(const std::string& name);
    void EndOperation(const std::string& name);
    void PrintResults() const;
    
private:
    std::chrono::high_resolution_clock::time_point startTime_;
    std::chrono::high_resolution_clock::time_point endTime_;
    bool isRunning_;
    
    struct OperationStats {
        std::chrono::milliseconds totalTime{0};
        int count = 0;
        std::chrono::milliseconds minTime{std::chrono::milliseconds::max()};
        std::chrono::milliseconds maxTime{0};
    };
    
    std::map<std::string, OperationStats> operations_;
};

// Position comparison utilities
class PositionComparator {
public:
    // Compare two NodeCards structures
    static bool AreEqual(const NodeCards& a, const NodeCards& b);
    
    // Compare bounds with tolerance
    static bool BoundsAreEquivalent(
    const NodeCards& a, 
    const NodeCards& b,
        int tolerance = 0
    );
    
    // Verify relative rank equivalence
    static bool RelativeRanksMatch(
        const unsigned short ranks1[DDS_SUITS],
        const unsigned short ranks2[DDS_SUITS],
        const unsigned short winMask[DDS_SUITS]
    );
    
    // Helper to print position data for debugging
    static std::string PositionToString(const NodeCards& node);
    static std::string RanksToString(const unsigned short ranks[DDS_SUITS]);
};

// Test data validation helpers
class TestDataValidator {
public:
    // Validate hand distribution is legal
    static bool IsValidHandDistribution(const int handDist[DDS_HANDS]);
    
    // Validate aggregate target data
    static bool IsValidAggrTarget(const unsigned short aggrTarget[DDS_SUITS]);
    
    // Validate winning ranks are consistent
    static bool IsValidWinRanks(const unsigned short winRanks[DDS_SUITS]);
    
    // Validate node data is reasonable
    static bool IsValidNodeData(const NodeCards& node);
    
    // Check if trick/hand parameters are in valid range
    static bool IsValidTrickHand(int trick, int hand);
};

} // namespace dds_test

#endif // DDS_TEST_UTILITIES_H
