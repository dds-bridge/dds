/// @file trans_table_p_test.cpp
/// @brief White-box tests for TransTableP, the shape → pattern transposition table.
///
/// TransTableP stores, per (tricks, hand, suit-length shape), relative-rank
/// ownership patterns ordered by generality (bridge-solver's "shape →
/// pattern" cache). These tests pin down the matching semantics, the
/// bound-tightening and ordering rules, memory limits, and equivalence of
/// cut decisions with the legacy TransTableL for small workloads.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <numeric>
#include <sstream>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include <api/dll.h>
#include <trans_table/trans_table_l.hpp>
#include <trans_table/trans_table_p.hpp>

namespace {

constexpr const char* AllRanks = "AKQJT98765432";

auto rank_of(char c) -> int
{
    switch (c) {
    case 'A': return 14;
    case 'K': return 13;
    case 'Q': return 12;
    case 'J': return 11;
    case 'T': return 10;
    default: return c - '0';
    }
}

auto seat_of(char c) -> int
{
    switch (c) {
    case 'N': return 0;
    case 'E': return 1;
    case 'S': return 2;
    default: return 3;
    }
}

/// Bitmask over ranks 2..14 (bit r-2) for the listed rank characters.
auto ranks(const std::string& text) -> unsigned short
{
    unsigned short bits = 0;
    for (char c : text) {
        bits = static_cast<unsigned short>(bits | (1u << (rank_of(c) - 2)));
    }
    return bits;
}

/// A deal expressed as, per suit, the owner (N/E/S/W) of each rank from A down to 2.
struct TestDeal
{
    int hand_lookup[DDS_SUITS][15] = {};

    static auto from_owners(
        const std::string& spades,
        const std::string& hearts,
        const std::string& diamonds,
        const std::string& clubs) -> TestDeal
    {
        TestDeal deal;
        const std::string* suits[DDS_SUITS] = {&spades, &hearts, &diamonds, &clubs};
        for (int s = 0; s < DDS_SUITS; ++s) {
            for (int i = 0; i < 13; ++i) {
                deal.hand_lookup[s][14 - i] = seat_of((*suits[s])[static_cast<size_t>(i)]);
            }
        }
        return deal;
    }

    /// Every rank r in every suit s is held by seat (r + s) % 4 (13 cards each).
    static auto rotating() -> TestDeal
    {
        TestDeal deal;
        for (int s = 0; s < DDS_SUITS; ++s) {
            for (int r = 2; r <= 14; ++r) {
                deal.hand_lookup[s][r] = (r + s) % DDS_HANDS;
            }
        }
        return deal;
    }

    static auto random(std::mt19937& rng) -> TestDeal
    {
        std::vector<int> cards(52);
        std::iota(cards.begin(), cards.end(), 0);
        std::shuffle(cards.begin(), cards.end(), rng);
        TestDeal deal;
        for (size_t i = 0; i < cards.size(); ++i) {
            const int s = cards[i] / 13;
            const int r = 2 + cards[i] % 13;
            deal.hand_lookup[s][r] = static_cast<int>(i / 13);
        }
        return deal;
    }
};

/// The remaining cards of a position, with the derived TT key inputs.
struct TestPosition
{
    unsigned short aggr[DDS_SUITS] = {};
    int hand_dist[DDS_HANDS] = {};
    int tricks = 0;

    static auto remaining(
        const TestDeal& deal,
        const std::string& spades,
        const std::string& hearts,
        const std::string& diamonds,
        const std::string& clubs) -> TestPosition
    {
        TestPosition pos;
        pos.aggr[0] = ranks(spades);
        pos.aggr[1] = ranks(hearts);
        pos.aggr[2] = ranks(diamonds);
        pos.aggr[3] = ranks(clubs);
        pos.finish(deal);
        return pos;
    }

    void finish(const TestDeal& deal)
    {
        int length[DDS_HANDS][DDS_SUITS] = {};
        int total = 0;
        for (int s = 0; s < DDS_SUITS; ++s) {
            for (int r = 2; r <= 14; ++r) {
                if (aggr[s] & (1u << (r - 2))) {
                    ++length[deal.hand_lookup[s][r]][s];
                    ++total;
                }
            }
        }
        for (int h = 0; h < DDS_HANDS; ++h) {
            hand_dist[h] = (length[h][0] << 8) | (length[h][1] << 4) | length[h][2];
        }
        tricks = total / 4 - 1;
    }
};

auto full_deal_position(const TestDeal& deal) -> TestPosition
{
    return TestPosition::remaining(deal, AllRanks, AllRanks, AllRanks, AllRanks);
}

/// A random legal position of `deal`: `tricks_played` whole tricks of random
/// cards have been removed.
auto random_position(const TestDeal& deal, std::mt19937& rng, int tricks_played) -> TestPosition
{
    TestPosition pos;
    for (int s = 0; s < DDS_SUITS; ++s) pos.aggr[s] = 0x1fff;
    for (int t = 0; t < tricks_played; ++t) {
        for (int h = 0; h < DDS_HANDS; ++h) {
            std::vector<std::pair<int, int>> held;
            for (int s = 0; s < DDS_SUITS; ++s)
                for (int r = 2; r <= 14; ++r)
                    if ((pos.aggr[s] & (1u << (r - 2))) && deal.hand_lookup[s][r] == h)
                        held.emplace_back(s, r);
            const auto [s, r] = held[std::uniform_int_distribution<size_t>(0, held.size() - 1)(rng)];
            pos.aggr[s] = static_cast<unsigned short>(pos.aggr[s] & ~(1u << (r - 2)));
        }
    }
    pos.finish(deal);
    return pos;
}

auto node(int lower, int upper, int best_suit = 0, int best_rank = 0) -> NodeCards
{
    NodeCards cards{};
    cards.lower_bound = static_cast<char>(lower);
    cards.upper_bound = static_cast<char>(upper);
    cards.best_move_suit = static_cast<char>(best_suit);
    cards.best_move_rank = static_cast<char>(best_rank);
    return cards;
}

struct WinRanks
{
    unsigned short ranks[DDS_SUITS] = {};
};

auto win(const std::string& spades,
         const std::string& hearts = "",
         const std::string& diamonds = "",
         const std::string& clubs = "") -> WinRanks
{
    WinRanks w;
    w.ranks[0] = ranks(spades);
    w.ranks[1] = ranks(hearts);
    w.ranks[2] = ranks(diamonds);
    w.ranks[3] = ranks(clubs);
    return w;
}

/// Random winning ranks drawn from the cards still in play.
auto random_win_ranks(const TestPosition& pos, std::mt19937& rng) -> WinRanks
{
    WinRanks w;
    for (int s = 0; s < DDS_SUITS; ++s) {
        w.ranks[s] = static_cast<unsigned short>(
            pos.aggr[s] & std::uniform_int_distribution<int>(0, 0x1fff)(rng));
    }
    return w;
}

/// One lookup-then-add of a random position, the way the search does it.
void add_random_entry(TransTableP& tt, const TestDeal& deal, std::mt19937& rng, int i)
{
    const auto pos = random_position(deal, rng, 1 + (i % 6));
    const auto w = random_win_ranks(pos, rng);
    const int hand = std::uniform_int_distribution<int>(0, 3)(rng);
    const int lo = std::uniform_int_distribution<int>(0, 13)(rng);
    bool lower_flag = false;
    (void)tt.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, -1, lower_flag);
    tt.add(pos.tricks, hand, pos.aggr, w.ranks, node(lo, 13), true);
}

class TransTablePTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        tt_.set_memory_default(16);
        tt_.set_memory_maximum(32);
        tt_.make_tt();
    }

    void init(const TestDeal& deal)
    {
        tt_.init(deal.hand_lookup);
    }

    /// Runs lookup() then add() the way ab_search_0 does for a fresh node.
    void store(const TestPosition& pos, int hand, const WinRanks& w, const NodeCards& cards,
               bool flag = true)
    {
        bool lower_flag = false;
        (void)tt_.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, -1, lower_flag);
        tt_.add(pos.tricks, hand, pos.aggr, w.ranks, cards, flag);
    }

    auto lookup(const TestPosition& pos, int hand, int limit, bool& lower_flag) -> NodeCards const*
    {
        lower_flag = false;
        return tt_.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, limit, lower_flag);
    }

    TransTableP tt_;
};

// ---------------------------------------------------------------------------
// Basic hit / miss semantics
// ---------------------------------------------------------------------------

TEST_F(TransTablePTest, LookupOnEmptyTableMisses)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    bool lower_flag = true;

    // Act
    NodeCards const* hit = lookup(pos, 0, 5, lower_flag);

    // Assert
    EXPECT_EQ(hit, nullptr);
    EXPECT_EQ(tt_.node_count(), 0u);
}

TEST_F(TransTablePTest, StoredPositionIsFoundWithLowerFlagWhenLowerBoundExceedsLimit)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(7, 12));
    bool lower_flag = false;

    // Act
    NodeCards const* hit = lookup(pos, 0, 6, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->lower_bound, 7);
    EXPECT_EQ(hit->upper_bound, 12);
    EXPECT_EQ(tt_.node_count(), 1u);
}

TEST_F(TransTablePTest, StoredPositionIsFoundWithoutLowerFlagWhenUpperBoundWithinLimit)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(2, 5));
    bool lower_flag = true;

    // Act
    NodeCards const* hit = lookup(pos, 0, 5, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_FALSE(lower_flag);
}

TEST_F(TransTablePTest, StoredPositionMissesWhenLimitFallsStrictlyInsideBounds)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(3, 8));
    bool lower_flag = false;

    // Act & Assert
    EXPECT_EQ(lookup(pos, 0, 3, lower_flag), nullptr);   // lower == limit: no cut
    EXPECT_EQ(lookup(pos, 0, 7, lower_flag), nullptr);   // upper > limit: no cut
    EXPECT_NE(lookup(pos, 0, 2, lower_flag), nullptr);
    EXPECT_NE(lookup(pos, 0, 8, lower_flag), nullptr);
}

TEST_F(TransTablePTest, DifferentHandTricksOrShapeDoNotMatch)
{
    // Arrange: the same deal with one trick of low cards played, twice, in two
    // ways that give different shapes.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto full = full_deal_position(deal);
    // Trick one: 5432 of spades (owners 3,2,1,0 for the rotating deal).
    const auto after_spade_trick =
        TestPosition::remaining(deal, "AKQJT9876", AllRanks, AllRanks, AllRanks);
    // Alternative first trick: 5432 of hearts.
    const auto after_heart_trick =
        TestPosition::remaining(deal, AllRanks, "AKQJT9876", AllRanks, AllRanks);
    ASSERT_EQ(after_spade_trick.tricks, after_heart_trick.tricks);
    ASSERT_NE(after_spade_trick.hand_dist[0], after_heart_trick.hand_dist[0]);
    store(after_spade_trick, 1, win("A"), node(6, 6));
    bool lower_flag = false;

    // Act & Assert
    EXPECT_NE(lookup(after_spade_trick, 1, 5, lower_flag), nullptr);
    EXPECT_EQ(lookup(after_spade_trick, 2, 5, lower_flag), nullptr) << "hand differs";
    EXPECT_EQ(lookup(after_heart_trick, 1, 5, lower_flag), nullptr) << "shape differs";
    EXPECT_EQ(lookup(full, 1, 5, lower_flag), nullptr) << "trick count differs";
}

// ---------------------------------------------------------------------------
// Relative-rank pattern generalization
// ---------------------------------------------------------------------------

