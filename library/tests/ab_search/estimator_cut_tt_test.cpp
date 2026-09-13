/// @file estimator_cut_tt_test.cpp
/// @brief QT/LT cutoffs can be memoized via store_ab_tt_result, but ab_search_0
///        must not auto-store them in the pattern TT.
///
/// Using the estimator's win_ranks as a pattern over-generalizes (AnalysePlay
/// then disagrees with SolveBoard). Storing an exact remaining-card key is
/// sound, but on list100 it roughly doubled solve/calc time by flooding each
/// shape's pattern list. The helper is tested directly; the search path stays
/// store-free for estimator cuts.

#include <cstring>
#include <memory>

#include <gtest/gtest.h>

#include <ab_search.hpp>
#include <api/dds.h>
#include <api/dll.h>
#include <init.hpp>
#include <later_tricks.hpp>
#include <lookup_tables/lookup_tables.hpp>
#include <quick_tricks.hpp>
#include <solver_context/solver_context.hpp>
#include <system/memory.hpp>
#include <trans_table/trans_table.hpp>
#include <utility/constants.h>

extern Memory memory;

namespace {

constexpr int kDepth = 20;  // TT lookup runs before QT
constexpr int kHand = 0;    // North leads

class EstimatorCutTtTest : public ::testing::Test {
 protected:
  void SetUp() override
  {
    InitializeStaticMemory();
    if (memory.NumThreads() == 0) {
      memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);
    }

    ctx_ = std::make_unique<SolverContext>();
    auto* thrp = ctx_->thread_ptr();
    ASSERT_NE(thrp, nullptr);
    std::memset(thrp->suit, 0, sizeof(thrp->suit));
    thrp->trump = DDS_NOTRUMP;
    std::memset(&thrp->lookAheadPos, 0, sizeof(thrp->lookAheadPos));
  }

  void place(int hand, int suit, int rank)
  {
    ctx_->thread_ptr()->suit[hand][suit] =
        static_cast<unsigned short>(ctx_->thread_ptr()->suit[hand][suit] | bit_map_rank[rank]);
  }

  void finish_deal()
  {
    auto thrp = ctx_->thread();
    SetDeal(thrp);
    SetDealTables(*ctx_);

    Deal dl{};
    dl.first = kHand;
    thrp->lookAheadPos.first[kDepth] = kHand;
    thrp->lookAheadPos.hand_rel_first = 0;
    InitWinners(dl, thrp->lookAheadPos, thrp);

    ctx_->search().node_type_store(0) = MAXNODE;
    ctx_->search().node_type_store(1) = MINNODE;
    ctx_->search().node_type_store(2) = MAXNODE;
    ctx_->search().node_type_store(3) = MINNODE;
  }

  auto pos() -> Pos& { return ctx_->thread_ptr()->lookAheadPos; }

  auto lookup_score(int target) -> bool
  {
    bool score_flag = false;
    const int tricks = kDepth >> 2;
    const bool hit = apply_ab_tt_lookup(
        &pos(), target, kDepth, tricks, kHand, *ctx_, score_flag);
    EXPECT_TRUE(hit) << "expected a TT hit after store_ab_tt_result";
    return score_flag;
  }

  std::unique_ptr<SolverContext> ctx_;
};

}  // namespace

TEST_F(EstimatorCutTtTest, SearchDoesNotStoreAQuickTricksCutoff)
{
  place(0, 0, 14);
  place(1, 0, 13);
  place(0, 1, 2);
  place(1, 1, 3);
  finish_deal();
  pos().tricks_max = 0;

  bool qt_cut = false;
  ASSERT_GE(QuickTricks(pos(), kHand, kDepth, /*target*/ 1, DDS_NOTRUMP, qt_cut, *ctx_), 1);
  ASSERT_TRUE(qt_cut);

  EXPECT_TRUE(ab_search_0(&pos(), /*target*/ 1, kDepth, *ctx_));

  bool score_flag = true;
  EXPECT_FALSE(apply_ab_tt_lookup(
      &pos(), /*target*/ 1, kDepth, kDepth >> 2, kHand, *ctx_, score_flag));
}

