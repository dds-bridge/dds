/// @file tt_lookup_test.cpp
/// @brief Unit tests for the shared ab_search_0 TT lookup helper.
///
/// Pins miss/hit behavior (score polarity, best-move TT update) so the
/// duplicated depth>=20 and depth<20 lookup blocks can share one function.

#include <cstring>

#include <gtest/gtest.h>

#include <ab_search.hpp>
#include <api/dds.h>
#include <api/dll.h>
#include <solver_context/solver_context.hpp>
#include <system/memory.hpp>
#include <trans_table/trans_table.hpp>

extern Memory memory;

namespace {

class AbTtLookupTest : public ::testing::Test {
 protected:
  void SetUp() override
  {
    InitializeStaticMemory();
    if (memory.NumThreads() == 0)
      memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

    ctx_ = std::make_unique<SolverContext>();
    auto* tt = ctx_->trans_table();
    ASSERT_NE(tt, nullptr);
    ctx_->reset_for_solve();

    int hand_lookup[DDS_SUITS][15] = {};
    tt->init(hand_lookup);

    std::memset(&pos_, 0, sizeof(pos_));
    pos_.first[depth_] = hand_;
    ctx_->search().node_type_store(0) = MAXNODE;
  }

  void SeedTtEntry(char best_suit, char best_rank)
  {
    // Lookup first so suit_lengths_[tricks_] is populated for add().
    bool lower_flag = false;
    (void)ctx_->trans_table()->lookup(
        tricks_, hand_, pos_.aggr, pos_.hand_dist, /*limit*/0, lower_flag);

    // Bounds that are conclusive for limit==0 (MAXNODE with target=1):
    // upper_bound <= limit yields a hit with lower_flag=false.
    NodeCards first{};
    first.lower_bound = 0;
    first.upper_bound = 0;
    first.best_move_suit = best_suit;
    first.best_move_rank = best_rank;
    std::memset(first.least_win, 0, sizeof(first.least_win));

    unsigned short win_ranks[DDS_SUITS] = {};
    // flag=true stores best-move hint; false would clear it.
    ctx_->trans_table()->add(
        tricks_, hand_, pos_.aggr, win_ranks, first, /*flag*/ true);
  }

  std::unique_ptr<SolverContext> ctx_;
  Pos pos_{};
  const int depth_ = 20;  // tricks = 5; also valid for the late (depth < 20) path
  const int tricks_ = 5;
  const int hand_ = 0;
  const int target_ = 1;
};

}  // namespace

TEST_F(AbTtLookupTest, MissReturnsFalse)
{
  bool score_flag = true;
  EXPECT_FALSE(apply_ab_tt_lookup(
      &pos_, target_, depth_, tricks_, hand_, *ctx_, score_flag));
}

TEST_F(AbTtLookupTest, HitReturnsScoreForMaxNode)
{
  SeedTtEntry(/*suit*/ 0, /*rank*/ 0);

  bool score_flag = true;
  ASSERT_TRUE(apply_ab_tt_lookup(
      &pos_, target_, depth_, tricks_, hand_, *ctx_, score_flag));
  // upper_bound <= limit → lower_flag false → MAXNODE score is false.
  EXPECT_FALSE(score_flag);
}

TEST_F(AbTtLookupTest, HitUpdatesBestMoveTtWhenRankNonZero)
{
  SeedTtEntry(/*suit*/ 2, /*rank*/ 14);

  bool score_flag = false;
  ASSERT_TRUE(apply_ab_tt_lookup(
      &pos_, target_, depth_, tricks_, hand_, *ctx_, score_flag));

  EXPECT_EQ(ctx_->search().best_move_tt(depth_).suit, 2);
  EXPECT_EQ(ctx_->search().best_move_tt(depth_).rank, 14);
}

TEST_F(AbTtLookupTest, HitInvertsScoreForMinNode)
{
  ctx_->search().node_type_store(0) = MINNODE;
  SeedTtEntry(/*suit*/ 0, /*rank*/ 0);

  bool max_score = false;
  ctx_->search().node_type_store(0) = MAXNODE;
  // Re-seed is unnecessary; same entry. Lookup twice with different node types.
  ASSERT_TRUE(apply_ab_tt_lookup(
      &pos_, target_, depth_, tricks_, hand_, *ctx_, max_score));

  bool min_score = false;
  ctx_->search().node_type_store(0) = MINNODE;
  ASSERT_TRUE(apply_ab_tt_lookup(
      &pos_, target_, depth_, tricks_, hand_, *ctx_, min_score));

  EXPECT_NE(max_score, min_score);
}

TEST_F(AbTtLookupTest, WorksForShallowDepthPath)
{
  // Same helper is used for depth < 20; ensure a shallow depth still hits.
  const int shallow_depth = 16;
  pos_.first[shallow_depth] = hand_;
  SeedTtEntry(/*suit*/ 1, /*rank*/ 13);

  bool score_flag = false;
  ASSERT_TRUE(apply_ab_tt_lookup(
      &pos_, target_, shallow_depth, tricks_, hand_, *ctx_, score_flag));
  EXPECT_EQ(ctx_->search().best_move_tt(shallow_depth).suit, 1);
  EXPECT_EQ(ctx_->search().best_move_tt(shallow_depth).rank, 13);
}
