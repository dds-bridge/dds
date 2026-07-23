/**
 * @file dispatch_findex_test.cpp
 * @brief Equivalence tests for findex-based heuristic dispatch.
 *
 * Move generation already knows the dispatch case (leading vs following
 * hand, trump vs NT, void vs not void) as a small integer. The
 * call_heuristic(ctx, findex) overload must select exactly the same weight
 * function as the legacy overload that re-derives the case from the context,
 * for every reachable findex.
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
  MoveType moves_legacy[kNumMoves]{};
  MoveType moves_findex[kNumMoves]{};
};

// Populate a deterministic pseudo-position that exercises the weight
// arithmetic (non-zero ranks, lengths, winners) without needing to be a
// fully consistent deal: both dispatchers run the same weight function on
// identical inputs, so equal outputs prove equal dispatch.
void fill_position(DispatchFixture& f, const bool trump_game, const bool is_void,
                   const int curr_hand, const int lead_suit)
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

  const int trump = trump_game ? 1 : DDS_NOTRUMP;
  if (trump_game)
    f.tpos.winner[trump].rank = 14;

  if (is_void)
  {
    f.tpos.rank_in_suit[curr_hand][lead_suit] = 0;
    f.tpos.length[curr_hand][lead_suit] = 0;
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
    const int suit = is_void ? ((lead_suit + 1) % DDS_SUITS) : lead_suit;
    f.moves_legacy[k] = MoveType{suit, 12 - 2 * k, 0, 0};
    f.moves_findex[k] = f.moves_legacy[k];
  }
}

HeuristicContext make_context(DispatchFixture& f, MoveType* mply,
                              const int trump, const bool is_void,
                              const int curr_hand, const int lead_hand,
                              const int lead_suit)
{
  const int move_suit = is_void ? ((lead_suit + 1) % DDS_SUITS) : lead_suit;
  HeuristicContext ctx = {
    f.tpos,
    f.best_move,
    f.best_move_tt,
    f.rel,
    mply,
    kNumMoves,     // num_moves
    0,             // last_num_moves
    trump,
    move_suit,     // suit under consideration
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

void expect_same_weights(const DispatchFixture& f, const int findex)
{
  for (int k = 0; k < kNumMoves; k++)
    EXPECT_EQ(f.moves_legacy[k].weight, f.moves_findex[k].weight)
        << "findex=" << findex << " move=" << k;
}

}  // namespace

// Leading hand: findex 0 (NT) and 1 (trump winner available) must match the
// legacy dispatch, which selects the weight function from the trump/winner
// state — not merely whether a trump suit is set.
TEST(DispatchFindex, LeadHandMatchesLegacyDispatch)
{
  for (const bool trump_game : {false, true})
  {
    DispatchFixture f;
    const int lead_hand = 2;
    fill_position(f, trump_game, /*is_void=*/false, lead_hand, /*lead_suit=*/0);

    const int trump = trump_game ? 1 : DDS_NOTRUMP;
    HeuristicContext legacy = make_context(
      f, f.moves_legacy, trump, false, lead_hand, lead_hand, 0);
    HeuristicContext with_findex = make_context(
      f, f.moves_findex, trump, false, lead_hand, lead_hand, 0);

    call_heuristic(legacy);
    call_heuristic(with_findex, trump_game ? 1 : 0);

    expect_same_weights(f, trump_game ? 1 : 0);
  }
}

// Out-of-range trump must not index winner[trump]; treat as no trump winner
// (same range guard Moves::MoveGen0 / MoveGen123 must apply).
TEST(DispatchFindex, LegacyTreatsOutOfRangeTrumpAsNoWinner)
{
  // DDS_NOTRUMP == DDS_SUITS, so use values strictly outside [0, DDS_SUITS).
  for (const int bad_trump : {-1, DDS_SUITS + 1, 99})
  {
    DispatchFixture f;
    const int lead_hand = 0;
    fill_position(f, /*trump_game=*/false, /*is_void=*/false, lead_hand,
                  /*lead_suit=*/0);

    HeuristicContext legacy = make_context(
      f, f.moves_legacy, bad_trump, false, lead_hand, lead_hand, 0);
    HeuristicContext with_findex = make_context(
      f, f.moves_findex, DDS_NOTRUMP, false, lead_hand, lead_hand, 0);

    call_heuristic(legacy);
    call_heuristic(with_findex, /*findex=*/0);

    expect_same_weights(f, /*findex=*/0);
  }
}

