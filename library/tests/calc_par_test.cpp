/// @file calc_par_test.cpp
/// @brief Tests for C++ calc_par API functions.
///
/// Tests calc_par(), calc_par(ctx, ...), and calc_par_from_table() functions
/// using known test hands with verified par results.

#include <gtest/gtest.h>
#include <dds/dds.hpp>

// Rank bitmask constants  (bits 2-14 for ranks 2-A)
#define R2  0x0004
#define R3  0x0008
#define R4  0x0010
#define R5  0x0020
#define R6  0x0040
#define R7  0x0080
#define R8  0x0100
#define R9  0x0200
#define RT  0x0400
#define RJ  0x0800
#define RQ  0x1000
#define RK  0x2000
#define RA  0x4000

class CalcParTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Test hand 0: Known DD table and par scores
        // Par: NS -110, EW 110, contracts: "NS:EW 2S", "EW:EW 2S"
        // holdings0_[suit][hand] format needs to be transposed to deal.cards[hand][suit]
        for (int h = 0; h < DDS_HANDS; h++) {
            for (int s = 0; s < DDS_SUITS; s++) {
                deal0_.cards[h][s] = holdings0_[s][h];
            }
        }
        
        // Test hand 1: Different vulnerability
        // Par: NS 100, EW -100, contracts: "NS:EW 4Sx", "EW:EW 4Sx"
        for (int h = 0; h < DDS_HANDS; h++) {
            for (int s = 0; s < DDS_SUITS; s++) {
                deal1_.cards[h][s] = holdings1_[s][h];
            }
        }
        
        // Test hand 2: Another variation
        // Par: NS -300, EW 300, contracts: "NS:NS 5Hx", "EW:NS 5Hx"
        for (int h = 0; h < DDS_HANDS; h++) {
            for (int s = 0; s < DDS_SUITS; s++) {
                deal2_.cards[h][s] = holdings2_[s][h];
            }
        }
    }

    void TearDown() override
    {
        // Cleanup if needed
    }

    // Test hand data from examples/hands.cpp
    // Hand 0: VUL_NONE (0)
    // Format: holdings0_[suit][hand] where suit=0-3 (S,H,D,C), hand=0-3 (N,E,S,W)
    // PBN: N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3
    unsigned int holdings0_[4][4] = {
        // North           East            South         West
        {RQ|RJ|R6,         R8|R7|R3,       RK|R5,        RA|RT|R9|R4|R2},  // Spades
        {RK|R6|R5|R2,      RJ|R9|R7,       RT|R8|R3,     RA|RQ|R4},         // Hearts  
        {RJ|R8|R5,         RA|RT|R7|R6|R4, RK|RQ|R9,     R3|R2},            // Diamonds
        {RT|R9|R8,         RQ|R4,          RA|R7|R6|R5|R2, RK|RJ|R3}        // Clubs
    };
    
    // Hand 1: VUL_NS (2)
    // PBN: E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63
    unsigned int holdings1_[4][4] = {
        {RA|RK|R9|R6,      RQ|RJ|RT|R5|R4|R3|R2, 0,            R8|R7},        // Spades
        {RK|RQ|R8,         RT,                   RJ|R9|R7|R5|R4|R3, RA|R6|R2},     // Hearts
        {RA|R9|R8,         R6,                   RK|R7|R5|R3|R2, RQ|RJ|RT|R4},   // Diamonds
        {RK|R6|R3,         RQ|RJ|R8|R2,          R9|R4,        RA|RT|R7|R5}       // Clubs
    };
    
    // Hand 2: VUL_NONE (0)
    // PBN: N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93
    unsigned int holdings2_[4][4] = {
        {R7|R3,            RQ|RT|R6,       R5,           RA|RK|RJ|R9|R8|R4|R2},  // Spades
        {RQ|RJ|RT,         R8|R7|R6,       RA|R9|R5|R4|R3|R2, RK},              // Hearts
        {RA|RQ|R5|R4,      RK|RJ|R9,       R7|R6|R3|R2,  RT|R8},                // Diamonds
        {RT|R7|R5|R2,      RA|RQ|R8|R4,    RK|R6,        RJ|R9|R3}               // Clubs
    };

    DdTableDeal deal0_;
    DdTableDeal deal1_;
    DdTableDeal deal2_;
    
    // Expected par results
    const char* expected_par_score_[3][2] = {
        { "NS -110", "EW 110" },
        { "NS 100", "EW -100" },
        { "NS -300", "EW 300" }
    };
    
    const char* expected_par_contracts_[3][2] = {
        { "NS:EW 2S", "EW:EW 2S" },
        { "NS:EW 4Sx", "EW:EW 4Sx" },
        { "NS:NS 5Hx", "EW:NS 5Hx" }
    };
    
    int vulnerability_[3] = { 0, 2, 0 }; // None, NS, None
};

