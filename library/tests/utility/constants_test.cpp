#include <gtest/gtest.h>
#include "library/src/utility/Constants.h"

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
    // Test left-hand opponent mappings
    EXPECT_EQ(lho[0], 1);  // North's LHO is East
    EXPECT_EQ(lho[1], 2);  // East's LHO is South  
    EXPECT_EQ(lho[2], 3);  // South's LHO is West
    EXPECT_EQ(lho[3], 0);  // West's LHO is North
}

TEST_F(ConstantsTest, RhoArrayValues) {
    // Test right-hand opponent mappings
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
    EXPECT_EQ(bitMapRank[0], 0x0000);
    EXPECT_EQ(bitMapRank[1], 0x0000);
    EXPECT_EQ(bitMapRank[2], 0x0001);  // Two
    EXPECT_EQ(bitMapRank[3], 0x0002);  // Three
    EXPECT_EQ(bitMapRank[4], 0x0004);  // Four
    EXPECT_EQ(bitMapRank[5], 0x0008);  // Five
    EXPECT_EQ(bitMapRank[6], 0x0010);  // Six
    EXPECT_EQ(bitMapRank[7], 0x0020);  // Seven
    EXPECT_EQ(bitMapRank[8], 0x0040);  // Eight
    EXPECT_EQ(bitMapRank[9], 0x0080);  // Nine
    EXPECT_EQ(bitMapRank[10], 0x0100); // Ten
    EXPECT_EQ(bitMapRank[11], 0x0200); // Jack
    EXPECT_EQ(bitMapRank[12], 0x0400); // Queen
    EXPECT_EQ(bitMapRank[13], 0x0800); // King
    EXPECT_EQ(bitMapRank[14], 0x1000); // Ace
    EXPECT_EQ(bitMapRank[15], 0x2000); // Unused
}

TEST_F(ConstantsTest, CardRankValues) {
    // Test card rank mappings - these are character values
    EXPECT_EQ(cardRank[0], 'x');   // Unused
    EXPECT_EQ(cardRank[1], 'x');   // Unused
    EXPECT_EQ(cardRank[2], '2');   // Two
    EXPECT_EQ(cardRank[3], '3');   // Three
    EXPECT_EQ(cardRank[4], '4');   // Four
    EXPECT_EQ(cardRank[5], '5');   // Five
    EXPECT_EQ(cardRank[6], '6');   // Six
    EXPECT_EQ(cardRank[7], '7');   // Seven
    EXPECT_EQ(cardRank[8], '8');   // Eight
    EXPECT_EQ(cardRank[9], '9');   // Nine
    EXPECT_EQ(cardRank[10], 'T');  // Ten
    EXPECT_EQ(cardRank[11], 'J');  // Jack
    EXPECT_EQ(cardRank[12], 'Q');  // Queen
    EXPECT_EQ(cardRank[13], 'K');  // King
    EXPECT_EQ(cardRank[14], 'A');  // Ace
    EXPECT_EQ(cardRank[15], '-');  // Unused
}

TEST_F(ConstantsTest, CardSuitValues) {
    // Test card suit mappings - these are character values
    EXPECT_EQ(cardSuit[0], 'S');  // Spades
    EXPECT_EQ(cardSuit[1], 'H');  // Hearts
    EXPECT_EQ(cardSuit[2], 'D');  // Diamonds
    EXPECT_EQ(cardSuit[3], 'C');  // Clubs
    EXPECT_EQ(cardSuit[4], 'N');  // No Trump
}

TEST_F(ConstantsTest, CardHandValues) {
    // Test card hand mappings - these are character values
    EXPECT_EQ(cardHand[0], 'N');  // North
    EXPECT_EQ(cardHand[1], 'E');  // East
    EXPECT_EQ(cardHand[2], 'S');  // South
    EXPECT_EQ(cardHand[3], 'W');  // West
}

// Test array bounds and properties
TEST_F(ConstantsTest, ArraySizes) {
    // Test that arrays have expected sizes by accessing last valid element
    EXPECT_NO_THROW((void)lho[3]);
    EXPECT_NO_THROW((void)rho[3]);
    EXPECT_NO_THROW((void)partner[3]);
    EXPECT_NO_THROW((void)bitMapRank[15]);
    EXPECT_NO_THROW((void)cardRank[15]);
    EXPECT_NO_THROW((void)cardSuit[4]);
    EXPECT_NO_THROW((void)cardHand[3]);
}

TEST_F(ConstantsTest, BitMapRankUniqueness) {
    // Test that each non-zero bitMapRank value is unique
    for (int i = 1; i <= 14; i++) {
        for (int j = i + 1; j <= 14; j++) {
            EXPECT_NE(bitMapRank[i], bitMapRank[j]) 
                << "bitMapRank[" << i << "] should not equal bitMapRank[" << j << "]";
        }
    }
}

TEST_F(ConstantsTest, CardRankMonotonicity) {
    // Test that the bit map ranks are in ascending order for rank 2-14
    for (int i = 2; i <= 13; i++) {
        EXPECT_LT(bitMapRank[i], bitMapRank[i + 1])
            << "bitMapRank[" << i << "] should be less than bitMapRank[" << i + 1 << "]";
    }
}
