/*
   DDS, a bridge double dummy solver.

   Tests the pure-C ABI shim through its own boundary.

   These call dds_c_* rather than the reference-taking dds_* functions on
   purpose: the null guards and the catch-all wrappers exist only in the shim,
   so exercising the C++ API directly would bypass exactly the code under test.

   The reference board matches jni/java/org/dds/ffm/DdsSmokeTest.java so the
   JVM, .NET, and C++ bindings all agree on one fixture.

   See LICENSE and README.
*/

#include <gtest/gtest.h>

#include <cstring>

#include <api/dds_c_api.h>

namespace {

// Full 13-card holding bitmask (ranks 2..A), matching the Java/Python fixtures.
constexpr unsigned int kFullSuit = 0x7FFCU;

// The reference board: North holds all spades, East all hearts, South all
// diamonds, West all clubs. With spades trump and North to lead, North/South
// take all 13 tricks.
constexpr int kExpectedTricks = 13;

// res_table[strain][hand], flattened 5 strains x 4 hands. Cross-checked against
// the JVM binding's EXPECTED_DD_TABLE.
constexpr int kExpectedDdTable[DDS_STRAINS][DDS_HANDS] = {
    {13, 0, 13, 0},   // spades
    {0, 13, 0, 13},   // hearts
    {13, 0, 13, 0},   // diamonds
    {0, 13, 0, 13},   // clubs
    {0, 0, 0, 0},     // no-trump
};

// The same board in PBN: <spades>.<hearts>.<diamonds>.<clubs> per hand.
constexpr const char* kReferencePbn =
    "N:AKQJT98765432... .AKQJT98765432.. ..AKQJT98765432. ...AKQJT98765432";

struct Deal MakeReferenceDeal()
{
    struct Deal dl = {};   // value-initialize; the shim does not zero for us
    dl.trump = 0;          // spades
    dl.first = 0;          // North leads
    dl.remainCards[0][0] = kFullSuit;   // North spades
    dl.remainCards[1][1] = kFullSuit;   // East hearts
    dl.remainCards[2][2] = kFullSuit;   // South diamonds
    dl.remainCards[3][3] = kFullSuit;   // West clubs
    return dl;
}

struct DdTableDeal MakeReferenceTableDeal()
{
    struct DdTableDeal deal = {};
    deal.cards[0][0] = kFullSuit;
    deal.cards[1][1] = kFullSuit;
    deal.cards[2][2] = kFullSuit;
    deal.cards[3][3] = kFullSuit;
    return deal;
}

// Solve the reference board on ctx and return the trick count.
int SolveReference(DDS_C_SOLVER_CTX ctx)
{
    const struct Deal dl = MakeReferenceDeal();
    struct FutureTricks fut = {};
    const int rc = dds_c_solve_board(ctx, &dl, -1, 1, 1, &fut);
    EXPECT_EQ(rc, RETURN_NO_FAULT);
    return fut.score[0];
}

// ---------------------------------------------------------------------------
// Null-handle safety. Every entry point must reject a null handle rather than
// dereferencing it: the int-returning ones with RETURN_UNKNOWN_FAULT, the
// void-returning ones by returning quietly.
// ---------------------------------------------------------------------------

TEST(DdsCApiNullHandle, IntReturningEntryPointsFailFast)
{
    const struct Deal dl = MakeReferenceDeal();
    const struct DdTableDeal table_deal = MakeReferenceTableDeal();
    struct DdTableDealPBN pbn_deal = {};
    struct FutureTricks fut = {};
    struct DdTableResults results = {};
    struct ParResults par = {};

    EXPECT_EQ(dds_c_solve_board(nullptr, &dl, -1, 1, 1, &fut), RETURN_UNKNOWN_FAULT);
    EXPECT_EQ(dds_c_calc_dd_table(nullptr, &table_deal, &results), RETURN_UNKNOWN_FAULT);
    EXPECT_EQ(dds_c_calc_dd_table_pbn(nullptr, &pbn_deal, &results), RETURN_UNKNOWN_FAULT);
    EXPECT_EQ(dds_c_calc_par(nullptr, &table_deal, 0, &results, &par), RETURN_UNKNOWN_FAULT);
}

TEST(DdsCApiNullHandle, VoidReturningEntryPointsAreNoOps)
{
    // Each must return without dereferencing; the test passing is the assertion.
    dds_c_destroy_solvercontext(nullptr);
    dds_c_configure_tt(nullptr, 1, 0, 0);
    dds_c_resize_tt(nullptr, 0, 0);
    dds_c_clear_tt(nullptr);
    dds_c_reset_for_solve(nullptr);
    dds_c_reset_best_moves_lite(nullptr);
    dds_c_log_append(nullptr, "ignored");
    dds_c_log_clear(nullptr);
    SUCCEED();
}

TEST(DdsCApiNullArgument, PointerArgumentsAreValidated)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    struct DdTableResults results = {};
    struct DdTableDealPBN pbn_deal = {};
    const struct DdTableDeal table_deal = MakeReferenceTableDeal();
    struct FutureTricks fut = {};

    EXPECT_EQ(dds_c_solve_board(ctx, nullptr, -1, 1, 1, &fut), RETURN_UNKNOWN_FAULT);
    EXPECT_EQ(dds_c_calc_dd_table_pbn(ctx, nullptr, &results), RETURN_UNKNOWN_FAULT);
    EXPECT_EQ(dds_c_calc_dd_table_pbn(ctx, &pbn_deal, nullptr), RETURN_UNKNOWN_FAULT);
    EXPECT_EQ(dds_c_calc_par(ctx, &table_deal, 0, &results, nullptr), RETURN_UNKNOWN_FAULT);