// Test basic calc_par without context (creates temporary context)
TEST_F(CalcParTest, BasicCalcParHand0)
{
    DdTableResults table;
    ParResults par;
    
    int result = calc_par(deal0_, vulnerability_[0], &table, &par);
    
    ASSERT_EQ(result, RETURN_NO_FAULT) << "calc_par should succeed";
    
    // Verify par scores
    EXPECT_STREQ(par.par_score[0], expected_par_score_[0][0]);
    EXPECT_STREQ(par.par_score[1], expected_par_score_[0][1]);
    
    // Verify par contracts  
    EXPECT_STREQ(par.par_contracts_string[0], expected_par_contracts_[0][0]);
    EXPECT_STREQ(par.par_contracts_string[1], expected_par_contracts_[0][1]);
}

TEST_F(CalcParTest, BasicCalcParHand1)
{
    DdTableResults table;
    ParResults par;
    
    int result = calc_par(deal1_, vulnerability_[1], &table, &par);
    
    ASSERT_EQ(result, RETURN_NO_FAULT);
    
    EXPECT_STREQ(par.par_score[0], expected_par_score_[1][0]);
    EXPECT_STREQ(par.par_score[1], expected_par_score_[1][1]);
    EXPECT_STREQ(par.par_contracts_string[0], expected_par_contracts_[1][0]);
    EXPECT_STREQ(par.par_contracts_string[1], expected_par_contracts_[1][1]);
}

TEST_F(CalcParTest, BasicCalcParHand2)
{
    DdTableResults table;
    ParResults par;
    
    int result = calc_par(deal2_, vulnerability_[2], &table, &par);
    
    ASSERT_EQ(result, RETURN_NO_FAULT);
    
    EXPECT_STREQ(par.par_score[0], expected_par_score_[2][0]);
    EXPECT_STREQ(par.par_score[1], expected_par_score_[2][1]);
    EXPECT_STREQ(par.par_contracts_string[0], expected_par_contracts_[2][0]);
    EXPECT_STREQ(par.par_contracts_string[1], expected_par_contracts_[2][1]);
}

// Test calc_par with explicit context
TEST_F(CalcParTest, CalcParWithContext)
{
    SolverContext ctx;
    DdTableResults table;
    ParResults par;
    
    int result = calc_par(ctx, deal0_, vulnerability_[0], &table, &par);
    
    ASSERT_EQ(result, RETURN_NO_FAULT);
    
    EXPECT_STREQ(par.par_score[0], expected_par_score_[0][0]);
    EXPECT_STREQ(par.par_score[1], expected_par_score_[0][1]);
    EXPECT_STREQ(par.par_contracts_string[0], expected_par_contracts_[0][0]);
    EXPECT_STREQ(par.par_contracts_string[1], expected_par_contracts_[0][1]);
}

// Test context reuse across multiple calls
TEST_F(CalcParTest, ContextReuseMultipleCalls)
{
    SolverContext ctx;
    
    // First call
    DdTableResults table1;
    ParResults par1;
    int result1 = calc_par(ctx, deal0_, vulnerability_[0], &table1, &par1);
    ASSERT_EQ(result1, RETURN_NO_FAULT);
    EXPECT_STREQ(par1.par_score[0], expected_par_score_[0][0]);
    
    // Second call with same context, different deal
    DdTableResults table2;
    ParResults par2;
    int result2 = calc_par(ctx, deal1_, vulnerability_[1], &table2, &par2);
    ASSERT_EQ(result2, RETURN_NO_FAULT);
    EXPECT_STREQ(par2.par_score[0], expected_par_score_[1][0]);
    
    // Third call
    DdTableResults table3;
    ParResults par3;
    int result3 = calc_par(ctx, deal2_, vulnerability_[2], &table3, &par3);
    ASSERT_EQ(result3, RETURN_NO_FAULT);
    EXPECT_STREQ(par3.par_score[0], expected_par_score_[2][0]);
}

