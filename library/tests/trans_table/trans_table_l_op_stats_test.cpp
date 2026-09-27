/// @file trans_table_l_op_stats_test.cpp
/// @brief Unit tests for TransTableL's op-stats counters (#379 item 3).
///
/// Follow-up to the instrumentation added in #365: TransTableL tracks
/// page_stats_.num_adds_/num_overwrites_/num_harvests_ in create_or_update(),
/// and exposes them via get_op_stats()/reset_op_stats().
///
/// The public interface has no way to fill a WinBlock to BlocksPerEntry
/// capacity (the precondition for the overwrite path), so
/// FullBlockReplacementIncrementsOverwrites seeds WinBlock/WinMatch state
/// directly via the TransTableLOpStatsTest friend declaration in
/// trans_table_l.hpp, rather than reverse-engineering add()'s internal
/// win-rank/aggregation encoding to manufacture 125 distinct real keys.
/// Every seeded entry uses mask_index_ = 0 and top_set1_..4_ = 0; the real
/// add() call below always computes a nonzero mask_index_ (suits with no
/// win rank contribute 0xF nibbles -- see add()'s w==0 branch), so it can
/// never spuriously match a seeded entry and is guaranteed to take the
/// "not found -> new add" path.

#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include <api/dll.h>
#include <trans_table/trans_table_l.hpp>

// Not wrapped in an anonymous namespace: the friend declaration in
// trans_table_l.hpp names ::TransTableLOpStatsTest at global scope, and an
// anonymous-namespace class of the same name would be a distinct,
// unrelated type with no access to TransTableL's private members.
class TransTableLOpStatsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        int hand_lookup[DDS_SUITS][15] = {};
        tt_.init(hand_lookup);
        tt_.set_memory_default(64);
        tt_.set_memory_maximum(64);
        tt_.make_tt();
    }

    // Pins last_block_seen_[tricks][hand] via a real (missing) lookup, as
    // add() requires -- see TransTableL::add()'s early-return guard.
    void SeedBlockPointer(int tricks, int hand)
    {
        int hand_dist[DDS_SUITS] = {3, 3, 3, 4};
        unsigned short aggr[DDS_SUITS] = {0, 0, 0, 0};
        bool lower_flag = false;
        tt_.lookup(tricks, hand, aggr, hand_dist, /*limit=*/0, lower_flag);
    }

    static NodeCards MakeNode()
    {
        NodeCards node{};
        node.lower_bound = 0;
        node.upper_bound = 13;
        node.best_move_suit = 0;
        node.best_move_rank = 0;
        std::memset(node.least_win, 0, sizeof(node.least_win));
        return node;
    }

    // Directly populates a full block of BlocksPerEntry (125) valid,
    // mutually distinct entries -- distinct from each other via xor_set_
    // alone, and distinct from any real add() key via mask_index_ == 0 /
    // top_set1_..4_ == 0 (a combination the real encoding never produces
    // once any suit has a nonzero win rank -- see add()'s w==0 branch,
    // which contributes 0xF nibbles to mask_index_ for suits with no win
    // rank). Friendship in trans_table_l.hpp is granted to this fixture
    // class, not to gtest's per-test derived classes, so this access must
    // live in a method defined here rather than inline in a TEST_F body.
    void SeedFullBlock(int tricks, int hand)
    {
        TransTableL::WinBlock* bp = tt_.last_block_seen_[tricks][hand];
        ASSERT_NE(bp, nullptr);
        for (int i = 0; i < BlocksPerEntry; ++i) {
            TransTableL::WinMatch& wp = bp->list_[i];
            wp.xor_set_ = static_cast<unsigned>(i + 1);
            wp.top_set1_ = 0;
            wp.top_set2_ = 0;
            wp.top_set3_ = 0;
            wp.top_set4_ = 0;
            wp.top_mask1_ = 0;
            wp.top_mask2_ = 0;
            wp.top_mask3_ = 0;
            wp.top_mask4_ = 0;
            wp.mask_index_ = 0;
            wp.last_mask_no_ = 1;
            wp.first_ = MakeNode();
        }
        bp->next_match_no_ = BlocksPerEntry;
        bp->next_write_no_ = 0;
    }

    static constexpr int kTricks = 5;
    static constexpr int kHand = 0;

    TransTableL tt_;
};

TEST_F(TransTableLOpStatsTest, InsertIncrementsAddsOnly)
{
    SeedBlockPointer(kTricks, kHand);

    int adds = -1, overwrites = -1, harvests = -1;
    tt_.get_op_stats(adds, overwrites, harvests);
    ASSERT_EQ(adds, 0);
    ASSERT_EQ(overwrites, 0);
    ASSERT_EQ(harvests, 0);

    const unsigned short win_ranks[DDS_SUITS] = {0x0002, 0, 0, 0};
    const unsigned short aggr_target[DDS_SUITS] = {0x0003, 0, 0, 0};
    tt_.add(kTricks, kHand, aggr_target, win_ranks, MakeNode(), /*flag=*/true);

    tt_.get_op_stats(adds, overwrites, harvests);
    EXPECT_EQ(adds, 1);
    EXPECT_EQ(overwrites, 0);
    EXPECT_EQ(harvests, 0);
}

TEST_F(TransTableLOpStatsTest, FullBlockReplacementIncrementsOverwrites)
{
    SeedBlockPointer(kTricks, kHand);
    SeedFullBlock(kTricks, kHand);

    int adds = -1, overwrites = -1, harvests = -1;
    tt_.get_op_stats(adds, overwrites, harvests);
    const std::string seed_precondition_msg =
        "seeding the block directly must not itself touch the op-stats "
        "counters";
    ASSERT_EQ(adds, 0) << seed_precondition_msg;
    ASSERT_EQ(overwrites, 0);

    const unsigned short win_ranks[DDS_SUITS] = {0x0002, 0, 0, 0};
    const unsigned short aggr_target[DDS_SUITS] = {0x0003, 0, 0, 0};
    tt_.add(kTricks, kHand, aggr_target, win_ranks, MakeNode(), /*flag=*/true);

    tt_.get_op_stats(adds, overwrites, harvests);
    EXPECT_EQ(adds, 1);
    EXPECT_EQ(overwrites, 1);
    EXPECT_EQ(harvests, 0);
}

TEST_F(TransTableLOpStatsTest, ResetOpStatsClearsCounters)
{
    SeedBlockPointer(kTricks, kHand);

    const unsigned short win_ranks1[DDS_SUITS] = {0x0002, 0, 0, 0};
    const unsigned short aggr_target1[DDS_SUITS] = {0x0003, 0, 0, 0};
    tt_.add(kTricks, kHand, aggr_target1, win_ranks1, MakeNode(), true);

    const unsigned short win_ranks2[DDS_SUITS] = {0x0004, 0, 0, 0};
    const unsigned short aggr_target2[DDS_SUITS] = {0x0007, 0, 0, 0};
    tt_.add(kTricks, kHand, aggr_target2, win_ranks2, MakeNode(), true);

    int adds = -1, overwrites = -1, harvests = -1;
    tt_.get_op_stats(adds, overwrites, harvests);
    const std::string reset_precondition_msg =
        "precondition: some activity must be recorded before reset is "
        "meaningful";
    ASSERT_GT(adds, 0) << reset_precondition_msg;

    tt_.reset_op_stats();

    tt_.get_op_stats(adds, overwrites, harvests);
    EXPECT_EQ(adds, 0);
    EXPECT_EQ(overwrites, 0);
    EXPECT_EQ(harvests, 0);
}