TEST_F(TransTablePTest, PositionDifferingOnlyInIrrelevantCardsHits)
{
    // Arrange: in spades North holds A and 4, East holds K and 3, the rest are
    // irrelevant. Two positions with the same shape whose spade holdings differ
    // only below the lowest winning rank (the king).
    const auto deal = TestDeal::from_owners(
        "NESWSWNE" "NESW" "N",
        "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    // Remaining spades A K Q J 5 4 3 2 (owners N E S W E S W N) ...
    const auto pos_a = TestPosition::remaining(deal, "AKQJ5432", "AKQJ", "AKQJ", "AKQJ");
    // ... and A K T 9 7 6 4 3 (owners N E S W E N S W): same shape, same top
    // four owners, different owners further down.
    const auto pos_b = TestPosition::remaining(deal, "AKT97643", "AKQJ", "AKQJ", "AKQJ");
    ASSERT_EQ(std::memcmp(pos_a.hand_dist, pos_b.hand_dist, sizeof(pos_a.hand_dist)), 0);
    store(pos_a, 0, win("J"), node(4, 4));
    bool lower_flag = false;

    // Act
    NodeCards const* hit = lookup(pos_b, 0, 3, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
}

TEST_F(TransTablePTest, PositionDifferingInARelevantCardMisses)
{
    // Arrange: same spade layout as above, but now the third-highest spade is
    // relevant and is held by different seats in the two positions.
    const auto deal = TestDeal::from_owners(
        "NESWSWNE" "NESW" "N",
        "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    const auto pos_a = TestPosition::remaining(deal, "AKQJ5432", "AKQJ", "AKQJ", "AKQJ");
    // A K J T 9 8 7 4 (owners N E W S W N E S): same shape as pos_a but the
    // third-highest spade now belongs to West, not South.
    const auto pos_c = TestPosition::remaining(deal, "AKJT9874", "AKQJ", "AKQJ", "AKQJ");
    ASSERT_EQ(std::memcmp(pos_a.hand_dist, pos_c.hand_dist, sizeof(pos_a.hand_dist)), 0);
    store(pos_a, 0, win("Q"), node(4, 4));
    bool lower_flag = false;

    // Act & Assert
    EXPECT_EQ(lookup(pos_c, 0, 3, lower_flag), nullptr);
    EXPECT_NE(lookup(pos_a, 0, 3, lower_flag), nullptr);
}

TEST_F(TransTablePTest, ZeroWinRanksMakesEverySameShapePositionMatch)
{
    // Arrange
    const auto deal = TestDeal::from_owners(
        "NESWSWNE" "NESW" "N",
        "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    const auto pos_a = TestPosition::remaining(deal, "AKQJ5432", "AKQJ", "AKQJ", "AKQJ");
    const auto pos_c = TestPosition::remaining(deal, "AKJT9874", "AKQJ", "AKQJ", "AKQJ");
    store(pos_a, 0, win(""), node(0, 2));
    bool lower_flag = true;

    // Act
    NodeCards const* hit = lookup(pos_c, 0, 2, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_FALSE(lower_flag);
    for (int s = 0; s < DDS_SUITS; ++s) {
        EXPECT_EQ(hit->least_win[s], 0);
    }
}

TEST_F(TransTablePTest, LeastWinEncodesLowestRelevantRankPerSuit)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("AK", "", "Q", "2"), node(9, 9));
    bool lower_flag = false;

    // Act
    NodeCards const* hit = lookup(pos, 0, 8, lower_flag);

    // Assert: least_win = 15 - lowest relevant absolute rank, 0 when unused.
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->least_win[0], 15 - 13);
    EXPECT_EQ(hit->least_win[1], 0);
    EXPECT_EQ(hit->least_win[2], 15 - 12);
    EXPECT_EQ(hit->least_win[3], 15 - 2);
}

// ---------------------------------------------------------------------------
// Bounds merging, best move, subsumption and deduplication
// ---------------------------------------------------------------------------

/// The key encodes the top twelve cards of a suit; the thirteenth's owner is
/// implied by the shape, so "all 13 relevant" and "top 12 relevant" match
/// exactly the same positions and share one entry. That entry must still
/// report the full suit as winning if either store did, whatever the order.
TEST_F(TransTablePTest, WholeSuitAndTopTwelveShareAnEntryThatKeepsTheWholeSuit)
{
    const auto deal = TestDeal::rotating();
    const auto pos = full_deal_position(deal);
    for (const bool whole_suit_first : {true, false}) {
        // Arrange
        tt_.reset_memory(ResetReason::NewDeal);
        init(deal);
        const WinRanks whole = win("2");   // lowest winner is the deuce: 13 relevant
        const WinRanks top12 = win("3");   // lowest winner is the three: 12 relevant
        store(pos, 0, whole_suit_first ? whole : top12, node(7, 12));

        // Act
        store(pos, 0, whole_suit_first ? top12 : whole, node(7, 12));
        bool lower_flag = false;
        NodeCards const* hit = lookup(pos, 0, 6, lower_flag);

        // Assert
        EXPECT_EQ(tt_.node_count(), 1u) << "whole_suit_first=" << whole_suit_first;
        ASSERT_NE(hit, nullptr);
        EXPECT_EQ(static_cast<int>(hit->least_win[0]), 13) << "whole_suit_first=" << whole_suit_first;
    }
}

/// A pattern is a statement about positions - this shape, these owners of the
/// relevant cards - and a position's value does not depend on which deal it
/// came from. The solver relies on that: for a "similar" deal it re-runs
/// init() without resetting the table (as with TransTableL). Entries must
/// therefore survive init() and keep hinging on the relevant cards only.
TEST_F(TransTablePTest, PatternsSurviveASimilarDealAndStillHingeOnTheRelevantCards)
{
    // Arrange: only the spade ace is relevant; the deals below share a shape.
    const auto original = TestDeal::from_owners("NESWNESWNESWN", "NESWNESWNESWN", "NESWNESWNESWN", "NESWNESWNESWN");
    const auto low_cards_swapped = TestDeal::from_owners("NESWNESWNESNW", "NESWNESWNESWN", "NESWNESWNESWN", "NESWNESWNESWN");
    const auto ace_moved = TestDeal::from_owners("ENSWNESWNESWN", "NESWNESWNESWN", "NESWNESWNESWN", "NESWNESWNESWN");
    init(original);
    store(full_deal_position(original), 0, win("A"), node(7, 12));

    // Act
    init(low_cards_swapped);
    bool lower_flag = false;
    NodeCards const* similar_hit = lookup(full_deal_position(low_cards_swapped), 0, 6, lower_flag);
    init(ace_moved);
    NodeCards const* moved_ace = lookup(full_deal_position(ace_moved), 0, 6, lower_flag);

    // Assert
    ASSERT_NE(similar_hit, nullptr);
    EXPECT_EQ(static_cast<int>(similar_hit->lower_bound), 7);
    EXPECT_EQ(static_cast<int>(similar_hit->upper_bound), 12);
    EXPECT_EQ(moved_ace, nullptr);
}

TEST_F(TransTablePTest, ReAddingTheSamePatternIntersectsBounds)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(2, 10));
    store(pos, 0, win("A"), node(5, 12));
    bool lower_flag = false;

    // Act
    NodeCards const* hit = lookup(pos, 0, 4, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->lower_bound, 5);
    EXPECT_EQ(hit->upper_bound, 10);
    EXPECT_EQ(tt_.node_count(), 1u);
}

TEST_F(TransTablePTest, BestMoveIsKeptOnlyWhenTheStoreIsFlaggedAsACutoff)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    bool lower_flag = false;

    // Act & Assert: an exhaustive (flag == false) store has no best move ...
    store(pos, 0, win("A"), node(0, 3, 2, 11), false);
    NodeCards const* hit = lookup(pos, 0, 3, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->best_move_suit, 0);
    EXPECT_EQ(hit->best_move_rank, 0);

    // ... while a cutoff store records it, also when merging into the entry.
    store(pos, 0, win("A"), node(1, 3, 2, 11), true);
    hit = lookup(pos, 0, 3, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(hit->best_move_suit, 2);
    EXPECT_EQ(hit->best_move_rank, 11);
}