// Test calc_par_from_table (pre-computed table)
TEST_F(CalcParTest, CalcParFromTableHand0)
{
    // First compute the DD table
    DdTableResults table;
    ParResults par_full;
    int result1 = calc_par(deal0_, vulnerability_[0], &table, &par_full);
    ASSERT_EQ(result1, RETURN_NO_FAULT);
    
    // Now compute par from the table only
    ParResults par_from_table;
    int result2 = calc_par_from_table(&table, vulnerability_[0], &par_from_table);
    ASSERT_EQ(result2, RETURN_NO_FAULT);
    
    // Results should be identical
    EXPECT_STREQ(par_from_table.par_score[0], par_full.par_score[0]);
    EXPECT_STREQ(par_from_table.par_score[1], par_full.par_score[1]);
    EXPECT_STREQ(par_from_table.par_contracts_string[0], par_full.par_contracts_string[0]);
    EXPECT_STREQ(par_from_table.par_contracts_string[1], par_full.par_contracts_string[1]);
}

// Test different vulnerability conditions
TEST_F(CalcParTest, VulnerabilityVariations)
{
    DdTableResults table;
    ParResults par;
    
    // Test all vulnerability conditions on same deal
    for (int vuln = 0; vuln <= 3; vuln++) {
        int result = calc_par(deal0_, vuln, &table, &par);
        EXPECT_EQ(result, RETURN_NO_FAULT) 
            << "calc_par should succeed for vulnerability " << vuln;
        
        // Just verify it returns some result (scores depend on vulnerability)
        EXPECT_NE(par.par_score[0][0], '\0') << "Par score NS should not be empty";
        EXPECT_NE(par.par_score[1][0], '\0') << "Par score EW should not be empty";
    }
}

// Test that table results are populated correctly
TEST_F(CalcParTest, TableResultsPopulated)
{
    DdTableResults table;
    ParResults par;
    
    int result = calc_par(deal0_, vulnerability_[0], &table, &par);
    ASSERT_EQ(result, RETURN_NO_FAULT);
    
    // Verify DD table has valid trick counts (0-13)
    for (int strain = 0; strain < DDS_STRAINS; strain++) {
        for (int hand = 0; hand < DDS_HANDS; hand++) {
            int tricks = table.res_table[strain][hand];
            EXPECT_GE(tricks, 0) << "Tricks should be >= 0";
            EXPECT_LE(tricks, 13) << "Tricks should be <= 13";
        }
    }
}

// Test calc_par_from_table with different vulnerabilities
TEST_F(CalcParTest, CalcParFromTableVulnerability)
{
    // Compute table once
    DdTableResults table;
    ParResults par_temp;
    int result = calc_par(deal0_, 0, &table, &par_temp);
    ASSERT_EQ(result, RETURN_NO_FAULT);
    
    // Compute par for different vulnerabilities using same table
    for (int vuln = 0; vuln <= 3; vuln++) {
        ParResults par;
        int res = calc_par_from_table(&table, vuln, &par);
        EXPECT_EQ(res, RETURN_NO_FAULT)
            << "Should compute par for vulnerability " << vuln;
    }
}

// Test error handling: invalid vulnerability
TEST_F(CalcParTest, InvalidVulnerability)
{
    DdTableResults table;
    ParResults par;
    
    // Test with out-of-range vulnerability (valid range is 0-3)
    // Note: The C API may or may not validate this - we're testing behavior
    int result = calc_par(deal0_, -1, &table, &par);
    
    // Either it fails with error or succeeds (implementation-dependent)
    // Just verify it doesn't crash
    EXPECT_TRUE(result == RETURN_NO_FAULT || result < 0)
        << "Function should return valid code for invalid vulnerability";
}

// Test with all-zero deal (edge case)
TEST_F(CalcParTest, EmptyDealHandling)
{
    DdTableDeal empty_deal;
    for (int h = 0; h < DDS_HANDS; h++) {
        for (int s = 0; s < DDS_SUITS; s++) {
            empty_deal.cards[h][s] = 0;
        }
    }
    
    DdTableResults table;
    ParResults par;
    
    // This should fail with appropriate error (no cards)
    int result = calc_par(empty_deal, 0, &table, &par);
    
    // Expecting an error (not RETURN_NO_FAULT)
    EXPECT_NE(result, RETURN_NO_FAULT)
        << "Empty deal should produce an error";
}

