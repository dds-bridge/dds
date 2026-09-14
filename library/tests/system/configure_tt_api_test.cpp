/// @file configure_tt_api_test.cpp
/// @brief Tests for transposition table configuration API.
///
/// Validates SolverContext configure_tt() behavior for resizing,
/// switching kinds, and lazy initialization of transposition tables.

#include <cstdlib>
#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include <api/solve_board.hpp>
#include <solver_context/solver_context.hpp>
#include <trans_table/trans_table_l.hpp>
#include <trans_table/trans_table_p.hpp>
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

auto kind_of(const TransTable* tt) -> TTKind
{
    if (dynamic_cast<const TransTableS*>(tt) != nullptr) return TTKind::Small;
    if (dynamic_cast<const TransTableP*>(tt) != nullptr) return TTKind::Pattern;
    return TTKind::Large;
}

TEST(ConfigureTtApiTest, ScopedEnvRestoresThePreviousValueAndAbsence)
{
    // Arrange: a known outer value, itself scoped so that whatever the runner
    // supplied is put back when the test ends.
    const char* name = "DDS_TEST_SCOPED_ENV";
    ScopedEnv outer(name, "before");

    // Act & Assert: an override is undone, and so is a removal.
    {
        ScopedEnv overridden(name, "during");
        EXPECT_STREQ(std::getenv(name), "during");
    }
    EXPECT_STREQ(std::getenv(name), "before");
    {
        ScopedEnv removed(name, nullptr);
        EXPECT_EQ(std::getenv(name), nullptr);
    }
    EXPECT_STREQ(std::getenv(name), "before");
}

TEST(ConfigureTtApiTest, DefaultConfigurationUsesThePatternTable)
{
    // Arrange: no explicit kind anywhere (and no environment override).
    ScopedEnv no_override("DDS_TT_KIND", nullptr);
    SolverConfig cfg;
    SolverContext configured(cfg);
    SolverContext bare;

    // Act & Assert
    EXPECT_EQ(cfg.tt_kind_, TTKind::Pattern);
    EXPECT_NE(nullptr, dynamic_cast<TransTableP*>(configured.trans_table()));
    EXPECT_NE(nullptr, dynamic_cast<TransTableP*>(bare.trans_table()));
}

TEST(ConfigureTtApiTest, PatternKindCreatesPatternTable)
{
    // Arrange: explicit kind, isolated from any ambient override.
    ScopedEnv no_override("DDS_TT_KIND", nullptr);
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Pattern;
    SolverContext ctx(cfg);

    // Act
    auto* tt = ctx.trans_table();

    // Assert
    ASSERT_NE(tt, nullptr);
    EXPECT_NE(nullptr, dynamic_cast<TransTableP*>(tt));
}

/// Prepares `tt` for a deal in which rank r of suit s belongs to seat (r + s) % 4.
void init_rotating_deal(TransTable& tt)
{
    int hand_lookup[DDS_SUITS][15] = {};
    for (int s = 0; s < DDS_SUITS; ++s)
        for (int r = 2; r <= 14; ++r) hand_lookup[s][r] = (r + s) % DDS_HANDS;
    tt.init(hand_lookup);
}

/// Fills a table with many distinct shapes and returns false as soon as its
/// footprint exceeds `cap_kb`.
auto stays_under(TransTable& tt, const double cap_kb) -> bool
{
    const unsigned short aggr[DDS_SUITS] = {0x1fff, 0x1fff, 0x1fff, 0x1fff};
    const unsigned short win_ranks[DDS_SUITS] = {1u << 12, 0, 0, 0};
    NodeCards cards{};
    cards.upper_bound = 13;
    for (unsigned i = 0; i < 20000; ++i) {
        int hand_dist[DDS_HANDS];
        for (int h = 0; h < DDS_HANDS; ++h)
            hand_dist[h] = static_cast<int>((i * 2654435761u * static_cast<unsigned>(h + 1)) & 0xfffu);
        const int tricks = 1 + static_cast<int>(i % 12);
        bool lower_flag = false;
        (void)tt.lookup(tricks, 0, aggr, hand_dist, -1, lower_flag);
        tt.add(tricks, 0, aggr, win_ranks, cards, true);
        if (tt.memory_in_use() > cap_kb) return false;
    }
    return true;
}