TEST_F(TransTablePTest, PatternsWithDifferentRelevantCardsAreStoredSeparately)
{
    // Arrange: a generic pattern (ace only) already bounds the position.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(3, 9));

    // Act: a more specific pattern (AKQ relevant) for the same position,
    // looser below but tighter above.
    store(pos, 0, win("Q"), node(2, 7));

    // Assert: the two patterns are distinct entries, each keeping its own
    // bounds; re-adding the generic one tightens only the generic one.
    EXPECT_EQ(tt_.node_count(), 2u);
    store(pos, 0, win("A"), node(5, 9));
    EXPECT_EQ(tt_.node_count(), 2u);
    bool lower_flag = false;
    NodeCards const* hit = lookup(pos, 0, 4, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->lower_bound, 5);
    EXPECT_EQ(hit->least_win[0], 1);
    hit = lookup(pos, 0, 9, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_FALSE(lower_flag);
    EXPECT_EQ(hit->upper_bound, 9);
    EXPECT_EQ(hit->least_win[0], 1);
    hit = lookup(pos, 0, 7, lower_flag);
    ASSERT_NE(hit, nullptr) << "only the specific pattern's upper bound cuts here";
    EXPECT_FALSE(lower_flag);
    EXPECT_EQ(hit->upper_bound, 7);
    EXPECT_EQ(hit->least_win[0], 3);
}

TEST_F(TransTablePTest, MoreSpecificPatternWithTighterBoundsIsStoredAndFound)
{
    // Arrange
    const auto deal = TestDeal::from_owners(
        "NESWSWNE" "NESW" "N",
        "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    const auto pos_a = TestPosition::remaining(deal, "AKQJ5432", "AKQJ", "AKQJ", "AKQJ");
    const auto pos_c = TestPosition::remaining(deal, "AKJT9874", "AKQJ", "AKQJ", "AKQJ");
    store(pos_a, 0, win("K"), node(3, 9));   // matches pos_a and pos_c
    store(pos_a, 0, win("Q"), node(6, 9));   // matches only pos_a
    bool lower_flag = false;

    // Act & Assert
    EXPECT_EQ(tt_.node_count(), 2u);
    NodeCards const* hit_a = lookup(pos_a, 0, 5, lower_flag);
    ASSERT_NE(hit_a, nullptr) << "specific pattern must cut at its tighter bound";
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit_a->lower_bound, 6);
    EXPECT_EQ(lookup(pos_c, 0, 5, lower_flag), nullptr) << "generic bound alone does not cut";
    EXPECT_NE(lookup(pos_c, 0, 2, lower_flag), nullptr) << "generic bound still applies";
}

TEST_F(TransTablePTest, GenericPatternAddedAfterSpecificOnesCoversThemAll)
{
    // Arrange: two specific patterns on different positions of one shape.
    const auto deal = TestDeal::from_owners(
        "NESWSWNE" "NESW" "N",
        "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    const auto pos_a = TestPosition::remaining(deal, "AKQJ5432", "AKQJ", "AKQJ", "AKQJ");
    const auto pos_c = TestPosition::remaining(deal, "AKJT9874", "AKQJ", "AKQJ", "AKQJ");
    store(pos_a, 0, win("Q"), node(6, 8));   // top three spades: N E S
    store(pos_c, 0, win("J"), node(2, 4));   // top three spades: N E W

    // Act: a generic pattern (top two spades: N E) covering both.
    store(pos_a, 0, win("K"), node(1, 9));

    // Assert: three entries; each position cuts on its own specific bound
    // and both share the generic one.
    EXPECT_EQ(tt_.node_count(), 3u);
    bool lower_flag = false;
    NodeCards const* hit_a = lookup(pos_a, 0, 5, lower_flag);
    ASSERT_NE(hit_a, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit_a->lower_bound, 6);
    NodeCards const* hit_c = lookup(pos_c, 0, 5, lower_flag);
    ASSERT_NE(hit_c, nullptr);
    EXPECT_FALSE(lower_flag);
    EXPECT_EQ(hit_c->upper_bound, 4);
    EXPECT_EQ(lookup(pos_a, 0, 0, lower_flag)->least_win[0], 2);
    EXPECT_EQ(lookup(pos_c, 0, 0, lower_flag)->least_win[0], 2);
}

TEST_F(TransTablePTest, TighteningAPatternDoesNotTouchOtherPatterns)
{
    // Arrange: a specific pattern alongside a generic one.
    const auto deal = TestDeal::from_owners(
        "NESWSWNE" "NESW" "N",
        "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    const auto pos_a = TestPosition::remaining(deal, "AKQJ5432", "AKQJ", "AKQJ", "AKQJ");
    store(pos_a, 0, win("K"), node(0, 12));
    store(pos_a, 0, win("Q"), node(5, 12));
    ASSERT_EQ(tt_.node_count(), 2u);

    // Act: the generic pattern learns an upper bound of 6.
    store(pos_a, 0, win("K"), node(0, 6));

    // Assert
    EXPECT_EQ(tt_.node_count(), 2u);
    bool lower_flag = true;
    NodeCards const* hit = lookup(pos_a, 0, 6, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_FALSE(lower_flag);
    EXPECT_EQ(hit->upper_bound, 6);
    EXPECT_EQ(hit->least_win[0], 2);
    hit = lookup(pos_a, 0, 4, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->lower_bound, 5);
    EXPECT_EQ(hit->upper_bound, 12);
    EXPECT_EQ(hit->least_win[0], 3);
}

TEST_F(TransTablePTest, IncomparableMatchingPatternsAreTriedMostGenericFirst)
{
    // Arrange: two patterns that both match the position but constrain
    // disjoint cards. The one with fewer relevant cards is more general and
    // so more likely to match future positions; it should be found first
    // regardless of insertion order.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("K", "", "K"), node(7, 7, 2, 13));     // 4 relevant cards
    store(pos, 0, win("Q", "Q"), node(7, 7, 0, 14));         // 6 relevant cards
    ASSERT_EQ(tt_.node_count(), 2u);

    // Act
    bool lower_flag = false;
    NodeCards const* hit = lookup(pos, 0, 6, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->best_move_suit, 2);
    EXPECT_EQ(hit->least_win[2], 2);
    EXPECT_EQ(hit->least_win[1], 0);
}

TEST_F(TransTablePTest, AmongEquallyGenericPatternsTheOlderIsTriedFirst)
{
    // Arrange: two incomparable patterns of equal weight in the same bucket
    // (same first relevant suit and top-card owner), both matching `pos`.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A", "A"), node(7, 12));   // older
    store(pos, 0, win("AK"), node(8, 12));       // newer
    ASSERT_EQ(tt_.node_count(), 2u);

    // Act: both cut at this limit; the first one scanned is returned.
    bool lower_flag = false;
    NodeCards const* hit = lookup(pos, 0, 6, lower_flag);

    // Assert
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->lower_bound, 7);
}

TEST_F(TransTablePTest, PatternsWhoseFirstRelevantSuitDiffersAreAllFound)
{
    // Arrange: one pattern per suit, each relevant only in that suit, plus a
    // pattern with no relevant cards at all. Every position of the shape
    // must be checked against all of them.
    const auto deal = TestDeal::rotating();
    init(deal);
    // In the rotating deal the spade owners are S E N W S E N W S E N W S
    // from the ace down; both positions below have one spade gone per hand.
    const auto pos = TestPosition::remaining(deal, "KQJT65432", AllRanks, AllRanks, AllRanks);
    const auto other = TestPosition::remaining(deal, "A98765432", AllRanks, AllRanks, AllRanks);
    ASSERT_EQ(pos.tricks, other.tricks);
    ASSERT_TRUE(std::equal(pos.hand_dist, pos.hand_dist + DDS_HANDS, other.hand_dist));
    store(pos, 0, win("K"), node(1, 12, 0, 0));
    store(pos, 0, win("", "K"), node(2, 12, 1, 0));
    store(pos, 0, win("", "", "K"), node(3, 12, 2, 0));
    store(pos, 0, win("", "", "", "K"), node(4, 12, 3, 0));
    store(pos, 0, win(""), node(0, 11, 0, 5));
    ASSERT_EQ(tt_.node_count(), 5u);

    // Act / Assert: raising the limit knocks the patterns out one by one.
    bool lower_flag = false;
    NodeCards const* hit = lookup(pos, 0, 3, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->best_move_suit, 3);
    hit = lookup(pos, 0, 2, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_GE(hit->lower_bound, 3);
    hit = lookup(pos, 0, 11, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_FALSE(lower_flag);
    EXPECT_EQ(hit->best_move_rank, 5);
    EXPECT_EQ(lookup(pos, 0, 4, lower_flag), nullptr);
    EXPECT_EQ(lookup(pos, 0, 10, lower_flag), nullptr);

    // The other position differs from pos only in who holds the top spades,
    // so it misses the spade pattern but still matches the heart, diamond,
    // club and wildcard patterns.
    hit = lookup(other, 0, 3, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->best_move_suit, 3);
    hit = lookup(other, 0, 0, lower_flag);   // only the spade pattern's [1, 12] bound
    ASSERT_NE(hit, nullptr);                 // is useless here; another must cut
    EXPECT_NE(hit->best_move_suit, 0);
    EXPECT_EQ(hit->least_win[0], 0);
}

// ---------------------------------------------------------------------------
// Memory management and lifecycle
// ---------------------------------------------------------------------------

// The table owns raw pattern blocks; an implicit copy would alias and then
// double-free them.
static_assert(!std::is_copy_constructible_v<TransTableP>, "TransTableP must not be copyable");
static_assert(!std::is_copy_assignable_v<TransTableP>, "TransTableP must not be copy-assignable");

TEST_F(TransTablePTest, ResetMemoryForgetsEverythingButKeepsTheTableUsable)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(7, 12));

    // Act
    tt_.reset_memory(ResetReason::NewDeal);

    // Assert
    bool lower_flag = false;
    EXPECT_EQ(lookup(pos, 0, 6, lower_flag), nullptr);
    EXPECT_EQ(tt_.node_count(), 0u);
    store(pos, 0, win("A"), node(7, 12));
    EXPECT_NE(lookup(pos, 0, 6, lower_flag), nullptr);
}

TEST_F(TransTablePTest, ReturnAllMemoryThenMakeTtStartsFresh)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(7, 12));

    // Act
    tt_.return_all_memory();
    tt_.make_tt();
    init(deal);

    // Assert
    bool lower_flag = false;
    EXPECT_EQ(lookup(pos, 0, 6, lower_flag), nullptr);
    store(pos, 0, win("A"), node(7, 12));
    EXPECT_NE(lookup(pos, 0, 6, lower_flag), nullptr);
}

TEST_F(TransTablePTest, TableWithoutADealIsInertUntilInit)
{
    // Arrange: return_all_memory() drops the deal-specific ownership table,
    // and make_tt() cannot rebuild it. Until init() runs again the table must
    // neither crash nor store anything.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    store(pos, 0, win("A"), node(7, 12));
    tt_.return_all_memory();
    tt_.make_tt();

    // Act
    bool lower_flag = false;
    NodeCards const* hit = lookup(pos, 0, 6, lower_flag);
    store(pos, 0, win("A"), node(7, 12));

    // Assert: inert without a deal, fully usable once init() has run.
    EXPECT_EQ(hit, nullptr);
    EXPECT_EQ(tt_.node_count(), 0u);
    init(deal);
    store(pos, 0, win("A"), node(7, 12));
    EXPECT_NE(lookup(pos, 0, 6, lower_flag), nullptr);
}

TEST_F(TransTablePTest, PooledBlockPointerStorageCountsTowardsMemoryInUse)
{
    // Arrange: one shape with a full block and nothing pooled yet.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    const char* spades[] = {"A", "AK", "AKQ", "AKQJ", "AKQJT", "AKQJT9", "AKQJT98", "AKQJT987"};
    for (const char* s : spades) store(pos, 0, win(s), node(7, 12));
    const double before_kb = tt_.memory_in_use();

    // Act: an ordinary reset pools the block; the shape table keeps its size.
    tt_.reset_memory(ResetReason::NewDeal);

    // Assert: the pool's pointer storage is part of the footprint.
    EXPECT_GT(tt_.memory_in_use(), before_kb);
}

/// A lookup() leaves behind the shape it resolved so that the following add()
/// can reuse it. Anything that empties the table, or changes the deal, makes
/// that remembered shape meaningless; an add() that arrives without a fresh
/// lookup() afterwards must be ignored rather than filed under the old shape.
TEST_F(TransTablePTest, AddWithoutAFreshLookupAfterAResetOrNewDealIsIgnored)
{
    const auto deal = TestDeal::rotating();
    const auto pos = full_deal_position(deal);
    const auto forget = [&](const char* how) {
        if (how == std::string("reset")) tt_.reset_memory(ResetReason::NewDeal);
        else if (how == std::string("make_tt")) tt_.make_tt();
        else if (how == std::string("return_all_memory")) {
            tt_.return_all_memory();
            tt_.make_tt();
            init(deal);
        }
        else init(deal);   // "init": a new deal on a live table
    };
    for (const char* how : {"reset", "make_tt", "return_all_memory", "init"}) {
        // Arrange: a lookup() has remembered the shape of pos for trick 0 / hand 0.
        tt_.make_tt();
        init(deal);
        bool lower_flag = false;
        (void)lookup(pos, 0, 6, lower_flag);
        forget(how);

        // Act: add() without a lookup() in between.
        tt_.add(pos.tricks, 0, pos.aggr, win("AK").ranks, node(7, 12), true);

        // Assert
        EXPECT_EQ(tt_.node_count(), 0u) << how;
        EXPECT_EQ(tt_.shape_count(), 0u) << how;
    }
}

/// An ordinary reset leaves no occupied shape slot behind: the same shapes
/// are counted afresh on the next solve, so the load factor stays exact and
/// the open-addressed table keeps growing when it should.
TEST_F(TransTablePTest, RepeatedSolveAndResetCyclesRecountEveryShape)
{
    // Arrange: enough distinct shapes to make the shape table grow.
    std::mt19937 rng(23);
    const auto deal = TestDeal::random(rng);
    init(deal);
    std::vector<TestPosition> positions;
    for (int i = 0; i < 3000; ++i) positions.push_back(random_position(deal, rng, 1 + (i % 12)));
    std::size_t first_cycle_shapes = 0;

    for (int cycle = 0; cycle < 4; ++cycle) {
        // Act: a solve's worth of stores, then the between-deals reset.
        for (const auto& pos : positions) store(pos, 0, win("A"), node(0, 13));
        if (cycle == 0) first_cycle_shapes = tt_.shape_count();

        // Assert: every shape is counted again each cycle, none linger.
        EXPECT_EQ(tt_.shape_count(), first_cycle_shapes) << "cycle " << cycle;
        EXPECT_GT(tt_.shape_count(), 1000u);
        tt_.reset_memory(ResetReason::NewDeal);
        EXPECT_EQ(tt_.shape_count(), 0u) << "cycle " << cycle;
        EXPECT_EQ(tt_.node_count(), 0u) << "cycle " << cycle;
    }
}

/// Writes a diagnostic dump through the ofstream API and returns it as text.
template <typename Dump>
auto dumped(Dump&& dump) -> std::string
{
    const std::string path = ::testing::TempDir() + "/trans_table_p_dump.txt";
    {
        std::ofstream fout(path, std::ios::trunc);
        dump(fout);
    }
    std::ifstream fin(path);
    std::stringstream text;
    text << fin.rdbuf();
    return text.str();
}

/// The card-aware dump reports which of the shape's patterns match the given
/// cards, and shows each match's bounds and the owners of its relevant cards.
TEST_F(TransTablePTest, CardAwareDumpListsOnlyThePatternsMatchingTheCards)
{
    // Arrange: two one-trick-played positions of the same shape whose top
    // spades have different owners (A=N K=E ... versus T=E 9=N ...).
    const auto deal = TestDeal::from_owners("NESWENSWNESWN", "NESWNESWNESWN", "ESWNESWNESWNE", "SWNESWNESWNES");
    init(deal);
    const auto low_spades_played = TestPosition::remaining(deal, "AKQJT9876", AllRanks, AllRanks, AllRanks);
    const auto high_spades_played = TestPosition::remaining(deal, "T98765432", AllRanks, AllRanks, AllRanks);
    ASSERT_EQ(low_spades_played.tricks, high_spades_played.tricks);
    ASSERT_TRUE(std::equal(std::begin(low_spades_played.hand_dist), std::end(low_spades_played.hand_dist),
                           std::begin(high_spades_played.hand_dist)));
    store(low_spades_played, 0, win("AK"), node(3, 9));
    store(low_spades_played, 0, win("AKQ"), node(4, 8));

    // Act
    const auto same_cards = dumped([&](std::ofstream& f) {
        tt_.print_entries_dist_and_cards(f, low_spades_played.tricks, 0, low_spades_played.aggr, low_spades_played.hand_dist);
    });
    const auto other_cards = dumped([&](std::ofstream& f) {
        tt_.print_entries_dist_and_cards(f, high_spades_played.tricks, 0, high_spades_played.aggr, high_spades_played.hand_dist);
    });

    // Assert
    EXPECT_NE(same_cards.find("2 patterns"), std::string::npos) << same_cards;
    EXPECT_NE(same_cards.find("2 match the cards"), std::string::npos) << same_cards;
    EXPECT_NE(same_cards.find("[3, 9]"), std::string::npos) << same_cards;
    EXPECT_NE(same_cards.find("[4, 8]"), std::string::npos) << same_cards;
    EXPECT_NE(same_cards.find("S:NE "), std::string::npos) << same_cards;
    EXPECT_NE(same_cards.find("S:NES "), std::string::npos) << same_cards;
    EXPECT_NE(other_cards.find("2 patterns"), std::string::npos) << other_cards;
    EXPECT_NE(other_cards.find("0 match the cards"), std::string::npos) << other_cards;
    EXPECT_EQ(other_cards.find("[3, 9]"), std::string::npos) << other_cards;
}

/// An unknown shape is reported as such rather than as an empty match list.
TEST_F(TransTablePTest, CardAwareDumpReportsAnUnknownShape)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);

    // Act
    const auto text = dumped([&](std::ofstream& f) {
        tt_.print_entries_dist_and_cards(f, pos.tricks, 0, pos.aggr, pos.hand_dist);
    });

    // Assert
    EXPECT_NE(text.find("0 patterns"), std::string::npos) << text;
    EXPECT_NE(text.find("0 match the cards"), std::string::npos) << text;
}

