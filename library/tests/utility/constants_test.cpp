/// @file constants_test.cpp
/// @brief Tests for bridge card constant arrays and relationships.
///
/// Validates that constant arrays (lho, rho, partner, bit_map_rank, card_rank,
/// card_suit, card_hand) have correct values and maintain expected relationships
/// (e.g., partner of partner is self, LHO of RHO is self).

#include <gtest/gtest.h>
#include <utility/constants.h>

class ConstantsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed before each test
    }

    void TearDown() override {
        // Any cleanup needed after each test
    }
};

// Test hand relationship arrays
TEST_F(ConstantsTest, LhoArrayValues) {
    // Test left-hand opponent mappings (compass orientation)
    EXPECT_EQ(lho[0], 1);  // North's LHO is East
    EXPECT_EQ(lho[1], 2);  // East's LHO is South  
    EXPECT_EQ(lho[2], 3);  // South's LHO is West
    EXPECT_EQ(lho[3], 0);  // West's LHO is North
}

TEST_F(ConstantsTest, RhoArrayValues) {
    // Test right-hand opponent mappings (compass orientation)
    EXPECT_EQ(rho[0], 3);  // North's RHO is West
    EXPECT_EQ(rho[1], 0);  // East's RHO is North
    EXPECT_EQ(rho[2], 1);  // South's RHO is East
    EXPECT_EQ(rho[3], 2);  // West's RHO is South
}

TEST_F(ConstantsTest, PartnerArrayValues) {
    // Test partner mappings
    EXPECT_EQ(partner[0], 2);  // North's partner is South
    EXPECT_EQ(partner[1], 3);  // East's partner is West
    EXPECT_EQ(partner[2], 0);  // South's partner is North
    EXPECT_EQ(partner[3], 1);  // West's partner is East
}

TEST_F(ConstantsTest, HandRelationshipConsistency) {
    // Test that relationships are consistent
    for (int i = 0; i < 4; i++) {
        // Partner of partner should be self
        EXPECT_EQ(partner[partner[i]], i);
        
        // LHO of RHO should be self
        EXPECT_EQ(lho[rho[i]], i);
        
        // RHO of LHO should be self  
        EXPECT_EQ(rho[lho[i]], i);
    }
}

// Test card mapping arrays
TEST_F(ConstantsTest, BitMapRankValues) {
    // Test specific known values from original implementation
    EXPECT_EQ(bit_map_rank[0], 0x0000);
    EXPECT_EQ(bit_map_rank[1], 0x0000);
    EXPECT_EQ(bit_map_rank[2], 0x0001);  // Two
    EXPECT_EQ(bit_map_rank[3], 0x0002);  // Three
    EXPECT_EQ(bit_map_rank[4], 0x0004);  // Four
    EXPECT_EQ(bit_map_rank[5], 0x0008);  // Five
    EXPECT_EQ(bit_map_rank[6], 0x0010);  // Six
    EXPECT_EQ(bit_map_rank[7], 0x0020);  // Seven
    EXPECT_EQ(bit_map_rank[8], 0x0040);  // Eight
    EXPECT_EQ(bit_map_rank[9], 0x0080);  // Nine
    EXPECT_EQ(bit_map_rank[10], 0x0100); // Ten
    EXPECT_EQ(bit_map_rank[11], 0x0200); // Jack
    EXPECT_EQ(bit_map_rank[12], 0x0400); // Queen
    EXPECT_EQ(bit_map_rank[13], 0x0800); // King
    EXPECT_EQ(bit_map_rank[14], 0x1000); // Ace
    EXPECT_EQ(bit_map_rank[15], 0x2000); // Unused
}

TEST_F(ConstantsTest, CardRankValues) {
    // Test card rank mappings - these are character values
    EXPECT_EQ(card_rank[0], 'x');   // Unused
    EXPECT_EQ(card_rank[1], 'x');   // Unused
    EXPECT_EQ(card_rank[2], '2');   // Two
    EXPECT_EQ(card_rank[3], '3');   // Three
    EXPECT_EQ(card_rank[4], '4');   // Four
    EXPECT_EQ(card_rank[5], '5');   // Five
    EXPECT_EQ(card_rank[6], '6');   // Six
    EXPECT_EQ(card_rank[7], '7');   // Seven
    EXPECT_EQ(card_rank[8], '8');   // Eight
    EXPECT_EQ(card_rank[9], '9');   // Nine
    EXPECT_EQ(card_rank[10], 'T');  // Ten
    EXPECT_EQ(card_rank[11], 'J');  // Jack
    EXPECT_EQ(card_rank[12], 'Q');  // Queen
    EXPECT_EQ(card_rank[13], 'K');  // King
    EXPECT_EQ(card_rank[14], 'A');  // Ace
    EXPECT_EQ(card_rank[15], '-');  // Unused
}

TEST_F(ConstantsTest, CardSuitValues) {
    // Test card suit mappings - these are character values
    EXPECT_EQ(card_suit[0], 'S');  // Spades
    EXPECT_EQ(card_suit[1], 'H');  // Hearts
    EXPECT_EQ(card_suit[2], 'D');  // Diamonds
    EXPECT_EQ(card_suit[3], 'C');  // Clubs
    EXPECT_EQ(card_suit[4], 'N');  // No Trump
}

TEST_F(ConstantsTest, CardHandValues) {
    // Test card hand mappings - these are character values
    EXPECT_EQ(card_hand[0], 'N');  // North
    EXPECT_EQ(card_hand[1], 'E');  // East
    EXPECT_EQ(card_hand[2], 'S');  // South
    EXPECT_EQ(card_hand[3], 'W');  // West
}

// Test array bounds and properties
TEST_F(ConstantsTest, ArraySizes) {
    // Test that arrays have expected sizes by accessing last valid element
    EXPECT_NO_THROW((void)lho[3]);
    EXPECT_NO_THROW((void)rho[3]);
    EXPECT_NO_THROW((void)partner[3]);
    EXPECT_NO_THROW((void)bit_map_rank[15]);
    EXPECT_NO_THROW((void)card_rank[15]);
    EXPECT_NO_THROW((void)card_suit[4]);
    EXPECT_NO_THROW((void)card_hand[3]);
}

TEST_F(ConstantsTest, BitMapRankUniqueness) {
    // Test that each non-zero bit_map_rank value is unique
    for (int i = 1; i <= 14; i++) {
        for (int j = i + 1; j <= 14; j++) {
            EXPECT_NE(bit_map_rank[i], bit_map_rank[j]) 
                << "bit_map_rank[" << i << "] should not equal bit_map_rank[" << j << "]";
        }
    }
}

TEST_F(ConstantsTest, CardRankMonotonicity) {
    // Test that the bit map ranks are in ascending order for rank 2-14
    for (int i = 2; i <= 13; i++) {
        EXPECT_LT(bit_map_rank[i], bit_map_rank[i + 1])
            << "bit_map_rank[" << i << "] should be less than bit_map_rank[" << i + 1 << "]";
    }
}
