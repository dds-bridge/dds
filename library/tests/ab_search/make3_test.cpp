/// @file make3_test.cpp
/// @brief Characterization tests for trick-completing make_3 / undo_0.
///
/// Pins make_3 card removal, next-leader selection, trickCards, and winners
/// snapshot/restore before collapsing the thrp and ctx duplicate variants.

#include <cstring>

#include <gtest/gtest.h>

#include <ab_search.hpp>
#include <api/dds.h>
#include <api/dll.h>
#include <solver_context/solver_context.hpp>
#include <system/memory.hpp>
#include <utility/constants.h>

extern Memory memory;

namespace {

constexpr int kDepth = 3;  // trick index (depth + 3) >> 2 == 1
constexpr int kTrick = 1;
constexpr int kHandDelta[DDS_SUITS] = {256, 16, 1, 0};

class Make3Test : public ::testing::Test {
 protected:
  void SetUp() override
  {
    InitializeStaticMemory();
    if (memory.NumThreads() == 0)
      memory.Resize(1, DDS_TT_SMALL, THREADMEM_SMALL_DEF_MB, THREADMEM_SMALL_MAX_MB);

    ctx_ = std::make_unique<SolverContext>();
    std::memset(&pos_, 0, sizeof(pos_));

    // Leader North; fourth hand is West (rel 3).
    leader_ = 0;
    fourth_ = HAND_ID(leader_, 3);
    pos_.first[kDepth] = leader_;

    suit_ = 0;   // spades
    rank_ = 14;  // Ace from fourth hand — wins the trick
    pos_.rank_in_suit[fourth_][suit_] = bit_map_rank[rank_];
    pos_.aggr[suit_] = bit_map_rank[rank_] | bit_map_rank[2] | bit_map_rank[3] |
                       bit_map_rank[4];
    pos_.length[fourth_][suit_] = 1;
    pos_.hand_dist[fourth_] = kHandDelta[suit_];

    // Snapshot winners that make_3 should save and undo_0 restore.
    pos_.winner[suit_].rank = 14;
    pos_.winner[suit_].hand = fourth_;
    pos_.second_best[suit_].rank = 4;
    pos_.second_best[suit_].hand = HAND_ID(leader_, 2);

    MoveType plays[4] = {
        {suit_, 2, 0, 0},
        {suit_, 3, 0, 0},
        {suit_, 4, 0, 0},
        {suit_, rank_, 0, 0},
    };
    for (int rel = 0; rel < 4; ++rel)
      ctx_->move_gen().make_specific(plays[rel], kTrick, rel);

    fourth_move_ = plays[3];
  }

  std::unique_ptr<SolverContext> ctx_;
  Pos pos_{};
  MoveType fourth_move_{};
  int leader_ = 0;
  int fourth_ = 0;
  int suit_ = 0;
  int rank_ = 0;
};

}  // namespace

TEST_F(Make3Test, RemovesFourthHandCardAndSetsNextLeader)
{
  unsigned short trick_cards[DDS_SUITS] = {1, 1, 1, 1};

  make_3(&pos_, trick_cards, kDepth, &fourth_move_, *ctx_);

  EXPECT_EQ(pos_.rank_in_suit[fourth_][suit_], 0);
  EXPECT_EQ(pos_.length[fourth_][suit_], 0);
  EXPECT_EQ(pos_.hand_dist[fourth_], 0);
  // Ace won; relative winner is 3 → next leader is West.
  EXPECT_EQ(pos_.first[kDepth - 1], fourth_);
  // Four cards in the suit → trickCards records the winning sequence.
  EXPECT_NE(trick_cards[suit_] & bit_map_rank[rank_], 0);
  for (int s = 0; s < DDS_SUITS; ++s)
  {
    if (s != suit_)
      EXPECT_EQ(trick_cards[s], 0);
  }
}

TEST_F(Make3Test, SnapshotsWinnersForPlayedSuits)
{
  unsigned short trick_cards[DDS_SUITS] = {};
  const int saved_winner_rank = pos_.winner[suit_].rank;
  const int saved_winner_hand = pos_.winner[suit_].hand;
  const int saved_second_rank = pos_.second_best[suit_].rank;
  const int saved_second_hand = pos_.second_best[suit_].hand;

  make_3(&pos_, trick_cards, kDepth, &fourth_move_, *ctx_);

  WinnersType const& wp = ctx_->search().winners(kTrick);
  ASSERT_GE(wp.number, 1);
  bool found = false;
  for (int n = 0; n < wp.number; ++n)
  {
    if (wp.winner[n].suit == suit_)
    {
      found = true;
      EXPECT_EQ(wp.winner[n].winnerRank, saved_winner_rank);
      EXPECT_EQ(wp.winner[n].winnerHand, saved_winner_hand);
      EXPECT_EQ(wp.winner[n].secondRank, saved_second_rank);
      EXPECT_EQ(wp.winner[n].secondHand, saved_second_hand);
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(Make3Test, Undo0RestoresCardAndWinners)
{
  unsigned short trick_cards[DDS_SUITS] = {};
  const auto ris = pos_.rank_in_suit[fourth_][suit_];
  const auto aggr = pos_.aggr[suit_];
  const auto len = pos_.length[fourth_][suit_];
  const auto dist = pos_.hand_dist[fourth_];
  const auto win_rank = pos_.winner[suit_].rank;
  const auto win_hand = pos_.winner[suit_].hand;
  const auto sec_rank = pos_.second_best[suit_].rank;
  const auto sec_hand = pos_.second_best[suit_].hand;

  make_3(&pos_, trick_cards, kDepth, &fourth_move_, *ctx_);
  undo_0(&pos_, kDepth, fourth_move_, *ctx_);

  EXPECT_EQ(pos_.rank_in_suit[fourth_][suit_], ris);
  EXPECT_EQ(pos_.aggr[suit_], aggr);
  EXPECT_EQ(pos_.length[fourth_][suit_], len);
  EXPECT_EQ(pos_.hand_dist[fourth_], dist);
  EXPECT_EQ(pos_.winner[suit_].rank, win_rank);
  EXPECT_EQ(pos_.winner[suit_].hand, win_hand);
  EXPECT_EQ(pos_.second_best[suit_].rank, sec_rank);
  EXPECT_EQ(pos_.second_best[suit_].hand, sec_hand);
}
