#pragma once

#include "dds/dds.h"

struct trackType
{
  int leadHand;
  int leadSuit;
  int playSuits[DDS_HANDS];
  int playRanks[DDS_HANDS];
  trickDataType trickData;
  extCard move[DDS_HANDS];
  int high[DDS_HANDS];
  int lowestWin[DDS_HANDS][DDS_SUITS];
  int removedRanks[DDS_SUITS];
};

struct HeuristicContext {
    const pos& tpos;
    const moveType& bestMove;
    const moveType& bestMoveTT;
    const relRanksType* thrp_rel;
    moveType* mply;
    int numMoves;
    int lastNumMoves;
    int trump;
    int suit; // For MoveGen0, the suit being considered
    const trackType* trackp;
    int currTrick;
    int currHand;
    int leadHand;
    int leadSuit; // For MoveGen123
  // Snapshot of per-suit removed ranks for the current trick. This is
  // populated by the caller to avoid relying on the underlying Moves::trackp
  // mutation and to localize mutable heuristic buffers inside the context.
  int removedRanks[DDS_SUITS] = {0};
    // Tiny trick-view snapshots to reduce dependence on trackp for hot helpers.
    // Only the fields required by RankForcesAce are copied for now.
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
void CallHeuristic(const HeuristicContext& context);