TEST_F(EstimatorCutTtTest, StoreHelperMemoizesAQuickTricksCutoffExactly)
{
  place(0, 0, 14);
  place(1, 0, 13);
  place(0, 1, 2);
  place(1, 1, 3);
  finish_deal();
  pos().tricks_max = 0;

  bool qt_cut = false;
  const int qtricks = QuickTricks(pos(), kHand, kDepth, /*target*/ 1, DDS_NOTRUMP, qt_cut, *ctx_);
  ASSERT_TRUE(qt_cut);
  ASSERT_GE(qtricks, 1);

  bool preexisting = false;
  ASSERT_FALSE(apply_ab_tt_lookup(
      &pos(), /*target*/ 1, kDepth, kDepth >> 2, kHand, *ctx_, preexisting));

  store_ab_tt_result(
      &pos(), /*target*/ 1, kDepth, kDepth >> 2, kHand, /*value*/ true, *ctx_, pos().aggr);

  EXPECT_TRUE(lookup_score(/*target*/ 1));

  for (int s = 0; s < DDS_SUITS; ++s) {
    pos().winner[s] = HighCardType{0, -1};
    pos().second_best[s] = HighCardType{0, -1};
  }
  EXPECT_TRUE(lookup_score(/*target*/ 1));
}

TEST_F(EstimatorCutTtTest, ExactEstimatorKeyDoesNotMatchSwappedLowCards)
{
  place(0, 0, 14);
  place(1, 0, 13);
  place(0, 1, 2);
  place(1, 1, 3);
  finish_deal();
  pos().tricks_max = 0;

  bool qt_cut = false;
  ASSERT_TRUE(QuickTricks(pos(), kHand, kDepth, /*target*/ 1, DDS_NOTRUMP, qt_cut, *ctx_) >= 1);
  ASSERT_TRUE(qt_cut);
  bool preexisting = false;
  ASSERT_FALSE(apply_ab_tt_lookup(
      &pos(), /*target*/ 1, kDepth, kDepth >> 2, kHand, *ctx_, preexisting));
  store_ab_tt_result(
      &pos(), /*target*/ 1, kDepth, kDepth >> 2, kHand, /*value*/ true, *ctx_, pos().aggr);

  ctx_->thread_ptr()->suit[0][1] = bit_map_rank[3];
  ctx_->thread_ptr()->suit[1][1] = bit_map_rank[2];
  finish_deal();
  pos().tricks_max = 0;

  bool score_flag = false;
  EXPECT_FALSE(apply_ab_tt_lookup(
      &pos(), /*target*/ 1, kDepth, kDepth >> 2, kHand, *ctx_, score_flag));
}

TEST_F(EstimatorCutTtTest, StoreHelperMemoizesALaterTricksCutoff)
{
  place(0, 0, 14);
  place(1, 0, 13);
  place(1, 1, 14);
  place(3, 2, 14);
  place(1, 3, 14);
  finish_deal();
  pos().tricks_max = 0;

  const int target = 6;
  bool qt_cut = true;
  (void)QuickTricks(pos(), kHand, kDepth, target, DDS_NOTRUMP, qt_cut, *ctx_);
  ASSERT_FALSE(qt_cut);
  ASSERT_FALSE(LaterTricksMIN(pos(), kHand, kDepth, target, DDS_NOTRUMP, *ctx_));

  bool preexisting = false;
  ASSERT_FALSE(apply_ab_tt_lookup(
      &pos(), target, kDepth, kDepth >> 2, kHand, *ctx_, preexisting));

  store_ab_tt_result(
      &pos(), target, kDepth, kDepth >> 2, kHand, /*value*/ false, *ctx_, pos().aggr);

  EXPECT_FALSE(lookup_score(target));
}
