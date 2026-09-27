/// @file analyse_later_board_tt_stats_test.cpp
/// @brief Tests for analyse_later_board's TT-counter reset/report (#379 item 4).
///
/// analyse_later_board() reuses the caller's warm SolverContext/TT across an
/// entire AnalysePlayBin/AnalysePlayPBN call (see the warm-context comment in
/// solver_if.cpp, guarding the #156 regression), so it must reset only its
/// own instrumentation counters -- not the TT itself -- and report them via
/// the shared print_tt_stats() helper, on both its normal exit and its
/// cardCount<=4 (last trick) early return.
///
/// Both tests use a contrived, fully one-suit-per-hand deal (North all
/// spades, East all hearts, South all diamonds, West all clubs, no trump) so
/// that every trick's winner is forced (only one hand ever holds a card of
/// the suit led), making the whole 52-card play-out deterministic without
/// needing real double-dummy legality/optimality reasoning.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <api/dds.h>
#include <api/dll.h>
#include <api/solve_board.hpp>
#include <pbn.hpp>
#include <solver_context/solver_context.hpp>
#include <solver_if.hpp>

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

constexpr int kNorth = 0;
constexpr int kSpades = 0;
constexpr int kHearts = 1;
constexpr int kDiamonds = 2;
constexpr int kClubs = 3;

// "N:AKQJT98765432... .AKQJT98765432.. ..AKQJT98765432. ...AKQJT98765432"
// North holds all spades, East all hearts, South all diamonds, West all
// clubs. No trump means whichever hand's suit is led always wins (no other
// hand can follow or ruff), so North leads every one of the 13 tricks.
auto MakePureSuitDealPbn() -> std::string
{
    const std::string full_suit = "AKQJT98765432";
    const std::string empty_suit;
    auto hand = [&](int owning_suit) {
        std::string out;
        for (int s = 0; s < 4; ++s) {
            if (s) out += '.';
            out += (s == owning_suit ? full_suit : empty_suit);
        }
        return out;
    };
    const std::string parts =
        "N:" + hand(kSpades) + " " + hand(kHearts) + " " +
        hand(kDiamonds) + " " + hand(kClubs);
    return parts;
}

// The forced 52-card play: trick i is North leads rank (14-i), then East,
// South, West each play their only-suit card of the same rank. North wins
// every trick (only hand following the led suit) and leads again.
auto MakePureSuitPlayCards() -> std::string
{
    static const char kSuitChar[4] = {'S', 'H', 'D', 'C'};
    static const char* kRanks = "23456789TJQKA";  // index 0 -> rank 2
    std::string out;
    for (int rank = 14; rank >= 2; --rank) {
        const char rc = kRanks[rank - 2];
        out += kSuitChar[kSpades];
        out += rc;
        out += kSuitChar[kHearts];
        out += rc;
        out += kSuitChar[kDiamonds];
        out += rc;
        out += kSuitChar[kClubs];
        out += rc;
    }
    return out;
}

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

