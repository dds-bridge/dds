/// @file make_undo_test.cpp
/// @brief Characterization tests for AB make/undo of cards in hands 0-2.
///
/// Pins card-removal and restore behavior for make_0/1/2 and undo_1/2/3
/// before consolidating their duplicated implementations.

#include <cstring>

#include <gtest/gtest.h>

#include <ab_search.hpp>
#include <api/dds.h>
#include <utility/constants.h>

namespace {

constexpr int kDepth = 4;
constexpr int kHandDelta[DDS_SUITS] = {256, 16, 1, 0};

struct CardFixture {
  Pos pos{};
  MoveType move{};
  int leader = 0;
  int player = 0;
  int suit = 0;
  int rank = 14;  // Ace

  CardFixture(int first_hand, int relative_hand, int suit, int rank)
      : leader(first_hand),
        player(HAND_ID(first_hand, relative_hand)),
        suit(suit),
        rank(rank)
  {
    std::memset(&pos, 0, sizeof(pos));
    pos.first[kDepth] = leader;
    pos.rank_in_suit[player][suit] = bit_map_rank[rank];
    pos.aggr[suit] = bit_map_rank[rank];
    pos.length[player][suit] = 1;
    pos.hand_dist[player] = kHandDelta[suit];
    move = MoveType{suit, rank, 0, 0};
  }
};

}  // namespace

class MakeUndoHands012 : public ::testing::Test {};

TEST_F(MakeUndoHands012, Make0RemovesLeadersCardAndStoresMove)
{
  CardFixture fx(/*first*/ 2, /*rel*/ 0, /*suit*/ 1, /*rank*/ 13);

  make_0(&fx.pos, kDepth, &fx.move);

  EXPECT_EQ(fx.pos.first[kDepth - 1], fx.leader);
  EXPECT_EQ(fx.pos.move[kDepth].suit, fx.suit);
  EXPECT_EQ(fx.pos.move[kDepth].rank, fx.rank);
  EXPECT_EQ(fx.pos.rank_in_suit[fx.player][fx.suit], 0);
  EXPECT_EQ(fx.pos.aggr[fx.suit], 0);
  EXPECT_EQ(fx.pos.length[fx.player][fx.suit], 0);
  EXPECT_EQ(fx.pos.hand_dist[fx.player], 0);
}

TEST_F(MakeUndoHands012, Make1RemovesSecondHandsCard)
{
  CardFixture fx(/*first*/ 0, /*rel*/ 1, /*suit*/ 0, /*rank*/ 14);

  make_1(&fx.pos, kDepth, &fx.move);

  EXPECT_EQ(fx.pos.first[kDepth - 1], fx.leader);
  EXPECT_EQ(fx.pos.rank_in_suit[fx.player][fx.suit], 0);
  EXPECT_EQ(fx.pos.aggr[fx.suit], 0);
  EXPECT_EQ(fx.pos.length[fx.player][fx.suit], 0);
  EXPECT_EQ(fx.pos.hand_dist[fx.player], 0);
}

TEST_F(MakeUndoHands012, Make2RemovesThirdHandsCard)
{
  CardFixture fx(/*first*/ 1, /*rel*/ 2, /*suit*/ 2, /*rank*/ 10);

  make_2(&fx.pos, kDepth, &fx.move);

  EXPECT_EQ(fx.pos.first[kDepth - 1], fx.leader);
  EXPECT_EQ(fx.pos.rank_in_suit[fx.player][fx.suit], 0);
  EXPECT_EQ(fx.pos.aggr[fx.suit], 0);
  EXPECT_EQ(fx.pos.length[fx.player][fx.suit], 0);
  EXPECT_EQ(fx.pos.hand_dist[fx.player], 0);
}

TEST_F(MakeUndoHands012, Undo1RestoresAfterMake0)
{
  CardFixture fx(/*first*/ 3, /*rel*/ 0, /*suit*/ 3, /*rank*/ 7);
  const auto ris = fx.pos.rank_in_suit[fx.player][fx.suit];
  const auto aggr = fx.pos.aggr[fx.suit];
  const auto len = fx.pos.length[fx.player][fx.suit];
  const auto dist = fx.pos.hand_dist[fx.player];

  make_0(&fx.pos, kDepth, &fx.move);
  undo_1(&fx.pos, kDepth, fx.move);

  EXPECT_EQ(fx.pos.rank_in_suit[fx.player][fx.suit], ris);
  EXPECT_EQ(fx.pos.aggr[fx.suit], aggr);
  EXPECT_EQ(fx.pos.length[fx.player][fx.suit], len);
  EXPECT_EQ(fx.pos.hand_dist[fx.player], dist);
}

TEST_F(MakeUndoHands012, Undo2RestoresAfterMake1)
{
  CardFixture fx(/*first*/ 2, /*rel*/ 1, /*suit*/ 0, /*rank*/ 12);
  const auto ris = fx.pos.rank_in_suit[fx.player][fx.suit];
  const auto aggr = fx.pos.aggr[fx.suit];
  const auto len = fx.pos.length[fx.player][fx.suit];
  const auto dist = fx.pos.hand_dist[fx.player];

  make_1(&fx.pos, kDepth, &fx.move);
  undo_2(&fx.pos, kDepth, fx.move);

  EXPECT_EQ(fx.pos.rank_in_suit[fx.player][fx.suit], ris);
  EXPECT_EQ(fx.pos.aggr[fx.suit], aggr);
  EXPECT_EQ(fx.pos.length[fx.player][fx.suit], len);
  EXPECT_EQ(fx.pos.hand_dist[fx.player], dist);
}

TEST_F(MakeUndoHands012, Undo3RestoresAfterMake2)
{
  CardFixture fx(/*first*/ 0, /*rel*/ 2, /*suit*/ 1, /*rank*/ 9);
  const auto ris = fx.pos.rank_in_suit[fx.player][fx.suit];
  const auto aggr = fx.pos.aggr[fx.suit];
  const auto len = fx.pos.length[fx.player][fx.suit];
  const auto dist = fx.pos.hand_dist[fx.player];

  make_2(&fx.pos, kDepth, &fx.move);
  undo_3(&fx.pos, kDepth, fx.move);

  EXPECT_EQ(fx.pos.rank_in_suit[fx.player][fx.suit], ris);
  EXPECT_EQ(fx.pos.aggr[fx.suit], aggr);
  EXPECT_EQ(fx.pos.length[fx.player][fx.suit], len);
  EXPECT_EQ(fx.pos.hand_dist[fx.player], dist);
}

TEST_F(MakeUndoHands012, MakeDoesNotAffectOtherHands)
{
  CardFixture fx(/*first*/ 0, /*rel*/ 1, /*suit*/ 0, /*rank*/ 14);
  // Give another hand a different card in the same suit.
  const int other = HAND_ID(fx.leader, 2);
  fx.pos.rank_in_suit[other][fx.suit] = bit_map_rank[13];
  fx.pos.aggr[fx.suit] |= bit_map_rank[13];
  fx.pos.length[other][fx.suit] = 1;
  fx.pos.hand_dist[other] = kHandDelta[fx.suit];

  make_1(&fx.pos, kDepth, &fx.move);

  EXPECT_EQ(fx.pos.rank_in_suit[other][fx.suit], bit_map_rank[13]);
  EXPECT_EQ(fx.pos.length[other][fx.suit], 1);
  EXPECT_EQ(fx.pos.hand_dist[other], kHandDelta[fx.suit]);
  EXPECT_EQ(fx.pos.aggr[fx.suit], bit_map_rank[13]);
}
