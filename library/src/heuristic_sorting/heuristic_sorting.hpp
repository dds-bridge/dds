#pragma once

#include <api/dds_data_types.hpp>

/// @brief Track information for maintaining position state during move generation.
/// 
/// Stores the current trick state, played cards, rankings, and indices used
/// by heuristic sorting routines to order candidate moves effectively.
struct TrackType
{
  int lead_hand;
  int lead_suit;
  int play_suits[DDS_HANDS];
  int play_ranks[DDS_HANDS];
  TrickDataType trick_data;
  ExtCard move[DDS_HANDS];
  int high[DDS_HANDS];
  int lowest_win[DDS_HANDS][DDS_SUITS];
  int removed_ranks[DDS_SUITS];
};

/// @brief Context information for heuristic move sorting and weighting.
///
/// Encapsulates all data needed by heuristic sorting functions to evaluate and
/// weight candidate moves. Combines position state, move arrays, and cached
/// snapshots to minimize repeated lookups during hot path execution.
struct HeuristicContext
{
    // Core position and move generation data
    const Pos& tpos;
    const MoveType& best_move;
    const MoveType& best_move_tt;
    const RelRanksType* thrp_rel;
    MoveType* mply;
    int num_moves;
    int last_num_moves;
    const int trump;
    // For MoveGen0, the suit being considered. Mutable so callers can build
    // the context once per move generation and update it per suit iteration.
    int suit;
    const TrackType* trackp;
    const int curr_trick;
    const int curr_hand;
    const int lead_hand;
    const int lead_suit; // For MoveGen123
    // Snapshot of per-suit removed ranks for the current trick. This is
    // populated by the caller to avoid relying on the underlying Moves::trackp
    // mutation and to localize mutable heuristic buffers inside the context.
    int removed_ranks[DDS_SUITS] = {0};
    // Tiny trick-view snapshots to reduce dependence on trackp for hot helpers.
    // Only the fields required by rank_forces_ace are copied for now.
    int move1_rank = 0; // trackp->move[1].rank
    int high1 = 0;      // trackp->high[1]
    int move1_suit = 0; // trackp->move[1].suit (for some helpers)

    // Third-hand snapshots for weight_alloc_combined_notvoid3 and weight_alloc_trump_void3 helpers.
    int move2_rank = 0; // trackp->move[2].rank
    int move2_suit = 0; // trackp->move[2].suit
    int high2 = 0;      // trackp->high[2]

    // Leader's card snapshot for targeted helpers.
    int lead0_rank = 0; // trackp->move[0].rank
};

/// @brief Which weight_alloc_* helper to run for the current move list.
///
/// Move generation already knows the lead/follow, trump-winner, and void
/// situation; it passes the matching case so call_heuristic does not re-derive
/// it from the context. Numeric values match the historical findex encoding
/// used by Moves::MoveGen0 / MoveGen123 (and DDS_MOVES RegisterList).
enum class WeightCase : int {
  Nt0 = 0,                   ///< Leading hand, no trump winner available
  Trump0 = 1,                ///< Leading hand, trump winner available
  NtNotVoid1 = 4,            ///< 2nd hand, can follow, no trump winner
  TrumpNotVoid1 = 5,         ///< 2nd hand, can follow, trump winner
  NtVoid1 = 6,               ///< 2nd hand, void in lead suit, no trump winner
  TrumpVoid1 = 7,            ///< 2nd hand, void in lead suit, trump winner
  NtNotVoid2 = 8,            ///< 3rd hand, can follow, no trump winner
  TrumpNotVoid2 = 9,         ///< 3rd hand, can follow, trump winner
  NtVoid2 = 10,              ///< 3rd hand, void in lead suit, no trump winner
  TrumpVoid2 = 11,           ///< 3rd hand, void in lead suit, trump winner
  CombinedNotVoid3 = 12,     ///< 4th hand, can follow, no trump winner
  CombinedNotVoid3Trump = 13,///< 4th hand, can follow, trump winner
  NtVoid3 = 14,              ///< 4th hand, void in lead suit, no trump winner
  TrumpVoid3 = 15,           ///< 4th hand, void in lead suit, trump winner
};

/// @brief Apply heuristic sorting using a precomputed weight case.
///
/// Evaluates candidate moves using position-dependent heuristics to assign
/// weights that guide search algorithms toward the most promising lines.
/// Move generation already knows the dispatch case; passing it avoids
/// re-deriving the relative hand, trump state and voidness from the context
/// on every call.
///
/// @param context Pre-constructed HeuristicContext containing position data,
///                move arrays, and cached snapshots for efficient evaluation.
///                Non-const because weighting writes through context.mply.
/// @param weight_case Precomputed weight case (see WeightCase).
void call_heuristic(HeuristicContext& context, WeightCase weight_case);