TEST_F(TransTablePTest, ReturnAllMemoryLeavesNothingAllocated)
{
    // Arrange: a table with patterns, pooled blocks and the ownership table.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    const char* spades[] = {"A", "AK", "AKQ", "AKQJ", "AKQJT", "AKQJT9", "AKQJT98", "AKQJT987", "AKQJT9876"};
    for (const char* s : spades) store(pos, 0, win(s), node(7, 12));   // grows a block → one pooled
    ASSERT_GT(tt_.memory_in_use(), 0.0);

    // Act
    tt_.return_all_memory();

    // Assert
    EXPECT_EQ(tt_.memory_in_use(), 0.0);
    EXPECT_EQ(tt_.node_count(), 0u);
    EXPECT_EQ(tt_.shape_count(), 0u);
}

TEST_F(TransTablePTest, MemoryExhaustedResetReturnsToTheEmptyTableFootprint)
{
    // Arrange
    const auto deal = TestDeal::rotating();
    init(deal);
    const double empty_kb = tt_.memory_in_use();
    const auto pos = full_deal_position(deal);
    const char* spades[] = {"A", "AK", "AKQ", "AKQJ", "AKQJT", "AKQJT9", "AKQJT98", "AKQJT987", "AKQJT9876"};
    for (const char* s : spades) store(pos, 0, win(s), node(7, 12));
    ASSERT_GT(tt_.memory_in_use(), empty_kb);

    // Act
    tt_.reset_memory(ResetReason::MemoryExhausted);

    // Assert: no active and no pooled blocks remain, only the empty shape table.
    EXPECT_EQ(tt_.memory_in_use(), empty_kb);
    EXPECT_EQ(tt_.node_count(), 0u);
    store(pos, 0, win("A"), node(7, 12));
    bool lower_flag = false;
    EXPECT_NE(lookup(pos, 0, 6, lower_flag), nullptr);
}

