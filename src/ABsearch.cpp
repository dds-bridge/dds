/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <iostream>
#include <sstream>
#include <assert.h>

#include "TransTable.h"
#include "Moves.h"
#include "QuickTricks.h"
#include "LaterTricks.h"
#include "ABsearch.h"
#include "ABstats.h"
#include "TimerList.h"
#include "dump.h"
#include "debug.h"


void Make3Simple(
  pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  moveType const * mply,
  ThreadData * thrp);

void Undo0(
  pos * posPoint,
  const int depth,
  const moveType& mply,
  ThreadData const * thrp);

void Undo0Simple(
  pos * posPoint,
  const int depth,
  const moveType& mply);

void Undo1(
  pos * posPoint,
  const int depth,
  const moveType& mply);

void Undo2(
  pos * posPoint,
  const int depth,
  const moveType& mply);

void Undo3(
  pos * posPoint,
  const int depth,
  const moveType& mply);


const int handDelta[DDS_SUITS] = { 256, 16, 1, 0 };


bool ABsearch(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
{
  /* posPoint points to the current look-ahead position,
     target is number of tricks to take for the player,
     depth is the remaining search length, must be positive,
     the value of the subtree is returned.
     This is a specialized AB function for handRelFirst == 0. */

  int hand = posPoint->first[depth];
  int tricks = depth >> 2;
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value = ! success;

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    thrp->lowestWin[depth][ss] = 0;

  thrp->moves.MoveGen0(
    tricks,
    * posPoint,
    thrp->bestMove[depth],
    thrp->bestMoveTT[depth],
    thrp->rel);
  thrp->moves.Purge(tricks, 0, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    moveType const * mply = thrp->moves.MakeNext(tricks, 0,
      posPoint->winRanks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    Make0(posPoint, depth, mply);

    TIMER_START(TIMER_NO_AB, depth - 1);
    value = ABsearch1(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    Undo1(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] =
          posPoint->winRanks[depth - 1][ss];

      thrp->bestMove[depth] = * mply;
#ifdef DDS_MOVES
      thrp->moves.RegisterHit(tricks, 0);
#endif
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |=
        posPoint->winRanks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:

  AB_COUNT(AB_MOVE_LOOP, value, depth);
#ifdef DDS_AB_STATS
  thrp->ABStats.PrintStats(thrp->fileABstats.GetStream());
#endif

  return value;
}


bool ABsearch0(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
{
  /* posPoint points to the current look-ahead position,
     target is number of tricks to take for the player,
     depth is the remaining search length, must be positive,
     the value of the subtree is returned.
     This is a specialized AB function for handRelFirst == 0. */

  int trump = thrp->trump;
  int hand = posPoint->first[depth];
  int tricks = depth >> 2;

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  if (depth >= 20)
  {
    /* Find node that fits the suit lengths */
    int limit;
    if (thrp->nodeTypeStore[0] == MAXNODE)
      limit = target - posPoint->tricksMAX - 1;
    else
      limit = tricks - (target - posPoint->tricksMAX - 1);

    bool lowerFlag;
    TIMER_START(TIMER_NO_LOOKUP, depth);
    nodeCardsType const * cardsP =
      thrp->transTable->Lookup(
        tricks, hand, posPoint->aggr, posPoint->handDist,
        limit, lowerFlag);
    TIMER_END(TIMER_NO_LOOKUP, depth);

    if (cardsP)
    {
#ifdef DDS_AB_HITS
      DumpRetrieved(thrp->fileRetrieved.GetStream(), 
        * posPoint, cardsP, target, depth);
#endif

      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] =
          winRanks[ posPoint->aggr[ss] ]
          [ static_cast<int>(cardsP->leastWin[ss]) ];

      if (cardsP->bestMoveRank != 0)
      {
        thrp->bestMoveTT[depth].suit = cardsP->bestMoveSuit;
        thrp->bestMoveTT[depth].rank = cardsP->bestMoveRank;
      }

      bool scoreFlag =
        (thrp->nodeTypeStore[0] == MAXNODE ? lowerFlag : ! lowerFlag);

      AB_COUNT(AB_MAIN_LOOKUP, scoreFlag, depth);
      return scoreFlag;
    }
  }

  if (posPoint->tricksMAX >= target)
  {
    AB_COUNT(AB_TARGET_REACHED, true, depth);
    return true;
  }
  else if (posPoint->tricksMAX + tricks + 1 < target)
  {
    AB_COUNT(AB_TARGET_REACHED, false, depth);
    return false;
  }
  else if (depth == 0) /* Maximum depth? */
  {
    TIMER_START(TIMER_NO_EVALUATE, depth);
    evalType evalData = Evaluate(posPoint, trump, thrp);
    TIMER_END(TIMER_NO_EVALUATE, depth);

    bool value = (evalData.tricks >= target ? true : false);

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] = evalData.winRanks[ss];

    AB_COUNT(AB_DEPTH_ZERO, value, depth);
    return value;
  }

  bool res;
  TIMER_START(TIMER_NO_QT, depth);
  int qtricks = QuickTricks(* posPoint, hand, depth, target,
                            trump, res, * thrp);
  TIMER_END(TIMER_NO_QT, depth);

  if (thrp->nodeTypeStore[hand] == MAXNODE)
  {
    if (res)
    {
      AB_COUNT(AB_QUICKTRICKS, 1, depth);
      return (qtricks == 0 ? false : true);
    }

    TIMER_START(TIMER_NO_LT, depth);
    res = LaterTricksMIN(* posPoint, hand, depth, target, trump, * thrp);
    TIMER_END(TIMER_NO_LT, depth);

    if (! res)
    {
      // Is 1 right here?!
      AB_COUNT(AB_LATERTRICKS, true, depth);
      return false;
    }
  }
  else
  {
    if (res)
    {
      AB_COUNT(AB_QUICKTRICKS, false, depth);
      return (qtricks == 0 ? true : false);
    }

    TIMER_START(TIMER_NO_LT, depth);
    res = LaterTricksMAX(* posPoint, hand, depth, target, trump, * thrp);
    TIMER_END(TIMER_NO_LT, depth);

    if (res)
    {
      AB_COUNT(AB_LATERTRICKS, false, depth);
      return true;
    }
  }

  if (depth < 20)
  {
    /* Find node that fits the suit lengths */
    int limit;
    if (thrp->nodeTypeStore[0] == MAXNODE)
      limit = target - posPoint->tricksMAX - 1;
    else
      limit = tricks - (target - posPoint->tricksMAX - 1);

    bool lowerFlag;
    TIMER_START(TIMER_NO_LOOKUP, depth);
    nodeCardsType const * cardsP =
      thrp->transTable->Lookup(
        tricks, hand, posPoint->aggr, posPoint->handDist,
        limit, lowerFlag);
    TIMER_END(TIMER_NO_LOOKUP, depth);

    if (cardsP)
    {
#ifdef DDS_AB_HITS
      DumpRetrieved(thrp->fileRetrieved.GetStream(), 
        * posPoint, * cardsP, target, depth);
#endif

      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] =
          winRanks[ posPoint->aggr[ss] ]
          [ static_cast<int>(cardsP->leastWin[ss]) ];

      if (cardsP->bestMoveRank != 0)
      {
        thrp->bestMoveTT[depth].suit = cardsP->bestMoveSuit;
        thrp->bestMoveTT[depth].rank = cardsP->bestMoveRank;
      }

      bool scoreFlag =
        (thrp->nodeTypeStore[0] == MAXNODE ? lowerFlag : ! lowerFlag);

      AB_COUNT(AB_MAIN_LOOKUP, scoreFlag, depth);
      return scoreFlag;
    }
  }

  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value = ! success;

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    thrp->lowestWin[depth][ss] = 0;

  thrp->moves.MoveGen0(
    tricks,
    * posPoint,
    thrp->bestMove[depth],
    thrp->bestMoveTT[depth],
    thrp->rel);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    moveType const * mply = thrp->moves.MakeNext(tricks, 0,
      posPoint->winRanks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    Make0(posPoint, depth, mply);

    TIMER_START(TIMER_NO_AB, depth - 1);
    value = ABsearch1(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    Undo1(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] =
          posPoint->winRanks[depth - 1][ss];

      thrp->bestMove[depth] = * mply;
#ifdef DDS_MOVES
      thrp->moves.RegisterHit(tricks, 0);
#endif
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |=
        posPoint->winRanks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  nodeCardsType first;
  if (value)
  {
    if (thrp->nodeTypeStore[0] == MAXNODE)
    {
      first.ubound = static_cast<char>(tricks + 1);
      first.lbound = static_cast<char>(target - posPoint->tricksMAX);
    }
    else
    {
      first.ubound = static_cast<char>
                     (tricks + 1 - target + posPoint->tricksMAX);
      first.lbound = 0;
    }
  }
  else
  {
    if (thrp->nodeTypeStore[0] == MAXNODE)
    {
      first.ubound = static_cast<char>
                     (target - posPoint->tricksMAX - 1);
      first.lbound = 0;
    }
    else
    {
      first.ubound = static_cast<char>(tricks + 1);
      first.lbound = static_cast<char>
                     (tricks + 1 - target + posPoint->tricksMAX + 1);
    }
  }

  first.bestMoveSuit = static_cast<char>(thrp->bestMove[depth].suit);
  first.bestMoveRank = static_cast<char>(thrp->bestMove[depth].rank);

  bool flag =
    ((thrp->nodeTypeStore[hand] == MAXNODE && value) ||
     (thrp->nodeTypeStore[hand] == MINNODE && !value))
    ? true : false;

  TIMER_START(TIMER_NO_BUILD, depth);
  thrp->transTable->Add(
    tricks,
    hand,
    posPoint->aggr,
    posPoint->winRanks[depth],
    first,
    flag);
  TIMER_END(TIMER_NO_BUILD, depth);

#ifdef DDS_AB_HITS
  DumpStored(thrp->fileStored.GetStream(), 
    * posPoint, thrp->moves, first, target, depth);
#endif

  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch1(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
{
  int trump = thrp->trump;
  int hand = handId(posPoint->first[depth], 1);
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value = ! success;
  int tricks = (depth + 3) >> 2;

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_NO_QT, depth);
  int res = QuickTricksSecondHand(* posPoint, hand, depth, target,
     trump, * thrp);
  TIMER_END(TIMER_NO_QT, depth);
  if (res) 
  {
    AB_COUNT(AB_QUICKTRICKS_2ND, true, depth);
    return success;
  }

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    thrp->lowestWin[depth][ss] = 0;

  thrp->moves.MoveGen123(tricks, 1, * posPoint);
  if (depth == thrp->iniDepth)
    thrp->moves.Purge(tricks, 1, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    moveType const * mply = thrp->moves.MakeNext(tricks, 1,
      posPoint->winRanks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    Make1(posPoint, depth, mply);

    TIMER_START(TIMER_NO_AB, depth - 1);
    value = ABsearch2(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    Undo2(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] =
          posPoint->winRanks[depth - 1][ss];

      thrp->bestMove[depth] = * mply;
#ifdef DDS_MOVES
      thrp->moves.RegisterHit(tricks, 1);
#endif
      goto ABexit;
    }

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |=
        posPoint->winRanks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch2(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
{
  int hand = handId(posPoint->first[depth], 2);
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value = ! success;
  int tricks = (depth + 3) >> 2;

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    thrp->lowestWin[depth][ss] = 0;

  thrp->moves.MoveGen123(tricks, 2, * posPoint);
  if (depth == thrp->iniDepth)
    thrp->moves.Purge(tricks, 2, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    moveType const * mply = thrp->moves.MakeNext(tricks, 2,
      posPoint->winRanks[depth]);

    if (mply == NULL)
      break;

    Make2(posPoint, depth, mply);

#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    TIMER_START(TIMER_NO_AB, depth - 1);
    value = ABsearch3(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    Undo3(posPoint, depth, * mply);
    TIMER_END(TIMER_NO_UNDO, depth);


    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] =
          posPoint->winRanks[depth - 1][ss];

      thrp->bestMove[depth] = * mply;
#ifdef DDS_MOVES
      thrp->moves.RegisterHit(tricks, 2);
#endif
      goto ABexit;
    }

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |=
        posPoint->winRanks[depth - 1][ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch3(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
{
  /* This is a specialized AB function for handRelFirst == 3. */

  unsigned short int makeWinRank[DDS_SUITS];

  int hand = handId(posPoint->first[depth], 3);
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value = ! success;

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    thrp->lowestWin[depth][ss] = 0;
  int tricks = (depth + 3) >> 2;

  thrp->moves.MoveGen123(tricks, 3, * posPoint);
  if (depth == thrp->iniDepth)
    thrp->moves.Purge(tricks, 3, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    moveType const * mply = thrp->moves.MakeNext(tricks, 3,
      posPoint->winRanks[depth]);
#ifdef DDS_AB_STATS
    thrp->ABStats.IncrNode(depth);
#endif
    TIMER_END(TIMER_NO_MAKE, depth);

    if (mply == NULL)
      break;

    Make3(posPoint, makeWinRank, depth, mply, thrp);

    thrp->trickNodes++; // As handRelFirst == 0

    if (thrp->nodeTypeStore[posPoint->first[depth - 1]] == MAXNODE)
      posPoint->tricksMAX++;

    TIMER_START(TIMER_NO_AB, depth - 1);
    value = ABsearch0(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_NO_AB, depth - 1);

    TIMER_START(TIMER_NO_UNDO, depth);
    Undo0(posPoint, depth, * mply, thrp);

    if (thrp->nodeTypeStore[posPoint->first[depth - 1]] == MAXNODE)
      posPoint->tricksMAX--;

    TIMER_END(TIMER_NO_UNDO, depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = static_cast<unsigned short>(
                                          posPoint->winRanks[depth - 1][ss] | makeWinRank[ss]);

      thrp->bestMove[depth] = * mply;
#ifdef DDS_MOVES
      thrp->moves.RegisterHit(tricks, 3);
#endif
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |=
        posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NO_NEXTMOVE, depth);
    TIMER_END(TIMER_NO_NEXTMOVE, depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


void Make0(
  pos * posPoint,
  const int depth,
  moveType const * mply)
{
  /* First hand is not changed in next move */
  int h = posPoint->first[depth];
  int s = mply->suit;
  int r = mply->rank;

  posPoint->first[depth - 1] = h;
  posPoint->move[depth] = * mply;

  posPoint->rankInSuit[h][s] &= (~bitMapRank[r]);
  posPoint->aggr[s] ^= bitMapRank[r];
  posPoint->handDist[h] -= handDelta[s];
  posPoint->length[h][s]--;
}


void Make1(
  pos * posPoint,
  const int depth,
  moveType const * mply)
{
  /* First hand is not changed in next move */
  int firstHand = posPoint->first[depth];
  posPoint->first[depth - 1] = firstHand;

  int h = handId(firstHand, 1);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rankInSuit[h][s] &= (~bitMapRank[r]);
  posPoint->aggr[s] ^= bitMapRank[r];
  posPoint->handDist[h] -= handDelta[s];
  posPoint->length[h][s]--;
}


void Make2(
  pos * posPoint,
  const int depth,
  moveType const * mply)
{
  /* First hand is not changed in next move */
  int firstHand = posPoint->first[depth];
  posPoint->first[depth - 1] = firstHand;

  int h = handId(firstHand, 2);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rankInSuit[h][s] &= (~bitMapRank[r]);
  posPoint->aggr[s] ^= bitMapRank[r];
  posPoint->handDist[h] -= handDelta[s];
  posPoint->length[h][s]--;
}


void Make3(
  pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  moveType const * mply,
  ThreadData * thrp)
{
  int firstHand = posPoint->first[depth];

  const trickDataType& data = thrp->moves.GetTrickData((depth + 3) >> 2);

  posPoint->first[depth - 1] = handId(firstHand, data.relWinner);
  /* Defines who is first in the next move */

  int h = handId(firstHand, 3);
  /* Hand pointed to by posPoint->first will lead the next trick */

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int ss = data.bestSuit;
  if (data.playCount[ss] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int rr = data.bestRank;
    trickCards[ss] = static_cast<unsigned short>
      (bitMapRank[rr] | data.bestSequence);
  }

  int r = mply->rank;
  int s = mply->suit;
  posPoint->rankInSuit[h][s] &= (~bitMapRank[r]);
  posPoint->aggr[s] ^= bitMapRank[r];
  posPoint->handDist[h] -= handDelta[s];
  posPoint->length[h][s]--;

  // Changes that we may have to undo.
  WinnersType * wp = &thrp->winners[ (depth + 3) >> 2];
  wp->number = 0;

  for (int st = 0; st < 4; st++)
  {
    if (data.playCount[st])
    {
      int n = wp->number;
      wp->winner[n].suit = st;
      wp->winner[n].winnerRank = posPoint->winner[st].rank;
      wp->winner[n].winnerHand = posPoint->winner[st].hand;
      wp->winner[n].secondRank = posPoint->secondBest[st].rank;
      wp->winner[n].secondHand = posPoint->secondBest[st].hand;
      wp->number++;

      int aggr = posPoint->aggr[st];

      posPoint->winner[st].rank = thrp->rel[aggr].absRank[1][st].rank;
      posPoint->winner[st].hand = thrp->rel[aggr].absRank[1][st].hand;
      posPoint->secondBest[st].rank = thrp->rel[aggr].absRank[2][st].rank;
      posPoint->secondBest[st].hand = thrp->rel[aggr].absRank[2][st].hand;

    }
  }
}


void Make3Simple(
  pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  moveType const * mply,
  ThreadData * thrp)
{
  const trickDataType& data = thrp->moves.GetTrickData((depth + 3) >> 2);

  int firstHand = posPoint->first[depth];

  // Leader of next trick
  posPoint->first[depth - 1] = handId(firstHand, data.relWinner);

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int s = data.bestSuit;
  if (data.playCount[s] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int r = data.bestRank;
    trickCards[s] = static_cast<unsigned short>
      (bitMapRank[r] | data.bestSequence);
  }

  int h = handId(firstHand, 3);
  int r = mply->rank;
  s = mply->suit;

  posPoint->aggr[s] ^= bitMapRank[r];
  posPoint->handDist[h] -= handDelta[s];
}


void Undo0(
  pos * posPoint,
  const int depth,
  const moveType& mply,
  ThreadData const * thrp)
{
  int h = handId(posPoint->first[depth], 3);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;

  // Changes that we now undo.
  WinnersType const * wp = &thrp->winners[ (depth + 3) >> 2];

  for (int n = 0; n < wp->number; n++)
  {
    int st = wp->winner[n].suit;
    posPoint->winner[st].rank = wp->winner[n].winnerRank;
    posPoint->winner[st].hand = wp->winner[n].winnerHand;
    posPoint->secondBest[st].rank = wp->winner[n].secondRank;
    posPoint->secondBest[st].hand = wp->winner[n].secondHand;
  }
}


void Undo0Simple(
  pos * posPoint,
  const int depth,
  const moveType& mply)
{
  int h = handId(posPoint->first[depth], 3);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
}


void Undo1(
  pos * posPoint,
  const int depth,
  const moveType& mply)
{
  int h = posPoint->first[depth];
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


void Undo2(
  pos * posPoint,
  const int depth,
  const moveType& mply)
{
  int h = handId(posPoint->first[depth], 1);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


void Undo3(
  pos * posPoint,
  const int depth,
  const moveType& mply)
{
  int h = handId(posPoint->first[depth], 2);
  int s = mply.suit;
  int r = mply.rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


evalType Evaluate(
  pos const * posPoint,
  const int trump,
  ThreadData const * thrp)
{
  int s, h, hmax = 0, count = 0, k = 0;
  unsigned short rmax = 0;
  evalType eval;

  int firstHand = posPoint->first[0];
  assert((firstHand >= 0) && (firstHand <= 3));

  for (s = 0; s < DDS_SUITS; s++)
    eval.winRanks[s] = 0;

  /* Who wins the last trick? */
  if (trump != DDS_NOTRUMP) /* Highest trump card wins */
  {
    for (h = 0; h < DDS_HANDS; h++)
    {
      if (posPoint->rankInSuit[h][trump] != 0)
        count++;
      if (posPoint->rankInSuit[h][trump] > rmax)
      {
        hmax = h;
        rmax = posPoint->rankInSuit[h][trump];
      }
    }

    if (rmax > 0) /* Trumpcard wins */
    {
      if (count >= 2)
        eval.winRanks[trump] = rmax;

      if (thrp->nodeTypeStore[hmax] == MAXNODE)
        goto maxexit;
      else
        goto minexit;
    }
  }

  /* Who has the highest card in the suit played by 1st hand? */

  k = 0;
  while (k <= 3) /* Find the card the 1st hand played */
  {
    if (posPoint->rankInSuit[firstHand][k] != 0) /* Is this the card? */
      break;
    k++;
  }

  assert(k < 4);

  for (h = 0; h < DDS_HANDS; h++)
  {
    if (posPoint->rankInSuit[h][k] != 0)
      count++;
    if (posPoint->rankInSuit[h][k] > rmax)
    {
      hmax = h;
      rmax = posPoint->rankInSuit[h][k];
    }
  }

  if (count >= 2)
    eval.winRanks[k] = rmax;

  if (thrp->nodeTypeStore[hmax] == MAXNODE)
    goto maxexit;
  else
    goto minexit;

maxexit:
  eval.tricks = posPoint->tricksMAX + 1;
  return eval;

minexit:
  eval.tricks = posPoint->tricksMAX;
  return eval;
}

