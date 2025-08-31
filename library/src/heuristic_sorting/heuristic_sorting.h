#pragma once

#include "../dds.h"
#include "../dll.h"

// Forward declarations for types from Moves.h that we need
struct trickDataType
{
  int playCount[DDS_SUITS];
  int bestRank;
  int bestSuit;
  int bestSequence;
  int relWinner;
  int nextLeadHand;
};

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

// Integration function for calling from moves.cpp
void CallHeuristic(
    const pos& tpos,
    const moveType& bestMove,
    const moveType& bestMoveTT,
    const relRanksType thrp_rel[],
    moveType* mply,
    int numMoves,
    int lastNumMoves,
    int trump,
    int suit,
    const trackType* trackp,
    int currTrick,
    int currHand,
    int leadHand,
    int leadSuit);