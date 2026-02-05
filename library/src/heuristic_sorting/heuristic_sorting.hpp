#ifndef DDS_HEURISTIC_SORTING_H
#define DDS_HEURISTIC_SORTING_H

#include <api/dds.h>

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

struct HeuristicContext {
    const Pos& tpos;
    const MoveType& best_move;
    const MoveType& best_move_tt;
    const RelRanksType* thrp_rel;
    MoveType* mply;
    int num_moves;
    int last_num_moves;
    int trump;
    int suit; // For MoveGen0, the suit being considered
    const TrackType* trackp;
    int curr_trick;
    int curr_hand;
    int lead_hand;
    int lead_suit; // For MoveGen123
  // Snapshot of per-suit removed ranks for the current trick. This is
  // populated by the caller to avoid relying on the underlying Moves::trackp
  // mutation and to localize mutable heuristic buffers inside the context.
  int removed_ranks[DDS_SUITS] = {0};
    // Tiny trick-view snapshots to reduce dependence on trackp for hot helpers.
    // Only the fields required by rank_forces_ace are copied for now.
    int move1_rank = 0; // trackp->move[1].rank
  int high1 = 0;      // trackp->high[1]
  int move1_suit = 0; // trackp->move[1].suit (for some helpers)

  // Third-hand snapshots for CombinedNotvoid3 and TrumpVoid3 helpers.
  int move2_rank = 0; // trackp->move[2].rank
  int move2_suit = 0; // trackp->move[2].suit
  int high2 = 0;      // trackp->high[2]

  // Leader's card snapshot for targeted helpers.
  int lead0_rank = 0; // trackp->move[0].rank
};

// Overload that accepts a pre-built context to avoid repeated
// aggregate construction at the call site. Callers that can
// pre-construct a HeuristicContext should use this to reduce
// per-call overhead in hot paths.
void call_heuristic(const HeuristicContext& context);

#endif // DDS_HEURISTIC_SORTING_H