#pragma once

#include "dds/dds.h"

struct trackType; // Forward declaration

struct HeuristicContext {
    const pos tpos;
    const moveType bestMove;
    const moveType bestMoveTT;
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
};

void SortMoves(HeuristicContext& context);