TEST(TransTablePMemoryTest, StaysWithinTheMaximumAndResetsWhenExhausted)
{
    // Arrange: a tiny table and a stream of distinct positions/patterns.
    TransTableP tt;
    tt.set_memory_default(1);
    tt.set_memory_maximum(2);
    tt.make_tt();
    std::mt19937 rng(7);
    const auto deal = TestDeal::random(rng);
    tt.init(deal.hand_lookup);
    const double baseline_kb = tt.memory_in_use();
    size_t max_nodes_seen = 0;
    bool shrank_at_some_point = false;

    // Act
    for (int i = 0; i < 50000; ++i) {
        const size_t before = tt.node_count();
        add_random_entry(tt, deal, rng, i);
        if (tt.node_count() < before) shrank_at_some_point = true;
        max_nodes_seen = std::max(max_nodes_seen, tt.node_count());
        ASSERT_LE(tt.memory_in_use(), baseline_kb + 2 * 1024.0 + 1.0);
    }

    // Assert
    EXPECT_TRUE(shrank_at_some_point) << "the table never hit its limit";
    EXPECT_GT(max_nodes_seen, 1000u);
}

TEST(TransTablePMemoryTest, LoweringTheMaximumBelowCurrentUsageIsEnforcedImmediately)
{
    // Arrange: a roomy table filled well past the 1 MB it is about to be given.
    TransTableP tt;
    tt.set_memory_default(16);
    tt.set_memory_maximum(32);
    tt.make_tt();
    std::mt19937 rng(11);
    const auto deal = TestDeal::random(rng);
    tt.init(deal.hand_lookup);
    const double baseline_kb = tt.memory_in_use();
    int i = 0;
    while (tt.memory_in_use() < baseline_kb + 3 * 1024.0 && i < 400000) {
        add_random_entry(tt, deal, rng, i++);
    }
    ASSERT_GT(tt.memory_in_use(), baseline_kb + 3 * 1024.0) << "could not fill the table";

    // Act
    tt.set_memory_maximum(1);

    // Assert: over-budget contents are reclaimed at once, and the new cap holds
    // for later inserts, including those that fit into existing blocks.
    EXPECT_LE(tt.memory_in_use(), baseline_kb + 1024.0 + 1.0);
    for (int j = 0; j < 10000; ++j) {
        add_random_entry(tt, deal, rng, j);
        ASSERT_LE(tt.memory_in_use(), baseline_kb + 1024.0 + 1.0);
    }
}

