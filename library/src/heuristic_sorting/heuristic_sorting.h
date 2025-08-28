#pragma once

#include "dds/dds.h"

/**
 * @brief A C-style API for a deterministic, side-effect-free move ordering
 * heuristic.
 */

/**
 * @brief Represents a candidate move to be scored.
 * This is an alias for the extCard struct from dds.h.
 */
typedef extCard CandidateMove;

/**
 * @brief Represents a scored move, with a weight for sorting.
 */
struct ScoredMove {
  /** @brief The candidate move. */
  CandidateMove move;
  /** @brief The calculated weight for the move. Higher is better. */
  int weight;
};

/**
 * @brief Holds all the necessary context for the heuristic to score moves.
 */
struct HeuristicContext {
  /** @brief The trump suit (0-3 for spades, hearts, diamonds, clubs, 4 for no-trump). */
  int trump_suit;

  /** @brief The hand leading the trick. (0-3 for North, East, South, West). */
  int lead_hand;

  /** @brief The suit led in the current trick. */
  int lead_suit;

  /** @brief The current player's hand index (0-3, relative to the lead hand). */
  int current_hand_index;

  /** @brief The number of tricks played so far. */
  int tricks;

  /** @brief The current state of the cards. */
  const pos* tpos;

  /** @brief The cards played so far in the trick. */
  card played_cards[3];

  /** @brief Best move from transposition table (if any). */
  moveType best_move;

  /** @brief Best move from transposition table (if any). */
  moveType best_move_tt;

  /** @brief Pre-calculated information about the relative ranks of cards. */
  const relRanksType* rel_ranks;

  const int* highest_rank;
  const int* lowest_rank;

  const struct moveGroupType* group_data;

  const unsigned short int* bit_map_rank;
  const int* count_table;

  const int* removed_ranks;
};

/**
 * @brief Scores and orders a list of candidate moves based on the provided heuristic
 * context.
 *
 * @param context The heuristic context.
 * @param candidate_moves An array of candidate moves to score.
 * @param num_candidate_moves The number of candidate moves.
 * @param scored_moves An output array to store the scored moves.
 * @return The number of scored moves.
 */
int score_and_order(const HeuristicContext& context,
                    const CandidateMove* candidate_moves,
                    int num_candidate_moves,
                    ScoredMove* scored_moves);