// findex bit 0/1 is "trump winner available", not "trump suit set".
// A trump deal with winner[trump].rank == 0 must dispatch as findex 0.
TEST(DispatchFindex, LeadHandTrumpWithoutWinnerUsesFindex0)
{
  DispatchFixture f;
  const int lead_hand = 2;
  fill_position(f, /*trump_game=*/true, /*is_void=*/false, lead_hand,
                /*lead_suit=*/0);
  f.tpos.winner[1].rank = 0;

  HeuristicContext legacy = make_context(
    f, f.moves_legacy, /*trump=*/1, false, lead_hand, lead_hand, 0);
  HeuristicContext with_findex = make_context(
    f, f.moves_findex, /*trump=*/1, false, lead_hand, lead_hand, 0);

  call_heuristic(legacy);
  call_heuristic(with_findex, /*findex=*/0);

  expect_same_weights(f, /*findex=*/0);

  // Passing 1 ("trump game") would select the wrong weight function.
  for (int k = 0; k < kNumMoves; k++)
    f.moves_findex[k].weight = 0;
  call_heuristic(with_findex, /*findex=*/1);
  bool any_differ = false;
  for (int k = 0; k < kNumMoves; k++)
  {
    if (f.moves_legacy[k].weight != f.moves_findex[k].weight)
      any_differ = true;
  }
  EXPECT_TRUE(any_differ)
      << "findex 1 must not match when trump is set but no winner is available";
}

// Out-of-range findex must still assign deterministic weights (NT0 fallback),
// not leave stale/uninitialized move weights that would scramble MergeSort.
TEST(DispatchFindex, InvalidFindexFallsBackToBasicNt0Weights)
{
  for (const int bad_findex : {2, 3, 16, -1, 99})
  {
    DispatchFixture f;
    const int lead_hand = 0;
    fill_position(f, /*trump_game=*/false, /*is_void=*/false, lead_hand,
                  /*lead_suit=*/0);

    constexpr int kStale = 0x7f0f0f0f;
    for (int k = 0; k < kNumMoves; k++)
    {
      f.moves_legacy[k].weight = 0;
      f.moves_findex[k].weight = kStale;
    }

    HeuristicContext expected = make_context(
      f, f.moves_legacy, DDS_NOTRUMP, false, lead_hand, lead_hand, 0);
    HeuristicContext with_findex = make_context(
      f, f.moves_findex, DDS_NOTRUMP, false, lead_hand, lead_hand, 0);

    weight_alloc_nt0(expected);
    call_heuristic(with_findex, bad_findex);

    for (int k = 0; k < kNumMoves; k++)
    {
      EXPECT_NE(f.moves_findex[k].weight, kStale)
          << "findex=" << bad_findex << " move=" << k
          << " left a stale weight";
      EXPECT_EQ(f.moves_legacy[k].weight, f.moves_findex[k].weight)
          << "findex=" << bad_findex << " move=" << k
          << " must match weight_alloc_nt0 fallback";
    }
  }
}

// Following hands: findex 4..15 (4*hand_rel + trump_winner + 2*void) must match
// the legacy dispatch that re-derives hand_rel, trump-winner state and voidness.
TEST(DispatchFindex, FollowingHandsMatchLegacyDispatchForAllFindexes)
{
  const int lead_hand = 1;
  const int lead_suit = 2;

  for (int hand_rel = 1; hand_rel <= 3; hand_rel++)
  {
    for (const bool trump_game : {false, true})
    {
      for (const bool is_void : {false, true})
      {
        const int curr_hand = (lead_hand + hand_rel) % 4;
        const int findex =
          4 * hand_rel + (trump_game ? 1 : 0) + (is_void ? 2 : 0);

        DispatchFixture f;
        fill_position(f, trump_game, is_void, curr_hand, lead_suit);

        const int trump = trump_game ? 1 : DDS_NOTRUMP;
        HeuristicContext legacy = make_context(
          f, f.moves_legacy, trump, is_void, curr_hand, lead_hand,
          lead_suit);
        HeuristicContext with_findex = make_context(
          f, f.moves_findex, trump, is_void, curr_hand, lead_hand,
          lead_suit);

        call_heuristic(legacy);
        call_heuristic(with_findex, findex);

        expect_same_weights(f, findex);
      }
    }
  }
}