TEST(TransTablePMemoryTest, PoolingOutgrownBlocksNeverExceedsTheMaximum)
{
    // Arrange: a tiny cap and deep positions, so that many shapes hold small
    // blocks that keep outgrowing (and pooling) their storage near the cap.
    TransTableP tt;
    tt.set_memory_default(1);
    tt.set_memory_maximum(2);
    tt.make_tt();
    std::mt19937 rng(17);
    const auto deal = TestDeal::random(rng);
    tt.init(deal.hand_lookup);
    const double cap_kb = tt.memory_in_use() + 2 * 1024.0;
    size_t max_shapes = 0;

    // Act & Assert: the hard cap holds after every single add, with no slack
    // for the pool's own bookkeeping.
    for (int i = 0; i < 40000; ++i) {
        const auto pos = random_position(deal, rng, 1 + (i % 12));
        const auto w = random_win_ranks(pos, rng);
        bool lower_flag = false;
        (void)tt.lookup(pos.tricks, 0, pos.aggr, pos.hand_dist, -1, lower_flag);
        tt.add(pos.tricks, 0, pos.aggr, w.ranks, node(0, 13), true);
        max_shapes = std::max(max_shapes, tt.shape_count());
        ASSERT_LE(tt.memory_in_use(), cap_kb) << "after add " << i;
    }
    EXPECT_GT(max_shapes, 4000u) << "not enough blocks to make pooling costly";
}

/// A caller that configures only the hard maximum gets exactly that maximum;
/// the unset default limit must not be replaced by a built-in value that then
/// floors the cap far above what was asked for.
TEST(TransTablePMemoryTest, AMaximumSetWithoutADefaultIsHonouredAsTheCap)
{
    // Arrange
    TransTableP tt;
    tt.set_memory_maximum(1);   // 1 MiB, no set_memory_default()
    tt.make_tt();
    std::mt19937 rng(31);
    const auto deal = TestDeal::random(rng);
    tt.init(deal.hand_lookup);
    const double cap_kb = tt.memory_in_use() + 1024.0;

    // Act & Assert: several MiB worth of entries never lift the footprint above the cap.
    for (int i = 0; i < 20000; ++i) {
        const auto pos = random_position(deal, rng, 1 + (i % 12));
        const auto w = random_win_ranks(pos, rng);
        bool lower_flag = false;
        (void)tt.lookup(pos.tricks, 0, pos.aggr, pos.hand_dist, -1, lower_flag);
        tt.add(pos.tricks, 0, pos.aggr, w.ranks, node(0, 13), true);
        ASSERT_LE(tt.memory_in_use(), cap_kb) << "after add " << i;
    }
}

TEST(TransTablePMemoryTest, LoweringTheMaximumWhileStillWithinItKeepsTheContents)
{
    // Arrange
    TransTableP tt;
    tt.set_memory_default(16);
    tt.set_memory_maximum(32);
    tt.make_tt();
    std::mt19937 rng(13);
    const auto deal = TestDeal::random(rng);
    tt.init(deal.hand_lookup);
    for (int i = 0; i < 2000; ++i) add_random_entry(tt, deal, rng, i);
    const size_t nodes_before = tt.node_count();
    ASSERT_GT(nodes_before, 0u);
    ASSERT_LT(tt.memory_in_use(), 4 * 1024.0);

    // Act
    tt.set_memory_maximum(4);

    // Assert
    EXPECT_EQ(tt.node_count(), nodes_before);
}

TEST_F(TransTablePTest, ReAddingAPatternToAFullTreeTightensInPlaceWithoutGrowingIt)
{
    // Arrange: exactly fill a fresh tree (InitialTreeNodes = 8) with distinct
    // patterns of one shape.
    const auto deal = TestDeal::rotating();
    init(deal);
    const auto pos = full_deal_position(deal);
    const char* spades[] = {"A", "AK", "AKQ", "AKQJ", "AKQJT", "AKQJT9", "AKQJT98", "AKQJT987"};
    for (const char* s : spades) store(pos, 0, win(s), node(7, 12));
    ASSERT_EQ(tt_.node_count(), 8u);
    const double before_kb = tt_.memory_in_use();

    // Act: re-add the first pattern with tighter bounds.
    store(pos, 0, win("A"), node(8, 11));

    // Assert: tightened in place; no block was grown (or the table reset).
    EXPECT_EQ(tt_.node_count(), 8u);
    EXPECT_EQ(tt_.memory_in_use(), before_kb);
    bool lower_flag = false;
    NodeCards const* hit = lookup(pos, 0, 7, lower_flag);
    ASSERT_NE(hit, nullptr);
    EXPECT_TRUE(lower_flag);
    EXPECT_EQ(hit->lower_bound, 8);
    EXPECT_EQ(hit->upper_bound, 11);
}

// ---------------------------------------------------------------------------
// Equivalence with TransTableL on small workloads
// ---------------------------------------------------------------------------