/// A configuration that sets only the maximum must yield a table capped at
/// that maximum when it is created lazily; the built-in default used for the
/// unset value may not lift the cap.
TEST(ConfigureTtApiTest, AMaximumOnlyConfigurationIsHonouredOnLazyCreation)
{
    // Arrange
    ScopedEnv no_kind("DDS_TT_KIND", nullptr);
    ScopedEnv no_default("DDS_TT_DEFAULT_MB", nullptr);
    ScopedEnv no_limit("DDS_TT_LIMIT_MB", nullptr);
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Pattern;
    cfg.tt_mem_default_mb_ = 0;
    cfg.tt_mem_maximum_mb_ = 1;
    SolverContext ctx(cfg);

    // Act
    TransTable* tt = ctx.trans_table();
    ASSERT_NE(tt, nullptr);
    init_rotating_deal(*tt);
    const double cap_kb = tt->memory_in_use() + 1024.0;

    // Assert
    EXPECT_TRUE(stays_under(*tt, cap_kb));
}

/// Solves the known deal from examples/hands.cpp (hand 0) in notrump with `ctx`.
auto solve_known_deal(SolverContext& ctx, FutureTricks& fut) -> int
{
    DealPBN dl{};
    dl.trump = 4;
    dl.first = 0;
    std::strcpy(dl.remainCards, "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3");
    return solve_board_pbn(ctx, dl, /*target=*/-1, /*solutions=*/1, /*mode=*/1, &fut);
}

/// A table recreated between two solves of the same deal has not seen that
/// deal; the next solve must initialize it again rather than run against an
/// inert (never init()-ed) cache.
TEST(ConfigureTtApiTest, ATableRecreatedBetweenSolvesOfTheSameDealIsInitialisedAgain)
{
    // Arrange: one solve, then a kind change and back, which recreates the table.
    ScopedEnv no_kind("DDS_TT_KIND", nullptr);
    SolverContext ctx;
    FutureTricks first{};
    ASSERT_EQ(solve_known_deal(ctx, first), RETURN_NO_FAULT);
    ctx.configure_tt(TTKind::Large, 8, 16);
    ctx.configure_tt(TTKind::Pattern, 8, 16);
    auto* recreated = dynamic_cast<TransTableP*>(ctx.maybe_trans_table());
    ASSERT_NE(recreated, nullptr);
    ASSERT_EQ(recreated->node_count(), 0u);

    // Act: the same deal again.
    FutureTricks again{};
    ASSERT_EQ(solve_known_deal(ctx, again), RETURN_NO_FAULT);

    // Assert: same answer, and the cache was actually in use.
    EXPECT_EQ(again.score[0], first.score[0]);
    EXPECT_GT(recreated->node_count(), 0u);
}

/// Reconfiguring a live table with unset limits must resolve them the same
/// way lazy creation does, not hand the table a zero maximum.
TEST(ConfigureTtApiTest, ReconfiguringALiveTableWithUnsetLimitsResolvesThem)
{
    // Arrange
    ScopedEnv no_kind("DDS_TT_KIND", nullptr);
    ScopedEnv no_default("DDS_TT_DEFAULT_MB", nullptr);
    ScopedEnv no_limit("DDS_TT_LIMIT_MB", nullptr);
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Pattern;
    SolverContext ctx(cfg);
    auto* tt = dynamic_cast<TransTableP*>(ctx.trans_table());
    ASSERT_NE(tt, nullptr);
    init_rotating_deal(*tt);

    // Act
    ctx.configure_tt(TTKind::Pattern, /*defMB=*/0, /*maxMB=*/0);

    // Assert: a few hundred shapes fit comfortably; a zero maximum would
    // clear the table on every allocation and keep it near empty.
    const unsigned short aggr[DDS_SUITS] = {0x1fff, 0x1fff, 0x1fff, 0x1fff};
    const unsigned short win_ranks[DDS_SUITS] = {1u << 12, 0, 0, 0};
    NodeCards cards{};
    cards.upper_bound = 13;
    for (unsigned i = 0; i < 300; ++i) {
        int hand_dist[DDS_HANDS];
        for (int h = 0; h < DDS_HANDS; ++h)
            hand_dist[h] = static_cast<int>((i * 2654435761u * static_cast<unsigned>(h + 1)) & 0xfffu);
        bool lower_flag = false;
        (void)tt->lookup(1 + static_cast<int>(i % 12), 0, aggr, hand_dist, -1, lower_flag);
        tt->add(1 + static_cast<int>(i % 12), 0, aggr, win_ranks, cards, true);
    }
    EXPECT_EQ(tt->node_count(), 300u);
}

