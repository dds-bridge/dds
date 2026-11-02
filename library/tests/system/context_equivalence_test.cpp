/// @file context_equivalence_test.cpp
/// @brief Tests for equivalence between legacy and context-based solver APIs.
///
/// Validates that SolverContext and legacy SolveBoard produce identical
/// results for the same input hands and parameters. Also validates that
/// the context-aware calc_dd_table and calc_par paths are equivalent to
/// their non-context counterparts.

#include <gtest/gtest.h>
#include <cstring>

#include <system/memory.hpp>
#include <solver_context/solver_context.hpp>
#include <dds/dds.hpp>

extern Memory memory;

static Deal make_empty_deal()
{
    Deal dl{};
    dl.trump = 0;
    dl.first = 0;
    std::memset(dl.currentTrickSuit, 0, sizeof(dl.currentTrickSuit));
    std::memset(dl.currentTrickRank, 0, sizeof(dl.currentTrickRank));
    std::memset(dl.remainCards, 0, sizeof(dl.remainCards));
    return dl;
}

// Known deal from examples/hands.cpp (hand 0)
// PBN: N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3
static DdTableDeal make_known_deal()
{
        DdTableDeal deal{};
        // Construct using the same rank bitmasks as calc_par_test.cpp.
        // North: S=QJ6, H=K652, D=J85, C=T98
        deal.cards[0][0] = 0x1800 | 0x0040;          // Spades: Q J 6
        deal.cards[0][1] = 0x2000 | 0x0060 | 0x0004; // Hearts: K 6 5 2
        deal.cards[0][2] = 0x0800 | 0x0100 | 0x0020; // Diamonds: J 8 5
        deal.cards[0][3] = 0x0400 | 0x0200 | 0x0100; // Clubs: T 9 8
        // East: S=873, H=J97, D=AT764, C=Q4
        deal.cards[1][0] = 0x0100 | 0x0080 | 0x0008; // Spades: 8 7 3
        deal.cards[1][1] = 0x0800 | 0x0200 | 0x0080; // Hearts: J 9 7
        deal.cards[1][2] = 0x4000 | 0x0400 | 0x0080 | 0x0040 | 0x0010; // Diamonds: A T 7 6 4
        deal.cards[1][3] = 0x1000 | 0x0010;          // Clubs: Q 4
        // South: S=K5, H=T83, D=KQ9, C=A7652
        deal.cards[2][0] = 0x2000 | 0x0020;          // Spades: K 5
        deal.cards[2][1] = 0x0400 | 0x0100 | 0x0008; // Hearts: T 8 3
        deal.cards[2][2] = 0x2000 | 0x1000 | 0x0200; // Diamonds: K Q 9
        deal.cards[2][3] = 0x4000 | 0x0080 | 0x0040 | 0x0020 | 0x0004; // Clubs: A 7 6 5 2
        // West: S=AT942, H=AQ4, D=32, C=KJ3
        deal.cards[3][0] = 0x4000 | 0x0400 | 0x0200 | 0x0010 | 0x0004; // Spades: A T 9 4 2
        deal.cards[3][1] = 0x4000 | 0x1000 | 0x0010; // Hearts: A Q 4
        deal.cards[3][2] = 0x0008 | 0x0004;          // Diamonds: 3 2
        deal.cards[3][3] = 0x2000 | 0x0800 | 0x0008; // Clubs: K J 3
        return deal;
}

TEST(SystemContextEquivalence, LegacyVsContextReturnCode)
{
    // Ensure DDS system and thread-local memory are initialized
    InitializeStaticMemory();
    const int thr = 0;
    FutureTricks ft_legacy{};
    FutureTricks ft_ctx{};
    Deal dl = make_empty_deal();

    const int r_legacy = SolveBoard(dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft_legacy, thr);

    // Construct a SolverContext-owned ThreadData for the context-based call.
    SolverContext ctx;
    const int r_ctx = solve_board(ctx, dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft_ctx);

    EXPECT_EQ(r_legacy, r_ctx) << "Legacy and context return codes should match";
}

// Verify calc_dd_table context overload produces same results as non-context overload
TEST(SystemContextEquivalence, CalcDDTableContextVsNonContext)
{
        DdTableDeal deal = make_known_deal();

        DdTableResults table_no_ctx{};
        int res1 = calc_dd_table(deal, &table_no_ctx);
        ASSERT_EQ(res1, RETURN_NO_FAULT) << "Non-context calc_dd_table failed";

        SolverContext ctx;
        DdTableResults table_with_ctx{};
        int res2 = calc_dd_table(ctx, deal, &table_with_ctx);
        ASSERT_EQ(res2, RETURN_NO_FAULT) << "Context calc_dd_table failed";

        // All trick counts must be identical
        for (int strain = 0; strain < DDS_STRAINS; strain++) {
                for (int hand = 0; hand < DDS_HANDS; hand++) {
                        EXPECT_EQ(table_no_ctx.res_table[strain][hand],
                                            table_with_ctx.res_table[strain][hand])
                                << "Mismatch at strain=" << strain << " hand=" << hand;
                }
        }
}