/// For workloads small enough that TransTableL never evicts, both tables must
/// take identical cut decisions on every lookup. Bounds are generated to be
/// consistent per (tricks, hand) so that intersections never become empty.
/// Both tables must hand ab_search the same `least_win`: the number of cards
/// at or above the lowest winning rank (what `win_ranks[aggr][least_win]`
/// expects), not an absolute rank. Sparse winners (e.g. A and 9 remaining,
/// 9 the lowest winner) are where a rank encoding and a count would differ.
TEST(TransTablePEquivalenceTest, LeastWinMatchesTransTableLForTheSameStore)
{
    std::mt19937 rng(5);
    const auto deal = TestDeal::random(rng);
    TransTableL large;
    large.set_memory_default(16);
    large.set_memory_maximum(32);
    large.make_tt();
    large.init(deal.hand_lookup);
    TransTableP pattern;
    pattern.set_memory_default(16);
    pattern.set_memory_maximum(32);
    pattern.make_tt();
    pattern.init(deal.hand_lookup);

    int compared = 0;
    for (int i = 0; i < 300; ++i) {
        // Arrange: one position, one pattern, stored in both tables. The
        // winning rank of each suit is a random remaining card, so the
        // relevant cards are usually a sparse subset of the suit.
        const auto pos = random_position(deal, rng, 1 + (i % 3));
        WinRanks w;
        for (int s = 0; s < DDS_SUITS; ++s) {
            if (pos.aggr[s] == 0 || (i + s) % 2 == 0) continue;
            std::vector<int> bits;
            for (int b = 0; b < 13; ++b)
                if (pos.aggr[s] & (1u << b)) bits.push_back(b);
            const int b = bits[std::uniform_int_distribution<size_t>(0, bits.size() - 1)(rng)];
            w.ranks[s] = static_cast<unsigned short>(1u << b);
        }
        const int hand = i % DDS_HANDS;
        bool lower = false;
        if (large.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, 5, lower) != nullptr ||
            pattern.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, 5, lower) != nullptr) {
            continue;   // an earlier store already covers this position
        }
        large.add(pos.tricks, hand, pos.aggr, w.ranks, node(5, 5), true);
        pattern.add(pos.tricks, hand, pos.aggr, w.ranks, node(5, 5), true);

        // Act
        NodeCards const* hit_l = large.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, 5, lower);
        NodeCards const* hit_p = pattern.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, 5, lower);

        // Assert
        ASSERT_NE(hit_l, nullptr) << "step " << i;
        ASSERT_NE(hit_p, nullptr) << "step " << i;
        for (int s = 0; s < DDS_SUITS; ++s) {
            EXPECT_EQ(static_cast<int>(hit_p->least_win[s]), static_cast<int>(hit_l->least_win[s]))
                << "step " << i << " suit " << s;
        }
        ++compared;
    }
    EXPECT_GT(compared, 100);
}

TEST(TransTablePEquivalenceTest, CutDecisionsMatchTransTableLOnSmallWorkloads)
{
    for (unsigned seed = 1; seed <= 12; ++seed) {
        // Arrange
        std::mt19937 rng(seed);
        const auto deal = TestDeal::random(rng);
        TransTableL large;
        large.set_memory_default(16);
        large.set_memory_maximum(32);
        large.make_tt();
        large.init(deal.hand_lookup);
        TransTableP pattern;
        pattern.set_memory_default(16);
        pattern.set_memory_maximum(32);
        pattern.make_tt();
        pattern.init(deal.hand_lookup);

        int hidden_value[13][DDS_HANDS];
        for (auto& row : hidden_value)
            for (int& v : row) v = std::uniform_int_distribution<int>(2, 8)(rng);

        std::vector<TestPosition> seen;
        auto random_position = [&]() {
            TestPosition pos;
            for (int s = 0; s < DDS_SUITS; ++s) pos.aggr[s] = 0x1fff;
            // TransTableL indexes tricks 0..11, so at least one trick is played.
            const int tricks_played = std::uniform_int_distribution<int>(1, 3)(rng);
            for (int t = 0; t < tricks_played; ++t) {
                for (int h = 0; h < DDS_HANDS; ++h) {
                    std::vector<std::pair<int, int>> held;
                    for (int s = 0; s < DDS_SUITS; ++s)
                        for (int r = 2; r <= 14; ++r)
                            if ((pos.aggr[s] & (1u << (r - 2))) && deal.hand_lookup[s][r] == h)
                                held.emplace_back(s, r);
                    // Prefer low cards so that shapes and top cards repeat often.
                    std::sort(held.begin(), held.end(),
                              [](auto a, auto b) { return a.second < b.second; });
                    const size_t idx = std::min(held.size() - 1,
                        static_cast<size_t>(std::uniform_int_distribution<int>(0, 5)(rng)));
                    const auto [s, r] = held[idx];
                    pos.aggr[s] = static_cast<unsigned short>(pos.aggr[s] & ~(1u << (r - 2)));
                }
            }
            pos.finish(deal);
            return pos;
        };

        int hits = 0;
        // Act & Assert
        for (int step = 0; step < 400; ++step) {
            TestPosition pos = (!seen.empty() && step % 3 == 0)
                ? seen[std::uniform_int_distribution<size_t>(0, seen.size() - 1)(rng)]
                : random_position();
            seen.push_back(pos);
            const int hand = std::uniform_int_distribution<int>(0, 3)(rng);
            const int limit = std::uniform_int_distribution<int>(-1, 13)(rng);

            bool lower_l = false;
            bool lower_p = false;
            NodeCards const* hit_l =
                large.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, limit, lower_l);
            NodeCards const* hit_p =
                pattern.lookup(pos.tricks, hand, pos.aggr, pos.hand_dist, limit, lower_p);
            ASSERT_EQ(hit_l != nullptr, hit_p != nullptr)
                << "seed " << seed << " step " << step << " limit " << limit;
            if (hit_l != nullptr) {
                ++hits;
                EXPECT_EQ(lower_l, lower_p) << "seed " << seed << " step " << step;
                continue;
            }

            // Store a pattern whose relevant cards are the top few of one or two suits.
            WinRanks w;
            for (int s = 0; s < DDS_SUITS; ++s) {
                if (std::uniform_int_distribution<int>(0, 2)(rng) != 0) continue;
                const int keep = std::uniform_int_distribution<int>(1, 3)(rng);
                unsigned short bits = pos.aggr[s];
                int count = 0;
                for (int r = 14; r >= 2 && count < keep; --r) {
                    if (bits & (1u << (r - 2))) {
                        ++count;
                        if (count == keep) w.ranks[s] = static_cast<unsigned short>(1u << (r - 2));
                    }
                }
            }
            const int v = hidden_value[pos.tricks][hand];
            const int lo = v - std::uniform_int_distribution<int>(0, 3)(rng);
            const int hi = v + std::uniform_int_distribution<int>(0, 3)(rng);
            const bool flag = std::uniform_int_distribution<int>(0, 1)(rng) == 1;
            const auto cards = node(std::max(lo, 0), std::min(hi, 13), 1, 12);
            large.add(pos.tricks, hand, pos.aggr, w.ranks, cards, flag);
            pattern.add(pos.tricks, hand, pos.aggr, w.ranks, cards, flag);
        }
        EXPECT_GT(hits, 20) << "seed " << seed << ": workload produced too few hits to be meaningful";
    }
}

} // namespace
