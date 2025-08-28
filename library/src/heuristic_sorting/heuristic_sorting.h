#pragma once

#include <cstdint>

#define DDS_HANDS 4
#define DDS_SUITS 4

// A C-style API for a deterministic, side-effect-free move ordering
// heuristic.

// Represents a single card.
struct Card {
  // Suit index (0-3 for spades, hearts, diamonds, clubs).
  int suit;
  // Rank index (2-14 for 2-Ace).
  int rank;
};

// Represents a candidate move to be scored.
struct CandidateMove {
  Card card;
  // A bitmask representing a sequence of cards.
  int sequence;
};

// Represents a scored move, with a weight for sorting.
struct ScoredMove {
  CandidateMove move;
  // The calculated weight for the move. Higher is better.
  int weight;
};

struct Pos {
  unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS];
  unsigned char length[DDS_HANDS][DDS_SUITS];
  unsigned short aggr[DDS_SUITS];
  struct {
    int hand;
    int rank;
  } winner[DDS_SUITS];
  struct {
    int hand;
    int rank;
  } second_best[DDS_SUITS];
};

struct RelRanksType
{
  int aggr;
  int rank[15];
  struct AbsRankType
  {
    int hand;
    int rank;
  } abs_rank[14][DDS_SUITS];
};

struct moveGroupType
{
  int lastGroup;
  int rank[7];
  int sequence[7];
  int fullseq[7];
  int gap[7];
};

// Holds all the necessary context for the heuristic to score moves.
struct HeuristicContext {
  // The trump suit (0-3 for spades, hearts, diamonds, clubs, 4 for no-trump).
  int trump_suit;

  // The hand leading the trick. (0-3 for North, East, South, West).
  int lead_hand;

  // The suit led in the current trick.
  int lead_suit;

  // The current player's hand index (0-3, relative to the lead hand).
  int current_hand_index;

  // The number of tricks played so far.
  int tricks;

  Pos pos;

  // The cards played so far in the trick.
  Card played_cards[3];

  // Best move from transposition table (if any).
  Card best_move;

  // Best move from transposition table (if any).
  Card best_move_tt;

  // Pre-calculated information about the relative ranks of cards.
  const RelRanksType* rel_ranks;

  const int* highest_rank;
  const int* lowest_rank;

  const struct moveGroupType* group_data;

  const unsigned short int* bit_map_rank;
  const int* count_table;

  const int* removed_ranks;
};

// Scores and orders a list of candidate moves based on the provided heuristic
// context.
//
// @param context The heuristic context.
// @param candidate_moves An array of candidate moves to score.
// @param num_candidate_moves The number of candidate moves.
// @param scored_moves An output array to store the scored moves.
// @return The number of scored moves.
int score_and_order(const HeuristicContext& context,
                    const CandidateMove* candidate_moves,
                    int num_candidate_moves,
                    ScoredMove* scored_moves);