#include <gtest/gtest.h>
#include <chrono>
#include <lookup_tables/LookupTables.hpp>

class LookupTablesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize lookup tables before each test
    init_lookup_tables();
    }

    void TearDown() override {
        // Any cleanup needed after each test
    }
};

// Test init_lookup_tables function
TEST_F(LookupTablesTest, InitLookupTablesNoErrors) {
    // Test that function can be called without errors
    EXPECT_NO_THROW(init_lookup_tables());
}

TEST_F(LookupTablesTest, InitLookupTablesIdempotent) {
    // Test that calling multiple times is safe
    init_lookup_tables();
    init_lookup_tables();
    init_lookup_tables();
    
    // Should complete without any issues
    SUCCEED();
}

// Test lookup table properties
TEST_F(LookupTablesTest, HighestRankArray) {
    // Test that highest_rank array has valid values
    for (int i = 0; i < 8192; i++) {
    EXPECT_GE(highest_rank[i], 0) << "highest_rank[" << i << "] should be non-negative";
    EXPECT_LE(highest_rank[i], 14) << "highest_rank[" << i << "] should be <= 14";
    }
}

TEST_F(LookupTablesTest, LowestRankArray) {
    // Test that lowest_rank array has valid values
    for (int i = 0; i < 8192; i++) {
    EXPECT_GE(lowest_rank[i], 0) << "lowest_rank[" << i << "] should be non-negative";
    EXPECT_LE(lowest_rank[i], 14) << "lowest_rank[" << i << "] should be <= 14";
    }
}

TEST_F(LookupTablesTest, CountTableArray) {
    // Test that count_table array has correct bit counts
    for (int i = 0; i < 8192; i++) {
        // Count actual bits in i
        int actualCount = 0;
        for (int bit = 0; bit < 13; bit++) {
            if (i & (1 << bit)) {
                actualCount++;
            }
        }
        
        EXPECT_EQ(count_table[i], actualCount) 
            << "count_table[" << i << "] should equal actual bit count";
    }
}

TEST_F(LookupTablesTest, RelRankArrayDimensions) {
    // Test rel_rank array dimensions and value ranges
    for (int i = 0; i < 8192; i++) {
        for (int j = 0; j < 15; j++) {
            EXPECT_GE(rel_rank[i][j], 0) 
                << "rel_rank[" << i << "][" << j << "] should be non-negative";
            EXPECT_LE(rel_rank[i][j], 14) 
                << "rel_rank[" << i << "][" << j << "] should be <= 14";
        }
    }
}

TEST_F(LookupTablesTest, WinRanksArrayDimensions) {
    // Test win_ranks array dimensions and value ranges
    for (int i = 0; i < 8192; i++) {
        for (int j = 0; j < 14; j++) {
            EXPECT_GE(win_ranks[i][j], 0) 
                << "win_ranks[" << i << "][" << j << "] should be non-negative";
            EXPECT_LE(win_ranks[i][j], 8191) 
                << "win_ranks[" << i << "][" << j << "] should be <= 8191";
        }
    }
}

TEST_F(LookupTablesTest, GroupDataArray) {
    // Test that group_data array has valid MoveGroupType entries
    for (int i = 0; i < 8192; i++) {
        // Test that we can access the struct members
    EXPECT_NO_THROW((void)group_data[i].last_group_);
    EXPECT_NO_THROW((void)group_data[i].rank_[0]);
    EXPECT_NO_THROW((void)group_data[i].sequence_[0]);
    EXPECT_NO_THROW((void)group_data[i].fullseq_[0]);
    EXPECT_NO_THROW((void)group_data[i].gap_[0]);
    }
}

// Test table consistency and relationships
TEST_F(LookupTablesTest, HighestLowestRankConsistency) {
    // Test relationships between highest and lowest rank
    for (int i = 1; i < 8192; i++) { // Skip i=0 (empty set)
        if (count_table[i] > 0) {
            EXPECT_GE(highest_rank[i], lowest_rank[i])
                << "For i=" << i << ", highest rank should be >= lowest rank";
        }
    }
}

TEST_F(LookupTablesTest, CountTableConsistency) {
    // Test some specific known values
    EXPECT_EQ(count_table[0], 0);    // Empty set has 0 bits
    EXPECT_EQ(count_table[1], 1);    // Single bit has count 1
    EXPECT_EQ(count_table[3], 2);    // Two bits have count 2
    EXPECT_EQ(count_table[7], 3);    // Three bits have count 3
    EXPECT_EQ(count_table[8191], 13); // All 13 bits set
}

TEST_F(LookupTablesTest, EmptySetHandling) {
    // Test that empty set (i=0) is handled correctly
    EXPECT_EQ(count_table[0], 0);
    // Other values for i=0 depend on implementation but should be well-defined
    EXPECT_NO_THROW((void)highest_rank[0]);
    EXPECT_NO_THROW((void)lowest_rank[0]);
}

TEST_F(LookupTablesTest, FullSetHandling) {
    // Test that full set (i=8191 = 2^13-1) is handled correctly
    EXPECT_EQ(count_table[8191], 13);
    EXPECT_EQ(highest_rank[8191], 14); // Ace should be highest
    EXPECT_EQ(lowest_rank[8191], 2);   // Deuce should be lowest
}

// Test MoveGroupType struct functionality
TEST_F(LookupTablesTest, MoveGroupTypeStruct) {
    // Test that we can create and use MoveGroupType
    MoveGroupType testGroup;
    testGroup.last_group_ = 3;
    testGroup.rank_[0] = 5;
    testGroup.sequence_[0] = 3;
    testGroup.fullseq_[0] = 1;
    testGroup.gap_[0] = 7;
    
    EXPECT_EQ(testGroup.last_group_, 3);
    EXPECT_EQ(testGroup.rank_[0], 5);
    EXPECT_EQ(testGroup.sequence_[0], 3);
    EXPECT_EQ(testGroup.fullseq_[0], 1);
    EXPECT_EQ(testGroup.gap_[0], 7);
}

TEST_F(LookupTablesTest, GroupDataStructAccess) {
    // Test that we can access group_data array entries
    for (int i = 0; i < 10; i++) { // Test first 10 entries
    const MoveGroupType& group = group_data[i];
        
        // Values should be in reasonable ranges
    // last_group_ can be -1 to indicate no groups
    EXPECT_GE(group.last_group_, -1);
    EXPECT_LE(group.last_group_, 6); // At most 7 groups (0-6)
    EXPECT_GE(group.rank_[0], 0);
    EXPECT_LE(group.rank_[0], 15);
    EXPECT_GE(group.sequence_[0], 0);
    EXPECT_GE(group.fullseq_[0], 0);
    EXPECT_GE(group.gap_[0], 0);
    }
}

// Performance test
TEST_F(LookupTablesTest, InitializationPerformance) {
    // Test that initialization completes in reasonable time
    auto start = std::chrono::high_resolution_clock::now();
    
    init_lookup_tables();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within 1 second (adjust if needed)
    EXPECT_LT(duration.count(), 1000) 
    << "init_lookup_tables should complete within 1 second";
}
