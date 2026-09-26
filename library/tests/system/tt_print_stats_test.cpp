/// @file tt_print_stats_test.cpp
/// @brief Integration tests for DDS_PRINT_TT_STATS stderr output (#397).
///
/// Follow-up to #392 and #393, which covered this behavior at the unit
/// level. These tests exercise the real solve_board_internal /
/// solve_same_board lifecycle and the TransTableS "n/a" summary path (via
/// a single solve with DDS_TT_KIND=small), both requiring full board setup
/// and stderr capture beyond the scope of the existing unit test files.

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <api/solve_board.hpp>
#include <pbn.hpp>
#include <solver_context/solver_context.hpp>
#include <solver_if.hpp>
#include <system/thread_data.hpp>
#include <trans_table/trans_table_s.hpp>

namespace {

/// Sets an environment variable portably; a null or empty value removes it.
void set_env_var(const char* name, const char* value)
{
#ifdef _WIN32
        _putenv_s(name, value != nullptr ? value : "");
#else
        if (value == nullptr || value[0] == '\0')
                unsetenv(name);
        else
                setenv(name, value, 1);
#endif
}

/// Overrides (or, with a null value, removes) an environment variable for
/// the lifetime of the guard and then restores whatever was there before.
/// (Mirrors the helper in configure_tt_api_test.cpp.)
struct ScopedEnv
{
        ScopedEnv(const char* name, const char* value) : name_(name)
        {
                if (const char* old = std::getenv(name)) {
                        had_old_ = true;
                        old_ = old;
                }
                set_env_var(name, value);
        }
        ~ScopedEnv()
        {
                set_env_var(name_, had_old_ ? old_.c_str() : nullptr);
        }
        const char* name_;
        bool had_old_ = false;
        std::string old_;
};

// A full 13-card-per-hand deal (the known deal also used in
// configure_tt_api_test.cpp).
constexpr const char* kFullDealPbn =
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

/// Pulls every "lookups=<n>" value out of captured DDS_TT_STATS lines, in
/// the order they were printed.
auto extract_lookup_counts(const std::string& captured) -> std::vector<long long>
{
    std::vector<long long> counts;
    const std::regex re(R"(DDS_TT_STATS: lookups=(\d+))");
    auto it = std::sregex_iterator(captured.begin(), captured.end(), re);
    const auto end = std::sregex_iterator();
    for (; it != end; ++it)
        counts.push_back(std::stoll((*it)[1].str()));
    return counts;
}

}  // namespace

/// solve_board_internal and solve_same_board each zero tt_lookup_count
/// before searching (solver_if.cpp). Rather than inferring the reset from
/// the shape of the organic counts a real search happens to produce --
/// which depends on move ordering / search profile and could in principle
/// still be non-decreasing even with a correct reset -- this test makes
/// the property directly observable: it seeds tt_lookup_count with a huge,
/// unmistakable sentinel immediately before each call, and confirms the
/// count each function prints afterward could not possibly include it. If
/// the reset were ever removed, the printed count would be
/// sentinel-plus-real-work and this fails deterministically, independent
/// of what the search itself does.
TEST(TtPrintStatsTest, PerSolveCounterResetAcrossSolveBoardAndSolveSameBoard)
{
    ScopedEnv no_kind("DDS_TT_KIND", nullptr);
    ScopedEnv print_stats("DDS_PRINT_TT_STATS", "1");

    SolverContext ctx;
    Deal dl{};
    ASSERT_EQ(convert_from_pbn(kFullDealPbn, dl.remainCards), RETURN_NO_FAULT);
    dl.trump = 4;
    dl.first = 0;

    ThreadData* thrp = ctx.thread_ptr();
    ASSERT_NE(thrp, nullptr);

    // Any real solve on this (small, fixed) deal does at most a few
    // thousand TT probes, so a printed count anywhere near the sentinel
    // is unambiguous evidence the counter was not reset.
    constexpr std::uint64_t kSentinel = 1'000'000'000ULL;
    constexpr long long kSentinelThreshold = 1'000'000LL;

    FutureTricks fut{};
    thrp->tt_lookup_count = kSentinel;
    testing::internal::CaptureStderr();
    ASSERT_EQ(solve_board_internal(ctx, dl, -1, 1, 1, &fut), RETURN_NO_FAULT);
    std::string captured = testing::internal::GetCapturedStderr();

    auto lookups = extract_lookup_counts(captured);
    ASSERT_EQ(lookups.size(), 1u) << "captured stderr:\n" << captured;
    const std::string board_failure_msg =
        "solve_board_internal's printed lookup count appears to carry "
        "over a stale value instead of resetting; captured stderr:\n";
    EXPECT_LT(lookups[0], kSentinelThreshold) << board_failure_msg << captured;

    // solve_same_board's null-window reuse path for the partner declarer,
    // matching calc_tables.cpp's k==2 branch (hint = the first solve's
    // score).
    Deal partner_dl = dl;
    partner_dl.first = 2;
    FutureTricks partner_fut{};
    thrp->tt_lookup_count = kSentinel;
    testing::internal::CaptureStderr();
    const int same_board_res =
        solve_same_board(ctx, partner_dl, &partner_fut, fut.score[0]);
    ASSERT_EQ(same_board_res, RETURN_NO_FAULT);
    captured = testing::internal::GetCapturedStderr();

    lookups = extract_lookup_counts(captured);
    ASSERT_EQ(lookups.size(), 1u) << "captured stderr:\n" << captured;
    const std::string same_board_failure_msg =
        "solve_same_board's printed lookup count appears to carry over "
        "a stale value instead of resetting; captured stderr:\n";
    EXPECT_LT(lookups[0], kSentinelThreshold) << same_board_failure_msg << captured;
}

/// TransTableS does not track add/overwrite/harvest operation counts:
/// get_op_stats() reports the -1 sentinel for all three fields, and
/// DDS_PRINT_TT_STATS must render that as the literal "n/a" fields rather
/// than the sentinel value leaking into the output.
TEST(TtPrintStatsTest, SmallTransTablePrintsNaForOpStats)
{
    ScopedEnv kind("DDS_TT_KIND", "small");
    ScopedEnv print_stats("DDS_PRINT_TT_STATS", "1");

    SolverContext ctx;
    DealPBN dl{};
    dl.trump = 4;
    dl.first = 0;
    std::strcpy(dl.remainCards, kFullDealPbn);
    FutureTricks fut{};

    testing::internal::CaptureStderr();
    const int res =
        solve_board_pbn(ctx, dl, /*target=*/-1, /*solutions=*/1, /*mode=*/1, &fut);
    const std::string captured = testing::internal::GetCapturedStderr();

    ASSERT_EQ(res, RETURN_NO_FAULT);
    // Confirm the small table is actually the one in play, so a future kind
    // resolution regression fails loudly here rather than passing by luck.
    ASSERT_NE(dynamic_cast<TransTableS*>(ctx.maybe_trans_table()), nullptr)
        << "expected TransTableS given DDS_TT_KIND=small";
    const std::string expected_line =
        "DDS_TT_STATS: adds=n/a overwrites=n/a overwrite_rate=n/a harvests=n/a";
    EXPECT_NE(captured.find(expected_line), std::string::npos)
        << "captured stderr:\n" << captured;
    // The raw sentinel must never leak into the printed summary.
    EXPECT_EQ(captured.find("adds=-1"), std::string::npos);
}