// Verify context reuse across multiple calc_dd_table calls produces consistent results
TEST(SystemContextEquivalence, CalcDDTableContextReuse)
{
        DdTableDeal deal = make_known_deal();
        SolverContext ctx;

        DdTableResults first_result{};
        ASSERT_EQ(calc_dd_table(ctx, deal, &first_result), RETURN_NO_FAULT);

        // Call again with same context — must produce identical results
        DdTableResults second_result{};
        ASSERT_EQ(calc_dd_table(ctx, deal, &second_result), RETURN_NO_FAULT);

        for (int strain = 0; strain < DDS_STRAINS; strain++) {
                for (int hand = 0; hand < DDS_HANDS; hand++) {
                        EXPECT_EQ(first_result.res_table[strain][hand],
                                            second_result.res_table[strain][hand])
                                << "Context reuse changed result at strain=" << strain << " hand=" << hand;
                }
        }
}

// Verify calc_par context overload produces same results as non-context overload
TEST(SystemContextEquivalence, CalcParContextVsNonContext)
{
        DdTableDeal deal = make_known_deal();

        DdTableResults table_no_ctx{};
        ParResults par_no_ctx{};
        int res1 = calc_par(deal, /*vulnerable=*/0, &table_no_ctx, &par_no_ctx);
        ASSERT_EQ(res1, RETURN_NO_FAULT) << "Non-context calc_par failed";

        SolverContext ctx;
        DdTableResults table_with_ctx{};
        ParResults par_with_ctx{};
        int res2 = calc_par(ctx, deal, /*vulnerable=*/0, &table_with_ctx, &par_with_ctx);
        ASSERT_EQ(res2, RETURN_NO_FAULT) << "Context calc_par failed";

        EXPECT_STREQ(par_no_ctx.par_score[0], par_with_ctx.par_score[0])
                << "NS par scores differ between context and non-context paths";
        EXPECT_STREQ(par_no_ctx.par_score[1], par_with_ctx.par_score[1])
                << "EW par scores differ between context and non-context paths";
}

// Verify the context-aware analyse_play overload produces results identical
// to the legacy AnalysePlayBin for the same deal and play.
TEST(SystemContextEquivalence, AnalysePlayContextVsLegacy)
{
        // North leads the spade 6 against a NT contract on the known deal.
        DdTableDeal table_deal = make_known_deal();
        Deal dl{};
        dl.trump = 4;  // No trump
        dl.first = 0;  // North leads
        std::memset(dl.currentTrickSuit, 0, sizeof(dl.currentTrickSuit));
        std::memset(dl.currentTrickRank, 0, sizeof(dl.currentTrickRank));
        for (int h = 0; h < DDS_HANDS; h++)
                for (int s = 0; s < DDS_SUITS; s++)
                        dl.remainCards[h][s] = table_deal.cards[h][s];

        PlayTraceBin play{};
        play.number = 1;
        play.suit[0] = 0;   // Spades
        play.rank[0] = 6;   // 6

        SolvedPlay solved_legacy{};
        const int r_legacy = AnalysePlayBin(dl, play, &solved_legacy, /*thrId=*/0);
        ASSERT_EQ(r_legacy, RETURN_NO_FAULT) << "Legacy AnalysePlayBin failed";

        SolverContext ctx;
        SolvedPlay solved_ctx{};
        const int r_ctx = analyse_play(ctx, dl, play, &solved_ctx);
        ASSERT_EQ(r_ctx, RETURN_NO_FAULT) << "Context analyse_play failed";

        EXPECT_EQ(solved_legacy.number, solved_ctx.number);
        for (int i = 0; i <= solved_legacy.number; i++) {
                EXPECT_EQ(solved_legacy.tricks[i], solved_ctx.tricks[i])
                        << "Mismatch at ply " << i;
        }
}

// Verify context reuse across multiple analyse_play calls produces results
// consistent with independent (single-use) calls.
TEST(SystemContextEquivalence, AnalysePlayContextReuse)
{
        DdTableDeal table_deal = make_known_deal();
        Deal dl{};
        dl.trump = 4;
        dl.first = 0;
        std::memset(dl.currentTrickSuit, 0, sizeof(dl.currentTrickSuit));
        std::memset(dl.currentTrickRank, 0, sizeof(dl.currentTrickRank));
        for (int h = 0; h < DDS_HANDS; h++)
                for (int s = 0; s < DDS_SUITS; s++)
                        dl.remainCards[h][s] = table_deal.cards[h][s];

        PlayTraceBin play{};
        play.number = 1;
        play.suit[0] = 0;
        play.rank[0] = 6;

        SolverContext ctx;
        SolvedPlay first_result{};
        ASSERT_EQ(analyse_play(ctx, dl, play, &first_result), RETURN_NO_FAULT);

        SolvedPlay second_result{};
        ASSERT_EQ(analyse_play(ctx, dl, play, &second_result), RETURN_NO_FAULT);

        EXPECT_EQ(first_result.number, second_result.number);
        for (int i = 0; i <= first_result.number; i++) {
                EXPECT_EQ(first_result.tricks[i], second_result.tricks[i])
                        << "Context reuse changed result at ply " << i;
        }
}