TEST(ConfigureTtApiTest, SwitchingToPatternRecreatesAndResizingKeepsInstance)
{
    // Arrange: start from the Large table, isolated from any ambient override.
    ScopedEnv no_override("DDS_TT_KIND", nullptr);
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    SolverContext ctx(cfg);
    auto* large = ctx.trans_table();
    ASSERT_NE(nullptr, dynamic_cast<TransTableL*>(large));

    // Act
    ctx.configure_tt(TTKind::Pattern, /*defMB=*/8, /*maxMB=*/16);
    auto* pattern = ctx.maybe_trans_table();
    ctx.configure_tt(TTKind::Pattern, /*defMB=*/16, /*maxMB=*/32);
    auto* resized = ctx.maybe_trans_table();

    // Assert
    ASSERT_NE(pattern, nullptr);
    EXPECT_NE(nullptr, dynamic_cast<TransTableP*>(pattern));
    EXPECT_EQ(pattern, resized) << "same kind: resize in place";
    ctx.configure_tt(TTKind::Large, 8, 16);
    EXPECT_NE(nullptr, dynamic_cast<TransTableL*>(ctx.maybe_trans_table()));
}

TEST(ConfigureTtApiTest, EnvironmentOverridesTableKind)
{
    // Arrange
    ScopedEnv env("DDS_TT_KIND", "pattern");
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Small;
    SolverContext ctx(cfg);

    // Act & Assert
    EXPECT_NE(nullptr, dynamic_cast<TransTableP*>(ctx.trans_table()));
    ScopedEnv env2("DDS_TT_KIND", "large");
    ctx.dispose_trans_table();
    EXPECT_NE(nullptr, dynamic_cast<TransTableL*>(ctx.trans_table()));
}

TEST(ConfigureTtApiTest, ConfigureTtComparesTheEnvironmentResolvedKind)
{
    // Arrange: the environment pins the effective kind to Pattern.
    ScopedEnv env("DDS_TT_KIND", "pattern");
    SolverContext ctx;
    auto* before = ctx.trans_table();
    ASSERT_NE(nullptr, dynamic_cast<TransTableP*>(before));
    // Give the live instance state a recreated one would not have (pointer
    // equality alone is unreliable: a recreated object may reuse the address).
    const int hand_lookup[DDS_SUITS][15] = {};
    before->init(hand_lookup);
    const double marked_kb = before->memory_in_use();

    // Act: asking for Small changes nothing effective, so the instance must
    // survive (resized in place) rather than be destroyed and recreated.
    ctx.configure_tt(TTKind::Small, /*defMB=*/8, /*maxMB=*/8);

    // Assert
    ASSERT_NE(nullptr, ctx.maybe_trans_table());
    EXPECT_EQ(ctx.maybe_trans_table()->memory_in_use(), marked_kb);

    // Act: a new override that differs from the live table must recreate it,
    // even though the configured kind (Small) has not changed.
    ScopedEnv env2("DDS_TT_KIND", "small");
    ctx.configure_tt(TTKind::Small, /*defMB=*/8, /*maxMB=*/8);

    // Assert
    EXPECT_NE(nullptr, dynamic_cast<TransTableS*>(ctx.maybe_trans_table()));
}

TEST(ConfigureTtApiTest, SwitchKindRecreatesTable)
{
  // Default context, isolated from any ambient override (override behavior
  // is covered by the Environment* tests).
  ScopedEnv no_override("DDS_TT_KIND", nullptr);
  SolverContext ctx;
  auto* tt1 = ctx.trans_table();
  ASSERT_NE(tt1, nullptr);

  // Flip to a different kind
  const TTKind new_kind = kind_of(tt1) == TTKind::Small ? TTKind::Large : TTKind::Small;
  ctx.configure_tt(new_kind, /*defMB=*/8, /*maxMB=*/8);

  auto* tt2 = ctx.maybe_trans_table();
  ASSERT_NE(tt2, nullptr);
  EXPECT_EQ(kind_of(tt2), new_kind);
}

TEST(ConfigureTtApiTest, ResizeInPlaceWhenKindUnchanged)
{
  SolverContext ctx;
  auto* tt1 = ctx.trans_table();
  ASSERT_NE(tt1, nullptr);
  const TTKind same_kind = kind_of(tt1);

  // Resize should not replace the instance when kind does not change
  ctx.configure_tt(same_kind, /*defMB=*/16, /*maxMB=*/32);
  auto* tt2 = ctx.maybe_trans_table();
  ASSERT_NE(tt2, nullptr);
  EXPECT_EQ(tt1, tt2) << "Resize should keep the same TT instance";
}

} // namespace
