#include <gtest/gtest.h>
#include <chrono>
#include "library/src/utility/LookupTables.h"

class LookupTablesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize lookup tables before each test
        InitLookupTables();
    }

    void TearDown() override {
        // Any cleanup needed after each test
    }
};

// Test InitLookupTables function
TEST_F(LookupTablesTest, InitLookupTablesNoErrors) {
    // Test that function can be called without errors
    EXPECT_NO_THROW(InitLookupTables());
}

TEST_F(LookupTablesTest, InitLookupTablesIdempotent) {
    // Test that calling multiple times is safe
    InitLookupTables();
    InitLookupTables();
    InitLookupTables();
    
    // Should complete without any issues
    SUCCEED();
}

// Test lookup table properties
TEST_F(LookupTablesTest, HighestRankArray) {
    // Test that highestRank array has valid values
    for (int i = 0; i < 8192; i++) {
        EXPECT_GE(highestRank[i], 0) << "highestRank[" << i << "] should be non-negative";
        EXPECT_LE(highestRank[i], 14) << "highestRank[" << i << "] should be <= 14";
    }
}

TEST_F(LookupTablesTest, LowestRankArray) {
    // Test that lowestRank array has valid values
    for (int i = 0; i < 8192; i++) {
        EXPECT_GE(lowestRank[i], 0) << "lowestRank[" << i << "] should be non-negative";
        EXPECT_LE(lowestRank[i], 14) << "lowestRank[" << i << "] should be <= 14";
    }
}

TEST_F(LookupTablesTest, CountTableArray) {
    // Test that counttable array has correct bit counts
    for (int i = 0; i < 8192; i++) {
        // Count actual bits in i
        int actualCount = 0;
        for (int bit = 0; bit < 13; bit++) {
            if (i & (1 << bit)) {
                actualCount++;
            }
        }
        
        EXPECT_EQ(counttable[i], actualCount) 
            << "counttable[" << i << "] should equal actual bit count";
    }
}

TEST_F(LookupTablesTest, RelRankArrayDimensions) {
    // Test relRank array dimensions and value ranges
    for (int i = 0; i < 8192; i++) {
        for (int j = 0; j < 15; j++) {
            EXPECT_GE(relRank[i][j], 0) 
                << "relRank[" << i << "][" << j << "] should be non-negative";
            EXPECT_LE(relRank[i][j], 14) 
                << "relRank[" << i << "][" << j << "] should be <= 14";
        }
    }
}

TEST_F(LookupTablesTest, WinRanksArrayDimensions) {
    // Test winRanks array dimensions and value ranges
    for (int i = 0; i < 8192; i++) {
        for (int j = 0; j < 14; j++) {
            EXPECT_GE(winRanks[i][j], 0) 
                << "winRanks[" << i << "][" << j << "] should be non-negative";
            EXPECT_LE(winRanks[i][j], 8191) 
                << "winRanks[" << i << "][" << j << "] should be <= 8191";
        }
    }
}

TEST_F(LookupTablesTest, GroupDataArray) {
    // Test that groupData array has valid moveGroupType entries
    for (int i = 0; i < 8192; i++) {
        // Test that we can access the struct members
    EXPECT_NO_THROW((void)groupData[i].lastGroup);
    EXPECT_NO_THROW((void)groupData[i].rank[0]);
    EXPECT_NO_THROW((void)groupData[i].sequence[0]);
    EXPECT_NO_THROW((void)groupData[i].fullseq[0]);
    EXPECT_NO_THROW((void)groupData[i].gap[0]);
    }
}

// Test table consistency and relationships
TEST_F(LookupTablesTest, HighestLowestRankConsistency) {
    // Test relationships between highest and lowest rank
    for (int i = 1; i < 8192; i++) { // Skip i=0 (empty set)
        if (counttable[i] > 0) {
            EXPECT_GE(highestRank[i], lowestRank[i])
                << "For i=" << i << ", highest rank should be >= lowest rank";
        }
    }
}

TEST_F(LookupTablesTest, CountTableConsistency) {
    // Test some specific known values
    EXPECT_EQ(counttable[0], 0);    // Empty set has 0 bits
    EXPECT_EQ(counttable[1], 1);    // Single bit has count 1
    EXPECT_EQ(counttable[3], 2);    // Two bits have count 2
    EXPECT_EQ(counttable[7], 3);    // Three bits have count 3
    EXPECT_EQ(counttable[8191], 13); // All 13 bits set
}

TEST_F(LookupTablesTest, EmptySetHandling) {
    // Test that empty set (i=0) is handled correctly
    EXPECT_EQ(counttable[0], 0);
    // Other values for i=0 depend on implementation but should be well-defined
    EXPECT_NO_THROW((void)highestRank[0]);
    EXPECT_NO_THROW((void)lowestRank[0]);
}

TEST_F(LookupTablesTest, FullSetHandling) {
    // Test that full set (i=8191 = 2^13-1) is handled correctly
    EXPECT_EQ(counttable[8191], 13);
    EXPECT_EQ(highestRank[8191], 14); // Ace should be highest
    EXPECT_EQ(lowestRank[8191], 2);   // Deuce should be lowest
}

// Test moveGroupType struct functionality
TEST_F(LookupTablesTest, MoveGroupTypeStruct) {
    // Test that we can create and use moveGroupType
    moveGroupType testGroup;
    testGroup.lastGroup = 3;
    testGroup.rank[0] = 5;
    testGroup.sequence[0] = 3;
    testGroup.fullseq[0] = 1;
    testGroup.gap[0] = 7;
    
    EXPECT_EQ(testGroup.lastGroup, 3);
    EXPECT_EQ(testGroup.rank[0], 5);
    EXPECT_EQ(testGroup.sequence[0], 3);
    EXPECT_EQ(testGroup.fullseq[0], 1);
    EXPECT_EQ(testGroup.gap[0], 7);
}

TEST_F(LookupTablesTest, GroupDataStructAccess) {
    // Test that we can access groupData array entries
    for (int i = 0; i < 10; i++) { // Test first 10 entries
        const moveGroupType& group = groupData[i];
        
        // Values should be in reasonable ranges
        // lastGroup can be -1 to indicate no groups
        EXPECT_GE(group.lastGroup, -1);
        EXPECT_LE(group.lastGroup, 6); // At most 7 groups (0-6)
        EXPECT_GE(group.rank[0], 0);
        EXPECT_LE(group.rank[0], 15);
        EXPECT_GE(group.sequence[0], 0);
        EXPECT_GE(group.fullseq[0], 0);
        EXPECT_GE(group.gap[0], 0);
    }
}

// Performance test
TEST_F(LookupTablesTest, InitializationPerformance) {
    // Test that initialization completes in reasonable time
    auto start = std::chrono::high_resolution_clock::now();
    
    InitLookupTables();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within 1 second (adjust if needed)
    EXPECT_LT(duration.count(), 1000) 
        << "InitLookupTables should complete within 1 second";
}