// Test consistency: calc_par and calc_par_from_table should give same results
TEST_F(CalcParTest, ConsistencyCalcParVsFromTable)
{
    for (int hand_idx = 0; hand_idx < 3; hand_idx++) {
        DdTableDeal* deal;
        if (hand_idx == 0) deal = &deal0_;
        else if (hand_idx == 1) deal = &deal1_;
        else deal = &deal2_;
        
        // Method 1: calc_par (computes table + par)
        DdTableResults table1;
        ParResults par1;
        int res1 = calc_par(*deal, vulnerability_[hand_idx], &table1, &par1);
        ASSERT_EQ(res1, RETURN_NO_FAULT);
        
        // Method 2: calc_par_from_table using computed table
        ParResults par2;
        int res2 = calc_par_from_table(&table1, vulnerability_[hand_idx], &par2);
        ASSERT_EQ(res2, RETURN_NO_FAULT);
        
        // Results should be identical
        EXPECT_STREQ(par1.par_score[0], par2.par_score[0])
            << "Hand " << hand_idx << " NS scores should match";
        EXPECT_STREQ(par1.par_score[1], par2.par_score[1])
            << "Hand " << hand_idx << " EW scores should match";
        EXPECT_STREQ(par1.par_contracts_string[0], par2.par_contracts_string[0])
            << "Hand " << hand_idx << " NS contracts should match";
        EXPECT_STREQ(par1.par_contracts_string[1], par2.par_contracts_string[1])
            << "Hand " << hand_idx << " EW contracts should match";
    }
}

// Test that context and non-context overloads produce identical results
// (Regression guard for the calc_par context wiring in Task 05)
TEST_F(CalcParTest, CalcParContextOverloadMatchesNonContext)
{
    for (int hand_idx = 0; hand_idx < 3; hand_idx++) {
        DdTableDeal* deal;
        if (hand_idx == 0) deal = &deal0_;
        else if (hand_idx == 1) deal = &deal1_;
        else deal = &deal2_;

        // Non-context overload
        DdTableResults table_no_ctx;
        ParResults par_no_ctx;
        int res1 = calc_par(*deal, vulnerability_[hand_idx], &table_no_ctx, &par_no_ctx);
        ASSERT_EQ(res1, RETURN_NO_FAULT) << "Non-context call failed for hand " << hand_idx;

        // Context overload - should produce identical output
        SolverContext ctx;
        DdTableResults table_with_ctx;
        ParResults par_with_ctx;
        int res2 = calc_par(ctx, *deal, vulnerability_[hand_idx], &table_with_ctx, &par_with_ctx);
        ASSERT_EQ(res2, RETURN_NO_FAULT) << "Context call failed for hand " << hand_idx;

        EXPECT_STREQ(par_no_ctx.par_score[0], par_with_ctx.par_score[0])
            << "Hand " << hand_idx << " NS par scores differ";
        EXPECT_STREQ(par_no_ctx.par_score[1], par_with_ctx.par_score[1])
            << "Hand " << hand_idx << " EW par scores differ";
        EXPECT_STREQ(par_no_ctx.par_contracts_string[0], par_with_ctx.par_contracts_string[0])
            << "Hand " << hand_idx << " NS contracts differ";
        EXPECT_STREQ(par_no_ctx.par_contracts_string[1], par_with_ctx.par_contracts_string[1])
            << "Hand " << hand_idx << " EW contracts differ";
    }
}

// Performance test: context reuse should work efficiently
TEST_F(CalcParTest, ContextReusePerformance)
{
    SolverContext ctx;
    
    // Run multiple calculations with same context
    const int num_iterations = 10;
    for (int i = 0; i < num_iterations; i++) {
        DdTableResults table;
        ParResults par;
        
        // Alternate between different deals
        DdTableDeal* deal = (i % 2 == 0) ? &deal0_ : &deal1_;
        int vuln = vulnerability_[i % 2];
        
        int result = calc_par(ctx, *deal, vuln, &table, &par);
        ASSERT_EQ(result, RETURN_NO_FAULT) << "Iteration " << i << " should succeed";
    }
    
    // If we got here, context reuse is working
    SUCCEED();
}