    // A null message must be ignored rather than passed through to strlen.
    dds_c_log_append(ctx, nullptr);

    dds_c_destroy_solvercontext(ctx);
}

// ---------------------------------------------------------------------------
// Functional paths for the newly added entry points.
// ---------------------------------------------------------------------------

class DdsCApiConfiguredContext : public testing::TestWithParam<int> {};

TEST_P(DdsCApiConfiguredContext, SolvesReferenceBoard)
{
    // tt_kind 0 = Small, 1 = Large; both must produce a usable context.
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext(GetParam(), 0, 0);
    ASSERT_NE(ctx, nullptr);

    EXPECT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_destroy_solvercontext(ctx);
}

INSTANTIATE_TEST_SUITE_P(BothTtKinds, DdsCApiConfiguredContext,
                         testing::Values(0, 1));

TEST(DdsCApiTtConfiguration, ContextRemainsUsableAfterReconfiguration)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    ASSERT_EQ(SolveReference(ctx), kExpectedTricks);

    // Reconfigure, resize, and clear the TT, then confirm the context still
    // solves correctly — the point is that these calls do not corrupt state.
    dds_c_configure_tt(ctx, 0, 1, 2);
    dds_c_resize_tt(ctx, 1, 2);
    dds_c_clear_tt(ctx);
    EXPECT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_destroy_solvercontext(ctx);
}

// Regression: clear_tt() used to return the TT's memory while keeping the
// instance, so the next lookup read freed pools (ASan: heap-use-after-free in
// TransTableL::lookup_suit). This is the default-configuration path — no TT
// kind switch involved — and is reachable from every binding as
// clear_tt() followed by a solve.
TEST(DdsCApiTtConfiguration, ClearTtThenSolveOnDefaultTt)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    ASSERT_EQ(SolveReference(ctx), kExpectedTricks);
    dds_c_clear_tt(ctx);
    EXPECT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_destroy_solvercontext(ctx);
}

TEST(DdsCApiResets, ResetsLeaveContextUsable)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    ASSERT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_reset_for_solve(ctx);
    dds_c_reset_best_moves_lite(ctx);
    EXPECT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_destroy_solvercontext(ctx);
}

// A Small-TT context survives clear_tt() followed by reset_for_solve() and
// still solves. clear_tt() disposes the TT, so reset_for_solve() finds no
// table and the following solve rebuilds one lazily from the context's config.
// The TransTableS::reset_memory() guard is not on this path — it is covered
// directly by //library/tests/trans_table:trans_table.
TEST(DdsCApiTtConfiguration, SmallTtClearThenResetForSolve)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    ASSERT_EQ(SolveReference(ctx), kExpectedTricks);
    dds_c_configure_tt(ctx, 0 /* Small */, 1, 2);
    dds_c_clear_tt(ctx);
    dds_c_reset_for_solve(ctx);
    EXPECT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_destroy_solvercontext(ctx);
}

TEST(DdsCApiLogging, AppendAndClearAreCallable)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    dds_c_log_append(ctx, "dds_c_api_test");
    dds_c_log_append(ctx, "");
    dds_c_log_clear(ctx);

    // Logging must not disturb solving.
    EXPECT_EQ(SolveReference(ctx), kExpectedTricks);

    dds_c_destroy_solvercontext(ctx);
}

TEST(DdsCApiDdTable, BinaryTableMatchesExpected)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    const struct DdTableDeal deal = MakeReferenceTableDeal();
    struct DdTableResults results = {};
    ASSERT_EQ(dds_c_calc_dd_table(ctx, &deal, &results), RETURN_NO_FAULT);

    for (int strain = 0; strain < DDS_STRAINS; ++strain)
        for (int hand = 0; hand < DDS_HANDS; ++hand)
            EXPECT_EQ(results.res_table[strain][hand], kExpectedDdTable[strain][hand])
                << "res_table[" << strain << "][" << hand << "]";

    dds_c_destroy_solvercontext(ctx);
}

TEST(DdsCApiDdTable, PbnTableMatchesBinaryTable)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    const struct DdTableDeal binary_deal = MakeReferenceTableDeal();
    struct DdTableResults binary_results = {};
    ASSERT_EQ(dds_c_calc_dd_table(ctx, &binary_deal, &binary_results),
              RETURN_NO_FAULT);

    struct DdTableDealPBN pbn_deal = {};
    std::snprintf(pbn_deal.cards, sizeof pbn_deal.cards, "%s", kReferencePbn);
    struct DdTableResults pbn_results = {};
    ASSERT_EQ(dds_c_calc_dd_table_pbn(ctx, &pbn_deal, &pbn_results),
              RETURN_NO_FAULT);

    for (int strain = 0; strain < DDS_STRAINS; ++strain)
        for (int hand = 0; hand < DDS_HANDS; ++hand)
            EXPECT_EQ(pbn_results.res_table[strain][hand],
                      binary_results.res_table[strain][hand])
                << "res_table[" << strain << "][" << hand << "]";

    dds_c_destroy_solvercontext(ctx);
}

TEST(DdsCApiPar, ProducesNonEmptyScore)
{
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    ASSERT_NE(ctx, nullptr);

    const struct DdTableDeal deal = MakeReferenceTableDeal();
    struct DdTableResults results = {};
    struct ParResults par = {};
    ASSERT_EQ(dds_c_calc_par(ctx, &deal, 0 /* vulnerable: none */, &results, &par),
              RETURN_NO_FAULT);

    EXPECT_GT(std::strlen(par.par_score[0]), 0U);

    dds_c_destroy_solvercontext(ctx);
}

}  // namespace
