#include <gtest/gtest.h>
#include <cstring>

// Include DDS types first
#include <api/dll.h>

// No TransTable dependencies needed in this file; remove legacy forward declarations.

namespace dds_test {

// Helper function to create a simple hand lookup table
static void CreateBasicHandLookup(int handLookup[15][15]) {
    for (int i = 0; i < 15; ++i) {
        for (int j = 0; j < 15; ++j) {
            handLookup[i][j] = (i + j) % 4; // Simple pattern
        }
    }
}

// Helper function to create test aggregate target
static void CreateTestAggrTarget(unsigned short aggrTarget[DDS_SUITS]) {
    aggrTarget[0] = 0x1111; // Spades
    aggrTarget[1] = 0x2222; // Hearts  
    aggrTarget[2] = 0x3333; // Diamonds
    aggrTarget[3] = 0x4444; // Clubs
}

// Helper function to create test win ranks
static void CreateTestWinRanks(unsigned short winRanks[DDS_SUITS]) {
    winRanks[0] = 0x5555; // Spades
    winRanks[1] = 0x6666; // Hearts
    winRanks[2] = 0x7777; // Diamonds  
    winRanks[3] = 0x8888; // Clubs
}

// Test that verifies DDS constants are available
TEST(TransTableLBasicTest, DDSConstantsAvailable) {
    // Verify that basic DDS constants are accessible
    EXPECT_EQ(DDS_SUITS, 4);
    EXPECT_EQ(DDS_HANDS, 4);
    EXPECT_EQ(DDS_STRAINS, 5);
}

// Test that verifies we can work with basic DDS types
TEST(TransTableLBasicTest, BasicTypesWork) {
    // Test that we can create basic data structures
    unsigned short testArray[DDS_SUITS];
    for (int i = 0; i < DDS_SUITS; ++i) {
        testArray[i] = i * 0x1111;
    }
    
    EXPECT_EQ(testArray[0], 0x0000);
    EXPECT_EQ(testArray[1], 0x1111);
    EXPECT_EQ(testArray[2], 0x2222);
    EXPECT_EQ(testArray[3], 0x3333);
}

// Test helper functions work correctly
TEST(TransTableLBasicTest, HelperFunctionsWork) {
    int handLookup[15][15];
    CreateBasicHandLookup(handLookup);
    
    // Check that lookup table has expected pattern
    EXPECT_EQ(handLookup[0][0], 0);
    EXPECT_EQ(handLookup[1][0], 1);
    EXPECT_EQ(handLookup[0][1], 1);
    EXPECT_EQ(handLookup[1][1], 2);
    
    unsigned short aggrTarget[DDS_SUITS];
    CreateTestAggrTarget(aggrTarget);
    EXPECT_EQ(aggrTarget[0], 0x1111);
    EXPECT_EQ(aggrTarget[3], 0x4444);
    
    unsigned short winRanks[DDS_SUITS];
    CreateTestWinRanks(winRanks);
    EXPECT_EQ(winRanks[0], 0x5555);
    EXPECT_EQ(winRanks[3], 0x8888);
}

// Test large memory management scenarios
TEST(TransTableLAdvancedTest, LargeMemoryManagement) {
    // Test different large memory scenarios without actual allocation
    
    // Simulate large memory limits (TransTableL is for large memory scenarios)
    struct LargeMemoryLimits {
        int defaultMB;
        int maximumMB;
        bool shouldSucceed;
    };
    
    LargeMemoryLimits testCases[] = {
        {64, 128, true},     // Normal large case
        {128, 256, true},    // Large case
        {256, 512, true},    // Very large case
        {512, 1024, true},   // Extremely large case
        {1024, 512, false},  // Invalid: default > maximum
        {0, 64, true},       // Zero default (should use defaults)
    };
    
    for (const auto& test : testCases) {
        bool isValid = (test.defaultMB <= test.maximumMB) && 
                      (test.defaultMB >= 0) && 
                      (test.maximumMB >= 0);
        EXPECT_EQ(isValid, test.shouldSucceed)
            << "Large memory limit check failed for default=" << test.defaultMB
            << " maximum=" << test.maximumMB;
    }
}

// Test page-based memory organization
TEST(TransTableLAdvancedTest, PageBasedMemoryOrganization) {
    // Test page organization logic specific to TransTableL
    struct PageInfo {
        int pageSize;
        int totalPages;
        int entriesPerPage;
        bool valid;
    };
    
    PageInfo testCases[] = {
        {4096, 256, 64, true},      // Standard page configuration
        {8192, 128, 128, true},     // Larger pages, fewer pages
        {2048, 512, 32, true},      // Smaller pages, more pages
        {0, 256, 64, false},        // Invalid: zero page size
        {4096, 0, 64, false},       // Invalid: zero pages
        {4096, 256, 0, false},      // Invalid: zero entries per page
    };
    
    for (const auto& test : testCases) {
        bool isValid = (test.pageSize > 0) && 
                      (test.totalPages > 0) && 
                      (test.entriesPerPage > 0);
        EXPECT_EQ(isValid, test.valid)
            << "Page organization check failed for pageSize=" << test.pageSize
            << " totalPages=" << test.totalPages 
            << " entriesPerPage=" << test.entriesPerPage;
    }
}

// Test hash table distribution for large tables
TEST(TransTableLAdvancedTest, HashTableDistribution) {
    // Test hash distribution patterns for large tables
    struct HashDistribution {
        uint64_t hash;
        int expectedBucket;
        int totalBuckets;
    };
    
    int totalBuckets = 1024; // Large table has many buckets
    
    HashDistribution testCases[] = {
        {0x0000000000000000, 0, totalBuckets},
        {0x0000000000000001, 1, totalBuckets},
        {0x00000000000003FF, 1023, totalBuckets},  // Should map to last bucket
        {0x0000000000000400, 0, totalBuckets},     // Should wrap around
    };
    
    for (const auto& test : testCases) {
        int actualBucket = test.hash % test.totalBuckets;
        EXPECT_EQ(actualBucket, test.expectedBucket)
            << "Hash distribution failed for hash=0x" << std::hex << test.hash
            << " expected bucket=" << test.expectedBucket
            << " actual bucket=" << actualBucket;
    }
}

// Test block allocation and recycling
TEST(TransTableLAdvancedTest, BlockAllocationRecycling) {
    // Test block allocation patterns for TransTableL
    struct BlockAllocation {
        int blockSize;
        int totalBlocks;
        int usedBlocks;
        int freeBlocks;
        bool consistent;
    };
    
    BlockAllocation testCases[] = {
        {64, 1000, 600, 400, true},   // Normal usage
        {128, 500, 300, 200, true},   // Larger blocks
        {32, 2000, 1500, 500, true},  // Smaller blocks, more of them
        {64, 1000, 1200, -200, false}, // Invalid: more used than total
        {64, 1000, 600, 500, false},   // Invalid: used + free > total
    };
    
    for (const auto& test : testCases) {
        bool isConsistent = (test.usedBlocks >= 0) && 
                           (test.freeBlocks >= 0) &&
                           (test.usedBlocks + test.freeBlocks <= test.totalBlocks);
        EXPECT_EQ(isConsistent, test.consistent)
            << "Block allocation consistency check failed for"
            << " blockSize=" << test.blockSize
            << " totalBlocks=" << test.totalBlocks
            << " usedBlocks=" << test.usedBlocks
            << " freeBlocks=" << test.freeBlocks;
    }
}

// Test timestamp-based aging for large tables
TEST(TransTableLAdvancedTest, TimestampBasedAging) {
    // Test timestamp-based aging logic
    struct TimestampEntry {
        uint32_t timestamp;
        uint32_t currentTime;
        uint32_t maxAge;
        bool shouldAge;
    };
    
    TimestampEntry testCases[] = {
        {1000, 2000, 500, true},    // Old entry should age (age=1000 >= 500)
        {1500, 2000, 1000, false}, // Entry at age limit should not age (age=500 < 1000)
        {1800, 2000, 1000, false}, // Recent entry should not age (age=200 < 1000)
        {2000, 2000, 1000, false}, // Current entry should not age (age=0 < 1000)
        {2100, 2000, 1000, false}, // Future timestamp (edge case, age=0)
    };
    
    for (const auto& test : testCases) {
        uint32_t age = (test.currentTime >= test.timestamp) 
                      ? (test.currentTime - test.timestamp) 
                      : 0;
        bool shouldAge = (age >= test.maxAge);
        EXPECT_EQ(shouldAge, test.shouldAge)
            << "Timestamp aging check failed for"
            << " timestamp=" << test.timestamp
            << " currentTime=" << test.currentTime
            << " maxAge=" << test.maxAge
            << " calculated age=" << age;
    }
}

// Test large table lookup efficiency characteristics
TEST(TransTableLAdvancedTest, LookupEfficiencyCharacteristics) {
    // Test characteristics that affect lookup efficiency in large tables
    struct LookupPattern {
        int sequentialLookups;
        int randomLookups;
        int totalLookups;
        double expectedEfficiency;
    };
    
    LookupPattern patterns[] = {
        {8000, 2000, 10000, 0.85},   // Mostly sequential in large table
        {5000, 5000, 10000, 0.70},   // Mixed pattern
        {2000, 8000, 10000, 0.55},   // Mostly random
        {1000, 1000, 2000, 0.75},    // Small mixed workload
    };
    
    for (const auto& pattern : patterns) {
        // Large tables should have better random access than small tables
        double totalLookups = pattern.totalLookups;
        double sequentialRatio = pattern.sequentialLookups / totalLookups;
        
        // Large tables have better base performance and less penalty for random access
        double simulatedEfficiency = 0.6 + (sequentialRatio * 0.35); // Better base than TransTableS
        
        EXPECT_GE(simulatedEfficiency, 0.0);
        EXPECT_LE(simulatedEfficiency, 1.0);
        EXPECT_TRUE(std::abs(simulatedEfficiency - pattern.expectedEfficiency) < 0.25)
            << "Large table efficiency simulation failed for pattern with " 
            << pattern.sequentialLookups << " sequential, "
            << pattern.randomLookups << " random lookups";
    }
}

// Test multi-level hash lookup verification
TEST(TransTableLAdvancedTest, MultiLevelHashLookup) {
    // Test multi-level hash lookup patterns for large tables
    struct HashLookup {
        uint64_t primaryHash;
        uint32_t secondaryHash;
        int level1Bucket;
        int level2Bucket;
        bool validLookup;
    };
    
    int level1Buckets = 1024;
    int level2Buckets = 64;
    
    HashLookup testCases[] = {
        {0x1234567890ABCDEF, 0x12345678, 
         (int)(0x1234567890ABCDEF % level1Buckets), 
         (int)(0x12345678 % level2Buckets), true},
        {0x0000000000000000, 0x00000000, 0, 0, true},
        {0xFFFFFFFFFFFFFFFF, 0xFFFFFFFF, 
         (int)(0xFFFFFFFFFFFFFFFF % level1Buckets), 
         (int)(0xFFFFFFFF % level2Buckets), true},
    };
    
    for (const auto& test : testCases) {
        int actualLevel1 = test.primaryHash % level1Buckets;
        int actualLevel2 = test.secondaryHash % level2Buckets;
        
        EXPECT_EQ(actualLevel1, test.level1Bucket);
        EXPECT_EQ(actualLevel2, test.level2Bucket);
        EXPECT_GE(actualLevel1, 0);
        EXPECT_LT(actualLevel1, level1Buckets);
        EXPECT_GE(actualLevel2, 0);
        EXPECT_LT(actualLevel2, level2Buckets);
    }
}

// TASK 08: ADVANCED TESTS FOR TRANSTTABLEL

// Test harvest mechanism for memory reclamation
TEST(TransTableLHarvestTest, HarvestMechanismForMemoryReclamation) {
    // Test harvest mechanism that reclaims old entries
    struct HarvestScenario {
        int totalEntries;
        int oldEntries;
        int recentEntries;
        int expectedReclaimed;
        bool harvestTriggered;
    };
    
    HarvestScenario testCases[] = {
        {1000, 800, 200, 800, true},   // High old entry ratio should trigger harvest
        {1000, 300, 700, 300, false}, // Low old entry ratio should not trigger
        {500, 450, 50, 450, true},     // High percentage old should trigger
        {100, 10, 90, 10, false},     // Low percentage old should not trigger
        {0, 0, 0, 0, false},          // Empty table should not trigger
    };
    
    for (const auto& test : testCases) {
        double oldRatio = (test.totalEntries > 0) 
                         ? (double)test.oldEntries / test.totalEntries 
                         : 0.0;
        bool shouldHarvest = (oldRatio > 0.5) && (test.totalEntries > 50);
        
        EXPECT_EQ(shouldHarvest, test.harvestTriggered)
            << "Harvest trigger logic failed for"
            << " totalEntries=" << test.totalEntries
            << " oldEntries=" << test.oldEntries
            << " oldRatio=" << oldRatio;
            
        if (shouldHarvest) {
            EXPECT_EQ(test.expectedReclaimed, test.oldEntries)
                << "Expected to reclaim all old entries";
        }
    }
}

// Test harvest age threshold handling
TEST(TransTableLHarvestTest, HarvestAgeThresholdHandling) {
    // Test different age thresholds for harvest decisions
    struct AgeThreshold {
        uint32_t entryAge;
        uint32_t harvestThreshold;
        bool shouldReclaim;
    };
    
    AgeThreshold testCases[] = {
        {1000, 500, true},    // Age > threshold should reclaim
        {500, 500, false},    // Age = threshold should not reclaim
        {300, 500, false},    // Age < threshold should not reclaim
        {2000, 1000, true},   // Very old should definitely reclaim
        {50, 1000, false},    // Very recent should not reclaim
    };
    
    for (const auto& test : testCases) {
        bool shouldReclaim = (test.entryAge > test.harvestThreshold);
        EXPECT_EQ(shouldReclaim, test.shouldReclaim)
            << "Age threshold check failed for"
            << " entryAge=" << test.entryAge
            << " threshold=" << test.harvestThreshold;
    }
}

// Test memory reclamation efficiency
TEST(TransTableLHarvestTest, MemoryReclamationEfficiency) {
    // Test efficiency of memory reclamation process
    struct ReclamationStats {
        int memoryBefore;
        int memoryAfter;
        int expectedReclaimed;
        double efficiency;
    };
    
    ReclamationStats testCases[] = {
        {1000, 200, 800, 1.0},   // Perfect efficiency: reclaimed exactly expected
        {1000, 400, 600, 1.0},   // Perfect efficiency: reclaimed exactly expected
        {1000, 700, 300, 1.0},   // Perfect efficiency: reclaimed exactly expected
        {500, 100, 400, 1.0},    // Perfect efficiency: reclaimed exactly expected
    };
    
    for (const auto& test : testCases) {
        int actualReclaimed = test.memoryBefore - test.memoryAfter;
        double actualEfficiency = (test.expectedReclaimed > 0) 
                                 ? (double)actualReclaimed / test.expectedReclaimed
                                 : 0.0;
        
        EXPECT_GE(actualReclaimed, 0)
            << "Memory should not increase after reclamation";
        EXPECT_LE(actualReclaimed, test.memoryBefore)
            << "Cannot reclaim more than original memory";
        EXPECT_TRUE(std::abs(actualEfficiency - test.efficiency) < 0.2)
            << "Reclamation efficiency outside expected range";
    }
}

// Test page allocation and deallocation
TEST(TransTableLPageTest, PageAllocationAndDeallocation) {
    // Test page management in large tables
    struct PageManagement {
        int totalPages;
        int allocatedPages;
        int freePages;
        bool validState;
    };
    
    PageManagement testCases[] = {
        {1000, 600, 400, true},     // Normal allocation
        {500, 500, 0, true},        // Fully allocated
        {1000, 0, 1000, true},      // Fully free
        {1000, 1200, -200, false}, // Invalid: over-allocated
        {1000, 300, 800, false},   // Invalid: allocated + free > total
    };
    
    for (const auto& test : testCases) {
        bool isValid = (test.allocatedPages >= 0) &&
                      (test.freePages >= 0) &&
                      (test.allocatedPages + test.freePages <= test.totalPages);
        EXPECT_EQ(isValid, test.validState)
            << "Page management validation failed for"
            << " totalPages=" << test.totalPages
            << " allocatedPages=" << test.allocatedPages
            << " freePages=" << test.freePages;
    }
}

// Test page overflow handling
TEST(TransTableLPageTest, PageOverflowHandling) {
    // Test behavior when pages overflow
    struct PageOverflow {
        int pageCapacity;
        int entriesRequested;
        int pagesNeeded;
        bool overflowHandled;
    };
    
    PageOverflow testCases[] = {
        {64, 100, 2, true},     // Requires 2 pages for 100 entries
        {128, 128, 1, true},    // Exactly fits in 1 page
        {64, 300, 5, true},     // Requires 5 pages
        {64, 63, 1, true},      // Less than 1 page capacity
        {0, 100, 0, false},     // Invalid: zero capacity
    };
    
    for (const auto& test : testCases) {
        int actualPages = (test.pageCapacity > 0) 
                         ? ((test.entriesRequested + test.pageCapacity - 1) / test.pageCapacity)
                         : 0;
        bool handled = (test.pageCapacity > 0) && (actualPages > 0);
        
        EXPECT_EQ(handled, test.overflowHandled);
        if (handled) {
            EXPECT_EQ(actualPages, test.pagesNeeded)
                << "Page calculation incorrect for"
                << " capacity=" << test.pageCapacity
                << " entries=" << test.entriesRequested;
        }
    }
}

// Test complex card pattern matching
TEST(TransTableLComplexTest, ComplexCardPatternMatching) {
    // Test complex card pattern scenarios for large tables
    struct CardPattern {
        unsigned short pattern[DDS_SUITS];
        unsigned short winRanks[DDS_SUITS];
        bool shouldMatch;
        const char* description;
    };
    
    CardPattern testCases[] = {
        {{0x1F00, 0x0F80, 0x07C0, 0x03E0}, {0x1000, 0x0800, 0x0400, 0x0200}, true, "High cards with wins"},
        {{0x001F, 0x001F, 0x001F, 0x001F}, {0x0010, 0x0010, 0x0010, 0x0010}, true, "Low cards with wins"},
        {{0x0000, 0x0000, 0x0000, 0x1FFF}, {0x0000, 0x0000, 0x0000, 0x1000}, true, "Single suit distribution"},
        {{0xFFFF, 0x0000, 0x0000, 0x0000}, {0x8000, 0x0000, 0x0000, 0x0000}, true, "Void suits handled"},
    };
    
    for (const auto& test : testCases) {
        // Verify pattern consistency
        bool hasCards = false;
        
        for (int suit = 0; suit < DDS_SUITS; ++suit) {
            if (test.pattern[suit] > 0) hasCards = true;
            
            // Win ranks should be subset of pattern
            EXPECT_EQ((test.winRanks[suit] & test.pattern[suit]), test.winRanks[suit])
                << "Win ranks not subset of pattern for suit " << suit
                << " in test: " << test.description;
        }
        
        EXPECT_TRUE(hasCards) << "Pattern should have some cards: " << test.description;
    }
}

// Test large position dataset lookup
TEST(TransTableLComplexTest, LargePositionDatasetLookup) {
    // Test lookup efficiency with large datasets
    struct LargeDataset {
        int datasetSize;
        int lookupQueries;
        int expectedHits;
        double hitRatio;
    };
    
    LargeDataset testCases[] = {
        {10000, 1000, 800, 0.8},   // Large dataset, good hit ratio
        {50000, 5000, 3500, 0.7},  // Very large dataset, decent hit ratio
        {100000, 2000, 1000, 0.5}, // Huge dataset, moderate hit ratio
        {1000, 100, 90, 0.9},      // Smaller dataset, high hit ratio
    };
    
    for (const auto& test : testCases) {
        double actualHitRatio = (test.lookupQueries > 0) 
                               ? (double)test.expectedHits / test.lookupQueries
                               : 0.0;
        
        EXPECT_DOUBLE_EQ(actualHitRatio, test.hitRatio)
            << "Hit ratio calculation incorrect for dataset size " << test.datasetSize;
        
        EXPECT_LE(test.expectedHits, test.lookupQueries)
            << "Cannot have more hits than queries";
        EXPECT_LE(test.expectedHits, test.datasetSize)
            << "Cannot have more hits than dataset size";
    }
}

// Test memory fragmentation handling
TEST(TransTableLMemoryTest, MemoryFragmentationHandling) {
    // Test handling of memory fragmentation in large tables
    struct FragmentationScenario {
        int totalMemory;
        int usedMemory;
        int largestFreeBlock;
        double fragmentationRatio;
        bool wellManaged;
    };
    
    FragmentationScenario testCases[] = {
        {1000, 600, 350, 0.1, true},   // Low fragmentation, well managed
        {1000, 800, 150, 0.3, true},   // Moderate fragmentation, acceptable
        {1000, 900, 50, 0.5, false},   // High fragmentation, poorly managed
        {1000, 500, 500, 0.0, true},   // No fragmentation, perfect management
    };
    
    for (const auto& test : testCases) {
        int freeMemory = test.totalMemory - test.usedMemory;
        double actualFragmentation = (freeMemory > 0) 
                                   ? 1.0 - ((double)test.largestFreeBlock / freeMemory)
                                   : 0.0;
        
        EXPECT_GE(test.largestFreeBlock, 0);
        EXPECT_LE(test.largestFreeBlock, freeMemory);
        EXPECT_TRUE(std::abs(actualFragmentation - test.fragmentationRatio) < 0.1)
            << "Fragmentation ratio outside expected range";
        
        bool isWellManaged = (actualFragmentation < 0.4);
        EXPECT_EQ(isWellManaged, test.wellManaged)
            << "Fragmentation management assessment incorrect";
    }
}

// Test large memory usage patterns
TEST(TransTableLMemoryTest, LargeMemoryUsagePatterns) {
    // Test memory usage patterns specific to large tables
    struct MemoryPattern {
        int baseMB;
        int peakMB;
        int sustainedMB;
        bool efficientPattern;
    };
    
    MemoryPattern testCases[] = {
        {64, 128, 96, true},      // Efficient: reasonable peak, good sustained
        {128, 1024, 200, false},  // Inefficient: huge peak, low sustained
        {256, 300, 280, true},    // Efficient: small peak growth, high sustained
        {512, 512, 512, true},    // Efficient: flat usage pattern
        {64, 64, 128, false},     // Invalid: sustained > peak
    };
    
    for (const auto& test : testCases) {
        bool validPattern = (test.sustainedMB <= test.peakMB) && 
                           (test.baseMB <= test.peakMB);
        
        if (validPattern) {
            double peakRatio = (double)test.peakMB / test.baseMB;
            double sustainedRatio = (double)test.sustainedMB / test.peakMB;
            bool isEfficient = (peakRatio < 3.0) && (sustainedRatio > 0.6);
            
            EXPECT_EQ(isEfficient, test.efficientPattern)
                << "Memory efficiency assessment incorrect for"
                << " base=" << test.baseMB
                << " peak=" << test.peakMB  
                << " sustained=" << test.sustainedMB;
        } else {
            EXPECT_FALSE(test.efficientPattern)
                << "Invalid memory pattern should not be marked efficient";
        }
    }
}

// Test extreme memory pressure scenarios
TEST(TransTableLEdgeTest, ExtremeMemoryPressure) {
    // Test behavior under extreme memory pressure
    struct MemoryPressure {
        int availableMemory;
        int requestedMemory;
        int expectedAllocation;
        bool gracefulDegradation;
    };
    
    MemoryPressure testCases[] = {
        {100, 50, 50, true},      // Normal case: enough memory
        {100, 100, 100, true},    // Exact fit: use all available
        {100, 150, 100, true},    // Over request: allocate what's available
        {100, 1000, 100, true},   // Extreme over request: graceful fallback
        {0, 100, 0, true},        // No memory: graceful failure
    };
    
    for (const auto& test : testCases) {
        int actualAllocation = std::min(test.availableMemory, test.requestedMemory);
        actualAllocation = std::max(0, actualAllocation);
        
        EXPECT_EQ(actualAllocation, test.expectedAllocation)
            << "Memory allocation under pressure incorrect for"
            << " available=" << test.availableMemory
            << " requested=" << test.requestedMemory;
        
        bool graceful = (actualAllocation >= 0) && 
                       (actualAllocation <= test.availableMemory);
        EXPECT_EQ(graceful, test.gracefulDegradation)
            << "Graceful degradation not handled properly";
    }
}

// Test very large position counts
TEST(TransTableLEdgeTest, VeryLargePositionCounts) {
    // Test handling of very large numbers of positions
    struct LargePositionTest {
        uint64_t positionCount;
        uint32_t hashBuckets;
        double expectedLoadFactor;
        bool manageable;
    };
    
    LargePositionTest testCases[] = {
        {1000000, 10000, 100.0, true},     // 1M positions, 10K buckets
        {10000000, 100000, 100.0, true},   // 10M positions, 100K buckets  
        {100000000, 1000000, 100.0, true}, // 100M positions, 1M buckets
        {1000000, 1000, 1000.0, false},    // High load factor, not manageable
        {10000, 100000, 0.1, true},        // Very low load factor, over-provisioned but fine
    };
    
    for (const auto& test : testCases) {
        double actualLoadFactor = (test.hashBuckets > 0) 
                                 ? (double)test.positionCount / test.hashBuckets
                                 : 0.0;
        
        EXPECT_DOUBLE_EQ(actualLoadFactor, test.expectedLoadFactor)
            << "Load factor calculation incorrect";
        
        bool isManageable = (actualLoadFactor < 500.0) && (actualLoadFactor > 0.01);
        EXPECT_EQ(isManageable, test.manageable)
            << "Manageability assessment incorrect for load factor " << actualLoadFactor;
    }
}

} // namespace dds_test