/// Directly exercises analyse_later_board() the same way #402's
/// PerSolveCounterResetAcrossSolveBoardAndSolveSameBoard exercises
/// solve_board_internal/solve_same_board: seed tt_lookup_count with an
/// unmistakable sentinel immediately before the call and confirm the
/// printed count could not possibly include it. This is independent of
/// search shape/move ordering.
///
/// The call parameters (leadHand, move, hint, hintDir) for the very first
/// analyse_later_board call after the opening lead are derived exactly as
/// play_analyser.cpp's AnalysePlayBin loop derives them for its first
/// played card (trick 1, card 1: usingCurrent is false, card != 4, so
/// hintDir=0 and hint = numTricks - fut.score[0]), rather than driving the
/// full public API, so the sentinel can be injected at the right moment.
TEST(AnalyseLaterBoardTtStatsTest, ResetsCountersIndependentlyOfPriorSolve)
{
    ScopedEnv no_kind("DDS_TT_KIND", nullptr);
    ScopedEnv print_stats("DDS_PRINT_TT_STATS", "1");

    Deal dl{};
    const std::string deal_pbn = MakePureSuitDealPbn();
    ASSERT_EQ(convert_from_pbn(deal_pbn.c_str(), dl.remainCards), RETURN_NO_FAULT);
    dl.trump = DDS_NOTRUMP;
    dl.first = kNorth;

    SolverContext ctx;
    FutureTricks fut{};
    testing::internal::CaptureStderr();
    ASSERT_EQ(solve_board_internal(ctx, dl, -1, 1, 1, &fut), RETURN_NO_FAULT);
    (void)testing::internal::GetCapturedStderr();  // discard the opening-lead report

    const int ini_depth = ctx.search().ini_depth();
    const int num_tricks = ((ini_depth + 3) >> 2) + 1;

    // North's first play-trace card: the Ace of spades (matches
    // MakePureSuitPlayCards()'s first card).
    MoveType move{};
    move.suit = kSpades;
    move.rank = 14;
    move.sequence = 14;
    const int hint = num_tricks - fut.score[0];
    const int hint_dir = 0;

    ThreadData* thrp = ctx.thread_ptr();
    ASSERT_NE(thrp, nullptr);
    constexpr std::uint64_t kSentinel = 1'000'000'000ULL;
    constexpr long long kSentinelThreshold = 1'000'000LL;
    thrp->tt_lookup_count = kSentinel;

    testing::internal::CaptureStderr();
    const int res = analyse_later_board(ctx, dl.first, &move, hint, hint_dir, &fut);
    const std::string captured = testing::internal::GetCapturedStderr();
    ASSERT_EQ(res, RETURN_NO_FAULT);

    const auto lookups = extract_lookup_counts(captured);
    ASSERT_EQ(lookups.size(), 1u) << "captured stderr:\n" << captured;
    const std::string reset_failure_msg =
        "analyse_later_board's printed lookup count appears to carry over "
        "a stale value from the preceding solve instead of resetting; "
        "captured stderr:\n";
    EXPECT_LT(lookups[0], kSentinelThreshold) << reset_failure_msg << captured;
}

/// Drives the full 52-card forced play-out through the real, public
/// AnalysePlayPBN, and confirms every card -- including the very last one,
/// which takes analyse_later_board's cardCount<=4 early-return path -- gets
/// its own DDS_TT_STATS report. Before the point-4 fix, the early-return
/// path skipped the report entirely, so a regression here shows up as one
/// fewer block than played cards rather than as a subtle count difference.
TEST(AnalyseLaterBoardTtStatsTest, ReportsOnBothNormalAndLastTrickEarlyReturnExits)
{
    ScopedEnv no_kind("DDS_TT_KIND", nullptr);
    ScopedEnv print_stats("DDS_PRINT_TT_STATS", "1");

    DealPBN dl{};
    const std::string deal_pbn = MakePureSuitDealPbn();
    std::strncpy(dl.remainCards, deal_pbn.c_str(), sizeof(dl.remainCards) - 1);
    dl.trump = DDS_NOTRUMP;
    dl.first = kNorth;

    PlayTracePBN trace{};
    const std::string play_cards = MakePureSuitPlayCards();
    trace.number = 52;
    std::strncpy(trace.cards, play_cards.c_str(), sizeof(trace.cards) - 1);

    SolvedPlay solved{};
    testing::internal::CaptureStderr();
    const int res = AnalysePlayPBN(dl, trace, &solved, /*thrId=*/0);
    const std::string captured = testing::internal::GetCapturedStderr();

    ASSERT_EQ(res, RETURN_NO_FAULT);

    const auto lookups = extract_lookup_counts(captured);
    // play_analyser.cpp clamps last_trick to numTricks-1 whenever the
    // supplied trace covers the whole deal (last_trick=(52+3)/4=13 >=
    // numTricks=13, so it clamps to 12), so the loop plays tricks 1..12 (48
    // cards) rather than all 13 -- trick 13 is the position the opening
    // solve already covers. 1 opening-lead solve_board_internal report + 48
    // analyse_later_board reports (cards 2..49 of the trace) = 49. The 48th
    // (last) analyse_later_board call is the one that hits cardCount<=4:
    // if that early-return path dropped its report, this would be 48.
    EXPECT_EQ(lookups.size(), 49u) << "captured stderr:\n" << captured;

    // Every individual card in this forced, one-suit-per-hand deal is a
    // trivial (near-instant) search; if the last report's count reflected
    // 51 unreset prior calls' worth of carryover, it would be dramatically
    // larger than a single such search, not merely a bit larger.
    ASSERT_FALSE(lookups.empty());
    constexpr long long kObviouslyCumulativeThreshold = 10'000LL;
    const std::string cumulative_failure_msg =
        "the last trick's printed lookup count looks like it accumulated "
        "prior calls' work instead of resetting; captured stderr:\n";
    EXPECT_LT(lookups.back(), kObviouslyCumulativeThreshold)
        << cumulative_failure_msg << captured;
}
