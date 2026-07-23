#pragma once

#include <api/dds.h>

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

/// @brief Apply heuristic sorting to candidate moves in the given context.
///
/// Evaluates candidate moves using position-dependent heuristics to assign
/// weights that guide search algorithms toward the most promising lines.
/// Heuristics vary based on position characteristics (leading/following hand,
/// trump presence, suit availability) to optimize move ordering efficiency.
///
/// @param context Pre-constructed HeuristicContext containing position data,
///                move arrays, and cached snapshots for efficient evaluation.
///                Non-const because weighting writes through context.mply.
///
/// @note Prefer the `call_heuristic(context, findex)` overload when the dispatch
///       index is already known (hot path). Use this overload when callers do
///       not have `findex` available.
void call_heuristic(HeuristicContext& context);

/// @brief Apply heuristic sorting using a precomputed dispatch index.
///
/// Move generation already knows the dispatch case; passing it avoids
/// re-deriving the relative hand, trump state and voidness from the context
/// on every call (hot path).
///
/// Encoding (matches the findex used by Moves::MoveGen0 / MoveGen123):
///   - Leading hand:   0 = no trump winner available, 1 = trump winner available
///     (trump in [0, DDS_SUITS) && trump != DDS_NOTRUMP &&
///      winner[trump].rank != 0)
///   - Following hand: 4 * hand_rel + trump_winner + (void_in_lead_suit ? 2 : 0)
///     for hand_rel in 1..3, i.e. values 4..15, where trump_winner is the
///     same 0/1 bit as for the leading hand.
///
/// @param context Pre-constructed HeuristicContext (see other overload).
///                Non-const because weighting writes through context.mply.
/// @param findex  Precomputed dispatch index as encoded above.
void call_heuristic(HeuristicContext& context, int findex);

