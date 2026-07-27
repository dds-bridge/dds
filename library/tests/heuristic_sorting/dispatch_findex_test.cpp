/**
 * @file dispatch_findex_test.cpp
 * @brief Behaviour of WeightCase-based heuristic dispatch.
 *
 * Move generation always passes a precomputed WeightCase. This file covers the
 * dispatcher's fallback when an unrecognized case reaches call_heuristic.
 */

#include <gtest/gtest.h>

#include "heuristic_sorting/heuristic_sorting.hpp"
#include "heuristic_sorting/internal.hpp"
#include <api/dds.h>

namespace
{

constexpr int kNumMoves = 4;
constexpr int kNumRelRanks = 8192;
static RelRanksType rel_ranks[kNumRelRanks] = {};

struct DispatchFixture
{
  Pos tpos{};
  MoveType best_move{};
  MoveType best_move_tt{};
  RelRanksType* rel = rel_ranks;
  TrackType track{};
  MoveType moves_expected[kNumMoves]{};
  MoveType moves_dispatched[kNumMoves]{};
};

void fill_position(DispatchFixture& f, const int curr_hand, const int lead_suit)
{
  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      f.tpos.rank_in_suit[h][s] =
        static_cast<unsigned short>(0x0155u << ((h + s) % 3));
      f.tpos.length[h][s] = static_cast<unsigned char>(3 + ((h + s) % 3));
    }
    f.tpos.hand_dist[h] = 13;
  }
  for (int s = 0; s < DDS_SUITS; s++)
  {
    f.tpos.aggr[s] = 0x1FF5;
    f.tpos.winner[s].rank = 14;
    f.tpos.winner[s].hand = (s + 1) % DDS_HANDS;
    f.tpos.second_best[s].rank = 13;
    f.tpos.second_best[s].hand = (s + 2) % DDS_HANDS;
  }

  f.track.lead_suit = lead_suit;
  f.track.move[0] = ExtCard{lead_suit, 8, 0};
  f.track.move[1] = ExtCard{lead_suit, 10, 0};
  f.track.move[2] = ExtCard{lead_suit, 6, 0};
  f.track.high[1] = 1;
  f.track.high[2] = 1;
  for (int s = 0; s < DDS_SUITS; s++)
    f.track.removed_ranks[s] = 0x0022;

  for (int k = 0; k < kNumMoves; k++)
  {
    f.moves_expected[k] = MoveType{lead_suit, 12 - 2 * k, 0, 0};
    f.moves_dispatched[k] = f.moves_expected[k];
  }
}

HeuristicContext make_context(DispatchFixture& f, MoveType* mply,
                              const int curr_hand, const int lead_hand,
                              const int lead_suit)
{
  HeuristicContext ctx = {
    f.tpos,
    f.best_move,
    f.best_move_tt,
    f.rel,
    mply,
    kNumMoves,     // num_moves
    0,             // last_num_moves
    DDS_NOTRUMP,
    lead_suit,     // suit under consideration
    &f.track,
    5,             // curr_trick
    curr_hand,
    lead_hand,
    lead_suit,
  };
  for (int s = 0; s < DDS_SUITS; s++)
    ctx.removed_ranks[s] = f.track.removed_ranks[s];
  ctx.move1_rank = f.track.move[1].rank;
  ctx.high1 = f.track.high[1];
  ctx.move1_suit = f.track.move[1].suit;
  ctx.move2_rank = f.track.move[2].rank;
  ctx.move2_suit = f.track.move[2].suit;
  ctx.high2 = f.track.high[2];
  ctx.lead0_rank = f.track.move[0].rank;
  return ctx;
}

}  // namespace

// Unrecognized WeightCase values must still assign deterministic weights
// (Nt0 fallback), not leave stale/uninitialized move weights that would
// scramble MergeSort.
TEST(DispatchWeightCase, InvalidWeightCaseFallsBackToBasicNt0Weights)
{
  for (const int bad_value : {2, 3, 16, -1, 99})
  {
    DispatchFixture f;
    const int lead_hand = 0;
    fill_position(f, lead_hand, /*lead_suit=*/0);

    constexpr int kStale = 0x7f0f0f0f;
    for (int k = 0; k < kNumMoves; k++)
    {
      f.moves_expected[k].weight = 0;
      f.moves_dispatched[k].weight = kStale;
    }

    HeuristicContext expected = make_context(
      f, f.moves_expected, lead_hand, lead_hand, 0);
    HeuristicContext dispatched = make_context(
      f, f.moves_dispatched, lead_hand, lead_hand, 0);

    weight_alloc_nt0(expected);
    call_heuristic(dispatched, static_cast<WeightCase>(bad_value));

    for (int k = 0; k < kNumMoves; k++)
    {
      EXPECT_NE(f.moves_dispatched[k].weight, kStale)
          << "weight_case=" << bad_value << " move=" << k
          << " left a stale weight";
      EXPECT_EQ(f.moves_expected[k].weight, f.moves_dispatched[k].weight)
          << "weight_case=" << bad_value << " move=" << k
          << " must match weight_alloc_nt0 fallback";
    }
  }
}

// Known WeightCase values must select the matching weight_alloc_* helper.
TEST(DispatchWeightCase, Nt0DispatchesToWeightAllocNt0)
{
  DispatchFixture f;
  const int lead_hand = 0;
  fill_position(f, lead_hand, /*lead_suit=*/0);

  HeuristicContext expected = make_context(
    f, f.moves_expected, lead_hand, lead_hand, 0);
  HeuristicContext dispatched = make_context(
    f, f.moves_dispatched, lead_hand, lead_hand, 0);

  weight_alloc_nt0(expected);
  call_heuristic(dispatched, WeightCase::Nt0);

  for (int k = 0; k < kNumMoves; k++)
    EXPECT_EQ(f.moves_expected[k].weight, f.moves_dispatched[k].weight)
        << "move=" << k;
}
