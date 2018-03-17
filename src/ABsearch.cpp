/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <sstream>

#include "dds.h"
#include "TransTable.h"
#include "Moves.h"
#include "threadmem.h"
#include "QuickTricks.h"
#include "LaterTricks.h"
#include "ABsearch.h"


#define DDS_POS_LINES 5
#define DDS_HAND_LINES 12
#define DDS_NODE_LINES 4
#define DDS_FULL_LINE 80
#define DDS_HAND_OFFSET 16
#define DDS_HAND_OFFSET2 12
#define DDS_DIAG_WIDTH 34


void Make3Simple(
  pos * posPoint,
  unsigned short int trickCards[DDS_SUITS],
  int depth,
  moveType * mply,
  localVarType * thrp);

void Undo0(
  pos * posPoint,
  int depth,
  moveType * mply,
  localVarType * thrp);

void Undo0Simple(
  pos * posPoint,
  int depth,
  moveType * mply);

void Undo1(
  pos * posPoint,
  int depth,
  moveType * mply);

void Undo2(
  pos * posPoint,
  int depth,
  moveType * mply);

void Undo3(
  pos * posPoint,
  int depth,
  moveType * mply);

void RankToDiagrams(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  nodeCardsType * np,
  char text[DDS_HAND_LINES][DDS_FULL_LINE]);

void WinnersToText(
  unsigned short int winRanks[DDS_SUITS],
  char text[DDS_SUITS][DDS_FULL_LINE]);

void NodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES][DDS_FULL_LINE]);

void FullNodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES][DDS_FULL_LINE]);

void PosToText(
  pos * posPoint,
  int target,
  int depth,
  char text[DDS_POS_LINES][DDS_FULL_LINE]);

void DumpRetrieved(
  FILE * fp,
  pos * posPoint,
  nodeCardsType * np,
  int target,
  int depth);

void DumpStored(
  FILE * fp,
  pos * posPoint,
  Moves * moves,
  nodeCardsType * np,
  int target,
  int depth);


const int handDelta[DDS_SUITS] = { 256, 16, 1, 0 };


// The top-level debugging should possibly go in SolveBoard.cpp
// at some point, but it's so similar to the code here that I
// leave it in this file for now.

void InitFileTopLevel(int thrId)
{
#ifdef DDS_TOP_LEVEL
  localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s",
          DDS_TOP_LEVEL_PREFIX,
          thrId,
          DDS_DEBUG_SUFFIX);

  thrp->fpTopLevel = fopen(fname, "w");
  if (! thrp->fpTopLevel)
    thrp->fpTopLevel = stdout;
#else
  UNUSED(thrId);
#endif
}


void InitFileABstats(int thrId)
{
#ifdef DDS_AB_STATS
  localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s",
          DDS_AB_STATS_PREFIX,
          thrId,
          DDS_DEBUG_SUFFIX);

  thrp->ABStats.SetFile(fname);

  thrp->ABStats.SetName(AB_TARGET_REACHED, "Target decided");
  thrp->ABStats.SetName(AB_DEPTH_ZERO , "depth == 0");
  thrp->ABStats.SetName(AB_QUICKTRICKS , "QuickTricks");
  thrp->ABStats.SetName(AB_LATERTRICKS , "LaterTricks");
  thrp->ABStats.SetName(AB_MAIN_LOOKUP , "Main lookup");
  thrp->ABStats.SetName(AB_SIDE_LOOKUP , "Other lookup");
  thrp->ABStats.SetName(AB_MOVE_LOOP , "Move trial");
#else
  UNUSED(thrId);
#endif
}


void InitFileABhits(int thrId)
{
#ifdef DDS_AB_HITS
  localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s",
          DDS_AB_HITS_RETRIEVED_PREFIX,
          thrId,
          DDS_DEBUG_SUFFIX);

  thrp->fpRetrieved = fopen(fname, "w");
  if (! thrp->fpRetrieved)
    thrp->fpRetrieved = stdout;

  sprintf(fname, "%s%d%s",
          DDS_AB_HITS_STORED_PREFIX,
          thrId,
          DDS_DEBUG_SUFFIX);

  thrp->fpStored = fopen(fname, "w");
  if (! thrp->fpStored)
    thrp->fpStored = stdout;
#else
  UNUSED(thrId);
#endif
}


void InitFileTTstats(int thrId)
{
#ifdef DDS_TT_STATS
  localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s",
          DDS_TT_STATS_PREFIX,
          thrId,
          DDS_DEBUG_SUFFIX);

  thrp->transTable.SetFile(fname);
#else
  UNUSED(thrId);
#endif
}


void InitFileTimer(int thrId)
{
  UNUSED(thrId);
}


void InitFileMoves(int thrId)
{
#ifdef DDS_MOVES
  Moves * movesp = &localVar[thrId].moves;

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s\0",
          DDS_MOVES_PREFIX,
          thrId,
          DDS_DEBUG_SUFFIX);

  movesp->SetFile(fname);
#else
  UNUSED(thrId);
#endif
}


void InitFileScheduler()
{
#ifdef DDS_SCHEDULER
  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%s\0",
          DDS_SCHEDULER_PREFIX,
          DDS_DEBUG_SUFFIX);

  scheduler.SetFile(fname);
#endif
}


void CloseFileTopLevel(int thrId)
{
#ifdef DDS_TOP_LEVEL
  localVarType * thrp = &localVar[thrId];
  if (thrp->fpTopLevel != stdout && thrp->fpTopLevel != nullptr)
    fclose(thrp->fpTopLevel);
#else
  UNUSED(thrId);
#endif
}


void CloseFileABhits(int thrId)
{
#ifdef DDS_AB_HITS
  localVarType * thrp = &localVar[thrId];
  if (thrp->fpStored != stdout && thrp->fpStored != nullptr)
    fclose(thrp->fpStored);
#else
  UNUSED(thrId);
#endif
}


bool ABsearch(
  pos * posPoint,
  int target,
  int depth,
  localVarType * thrp)
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
    posPoint,
    &thrp->bestMove[depth],
    &thrp->bestMoveTT[depth],
    thrp->rel);
  thrp->moves.Purge(tricks, 0, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  moveType * mply;
  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    mply = thrp->moves.MakeNext(tricks, 0,
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
    Undo1(posPoint, depth, mply);
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
  thrp->ABStats.PrintStats();
#endif

  return value;
}


bool ABsearch0(
  pos * posPoint,
  int target,
  int depth,
  localVarType * thrp)
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
    nodeCardsType * cardsP =
      thrp->transTable.Lookup(
        tricks, hand, posPoint->aggr, posPoint->handDist,
        limit, &lowerFlag);
    TIMER_END(TIMER_NO_LOOKUP, depth);

    if (cardsP)
    {
#ifdef DDS_AB_HITS
      DumpRetrieved(thrp->fpRetrieved, posPoint, cardsP, target, depth);
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
  int qtricks = QuickTricks(posPoint, hand, depth, target,
                            trump, &res, thrp);
  TIMER_END(TIMER_NO_QT, depth);

  if (thrp->nodeTypeStore[hand] == MAXNODE)
  {
    if (res)
    {
      AB_COUNT(AB_QUICKTRICKS, 1, depth);
      return (qtricks == 0 ? false : true);
    }

    TIMER_START(TIMER_NO_LT, depth);
    res = LaterTricksMIN(posPoint, hand, depth, target, trump, thrp);
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
    res = LaterTricksMAX(posPoint, hand, depth, target, trump, thrp);
    TIMER_END(TIMER_NO_LT, depth);

    if (res)
    {
      AB_COUNT(AB_LATERTRICKS, false, depth);
      return true;
    }
  }

  /*
  if (depth == 4)
  {
    char text[DDS_HAND_LINES][DDS_FULL_LINE];

    RankToText(posPoint->rankInSuit, text);
    for (int i = 0; i < DDS_HAND_LINES; i++)
      printf("%s\n", text[i]);
    printf("\nTrump %c, leader %c, target %d tricks\n",
      cardSuit[trump],
      cardHand[hand],
      - posPoint->tricksMAX + target);
    printf("----------------------------------\n\n");
  }
  */

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
    nodeCardsType * cardsP =
      thrp->transTable.Lookup(
        tricks, hand, posPoint->aggr, posPoint->handDist,
        limit, &lowerFlag);
    TIMER_END(TIMER_NO_LOOKUP, depth);

    if (cardsP)
    {
#ifdef DDS_AB_HITS
      DumpRetrieved(thrp->fpRetrieved, posPoint, cardsP, target, depth);
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
    posPoint,
    &thrp->bestMove[depth],
    &thrp->bestMoveTT[depth],
    thrp->rel);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  moveType * mply;
  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    mply = thrp->moves.MakeNext(tricks, 0,
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
    Undo1(posPoint, depth, mply);
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
  thrp->transTable.Add(
    tricks,
    hand,
    posPoint->aggr,
    posPoint->winRanks[depth],
    &first,
    flag);
  TIMER_END(TIMER_NO_BUILD, depth);

#ifdef DDS_AB_HITS
  DumpStored(thrp->fpStored, posPoint, &thrp->moves,
             &first, target, depth);
#endif

  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch1(
  pos * posPoint,
  int target,
  int depth,
  localVarType * thrp)
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
  int res = QuickTricksSecondHand(posPoint, hand, depth, target,
                                  trump, thrp);
  TIMER_END(TIMER_NO_QT, depth);
  if (res) return success;

  TIMER_START(TIMER_NO_MOVEGEN, depth);
  for (int ss = 0; ss < DDS_SUITS; ss++)
    thrp->lowestWin[depth][ss] = 0;

  thrp->moves.MoveGen123(tricks, 1, posPoint);
  if (depth == thrp->iniDepth)
    thrp->moves.Purge(tricks, 1, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  moveType * mply;
  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    mply = thrp->moves.MakeNext(tricks, 1,
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
    Undo2(posPoint, depth, mply);
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
  int target,
  int depth,
  localVarType * thrp)
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

  thrp->moves.MoveGen123(tricks, 2, posPoint);
  if (depth == thrp->iniDepth)
    thrp->moves.Purge(tricks, 2, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  moveType * mply;
  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    mply = thrp->moves.MakeNext(tricks, 2,
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
    Undo3(posPoint, depth, mply);
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
  int target,
  int depth,
  localVarType * thrp)
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

  thrp->moves.MoveGen123(tricks, 3, posPoint);
  if (depth == thrp->iniDepth)
    thrp->moves.Purge(tricks, 3, thrp->forbiddenMoves);

  TIMER_END(TIMER_NO_MOVEGEN, depth);

  moveType * mply;

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (1)
  {
    TIMER_START(TIMER_NO_MAKE, depth);
    mply = thrp->moves.MakeNext(tricks, 3,
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
    Undo0(posPoint, depth, mply, thrp);

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
  int depth,
  moveType * mply)
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
  int depth,
  moveType * mply)
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
  int depth,
  moveType * mply)
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
  unsigned short int trickCards[DDS_SUITS],
  int depth,
  moveType * mply,
  localVarType * thrp)
{
  int firstHand = posPoint->first[depth];

  trickDataType * datap = thrp->moves.GetTrickData((depth + 3) >> 2);

  posPoint->first[depth - 1] = handId(firstHand, datap->relWinner);
  /* Defines who is first in the next move */

  int h = handId(firstHand, 3);
  /* Hand pointed to by posPoint->first will lead the next trick */

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int ss = datap->bestSuit;
  if (datap->playCount[ss] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int rr = datap->bestRank;
    trickCards[ss] = static_cast<unsigned short>
                     (bitMapRank[rr] | datap->bestSequence);
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
    if (datap->playCount[st])
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
  unsigned short int trickCards[DDS_SUITS],
  int depth,
  moveType * mply,
  localVarType * thrp)
{
  trickDataType * datap = thrp->moves.GetTrickData((depth + 3) >> 2);

  int firstHand = posPoint->first[depth];

  // Leader of next trick
  posPoint->first[depth - 1] = handId(firstHand, datap->relWinner);

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int s = datap->bestSuit;
  if (datap->playCount[s] >= 2)
  {
    // Win by rank when some else played that suit, too.
    int r = datap->bestRank;
    trickCards[s] = static_cast<unsigned short>
                    (bitMapRank[r] | datap->bestSequence);
  }

  int h = handId(firstHand, 3);
  int r = mply->rank;
  s = mply->suit;

  posPoint->aggr[s] ^= bitMapRank[r];
  posPoint->handDist[h] -= handDelta[s];
}


void Undo0(
  pos * posPoint,
  int depth,
  moveType * mply,
  localVarType * thrp)
{
  int h = handId(posPoint->first[depth], 3);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;

  // Changes that we now undo.
  WinnersType * wp = &thrp->winners[ (depth + 3) >> 2];

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
  int depth,
  moveType * mply)
{
  int h = handId(posPoint->first[depth], 3);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
}


void Undo1(
  pos * posPoint,
  int depth,
  moveType * mply)
{
  int h = posPoint->first[depth];
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


void Undo2(
  pos * posPoint,
  int depth,
  moveType * mply)
{
  int h = handId(posPoint->first[depth], 1);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


void Undo3(
  pos * posPoint,
  int depth,
  moveType * mply)
{
  int h = handId(posPoint->first[depth], 2);
  int s = mply->suit;
  int r = mply->rank;

  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s] |= bitMapRank[r];
  posPoint->handDist[h] += handDelta[s];
  posPoint->length[h][s]++;
}


evalType Evaluate(
  pos * posPoint,
  int trump,
  localVarType * thrp)
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


void RankToText(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  char text[DDS_HAND_LINES][DDS_FULL_LINE])
{
  int c, h, s, r;

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE - 1] = '\0';
  }

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET;
      line = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET;
      line = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET;
      line = 8;
    }
    else
    {
      offset = 0;
      line = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if (rankInSuit[h][s] & bitMapRank[r])
          text[line + s][c++] = static_cast<char>(cardRank[r]);
      }

      if (c == offset)
        text[line + s][c++] = '-';

      if (h != 3)
        text[line + s][c] = '\0';
    }
  }
}


void RankToDiagrams(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  nodeCardsType * np,
  char text[DDS_HAND_LINES][DDS_FULL_LINE])
{
  int c, h, s, r;

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE - 1] = '\0';
    text[l][DDS_DIAG_WIDTH ] = '|';
  }

  strncpy(text[0], "Sought", 6);
  strncpy(&text[0][DDS_DIAG_WIDTH + 5], "Found", 5);

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET2;
      line = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET2;
      line = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET2;
      line = 8;
    }
    else
    {
      offset = 0;
      line = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if (rankInSuit[h][s] & bitMapRank[r])
        {
          text[line + s][c] = static_cast<char>(cardRank[r]);
          text[line + s][c + DDS_DIAG_WIDTH + 5] =
            (r >= 15 - np->leastWin[s] ?
             static_cast<char>(cardRank[r]) : 'x');
          c++;
        }
      }

      if (c == offset)
      {
        text[line + s][c] = '-';
        text[line + s][c + DDS_DIAG_WIDTH + 5] = '-';
        c++;
      }

      if (h != 3)
        text[line + s][c + DDS_DIAG_WIDTH + 5] = '\0';
    }
  }
}


void WinnersToText(
  unsigned short int ourWinRanks[DDS_SUITS],
  char text[DDS_SUITS][DDS_FULL_LINE])
{
  int c, s, r;

  for (int l = 0; l < DDS_SUITS; l++)
    memset(text[l], ' ', DDS_FULL_LINE);

  for (s = 0; s < DDS_SUITS; s++)
  {
    text[s][0] = static_cast<char>(cardSuit[s]);

    c = 2;
    for (r = 14; r >= 2; r--)
    {
      if (ourWinRanks[s] & bitMapRank[r])
        text[s][c++] = static_cast<char>(cardRank[r]);
    }
    text[s][c] = '\0';
  }
}


void NodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES - 1][DDS_FULL_LINE])

{
  sprintf(text[0], "Address\t\t%p\n", static_cast<void *>(np));

  sprintf(text[1], "Bounds\t\t%d to %d tricks\n",
          static_cast<int>(np->lbound),
          static_cast<int>(np->ubound));

  sprintf(text[2], "Best move\t%c%c\n",
          cardSuit[ static_cast<int>(np->bestMoveSuit) ],
          cardRank[ static_cast<int>(np->bestMoveRank) ]);

}


void FullNodeToText(
  nodeCardsType * np,
  char text[DDS_NODE_LINES][DDS_FULL_LINE])

{
  sprintf(text[0], "Address\t\t%p\n", static_cast<void *>(np));

  sprintf(text[1], "Lowest used\t%c%c, %c%c, %c%c, %c%c\n",
          cardSuit[0], cardRank[ 15 - static_cast<int>(np->leastWin[0]) ],
          cardSuit[1], cardRank[ 15 - static_cast<int>(np->leastWin[1]) ],
          cardSuit[2], cardRank[ 15 - static_cast<int>(np->leastWin[2]) ],
          cardSuit[3], cardRank[ 15 - static_cast<int>(np->leastWin[3]) ]);

  sprintf(text[2], "Bounds\t\t%d to %d tricks\n",
          static_cast<int>(np->lbound),
          static_cast<int>(np->ubound));

  sprintf(text[3], "Best move\t%c%c\n",
          cardSuit[ static_cast<int>(np->bestMoveSuit) ],
          cardRank[ static_cast<int>(np->bestMoveRank) ]);

}


void PosToText(
  pos * posPoint,
  int target,
  int depth,
  char text[DDS_POS_LINES][DDS_FULL_LINE])
{
  sprintf(text[0], "Target\t\t%d\n" , target);
  sprintf(text[1], "Depth\t\t%d\n" , depth);
  sprintf(text[2], "tricksMAX\t%d\n" , posPoint->tricksMAX);
  sprintf(text[3], "First hand\t%c\n",
          cardHand[ posPoint->first[depth] ]);
  sprintf(text[4], "Next first\t%c\n",
          cardHand[ posPoint->first[depth - 1] ]);
}


void DumpRetrieved(
  FILE * fp,
  pos * posPoint,
  nodeCardsType * np,
  int target,
  int depth)
{
  // Big enough for all uses.
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  fprintf(fp, "Retrieved entry\n");
  fprintf(fp, "---------------\n");

  PosToText(posPoint, target, depth, text);
  for (int i = 0; i < DDS_POS_LINES; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  FullNodeToText(np, text);
  for (int i = 0; i < DDS_NODE_LINES; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  RankToDiagrams(posPoint->rankInSuit, np, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");
}


void DumpStored(
  FILE * fp,
  pos * posPoint,
  Moves * moves,
  nodeCardsType * np,
  int target,
  int depth)
{
  // Big enough for all uses.
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  fprintf(fp, "Stored entry\n");
  fprintf(fp, "------------\n");

  PosToText(posPoint, target, depth, text);
  for (int i = 0; i < DDS_POS_LINES; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  NodeToText(np, text);
  for (int i = 0; i < DDS_NODE_LINES - 1; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  moves->TrickToText((depth >> 2) + 1, text[0]);
  fprintf(fp, "%s", text[0]);
  fprintf(fp, "\n");

  RankToText(posPoint->rankInSuit, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");
}


void DumpTopLevel(
  localVarType * thrp,
  int tricks,
  int lower,
  int upper,
  int printMode)
{
#ifdef DDS_TOP_LEVEL
  char text[DDS_HAND_LINES][DDS_FULL_LINE];
  pos * posPoint = &thrp->lookAheadPos;
  FILE * fp = thrp->fpTopLevel;

  if (printMode == 0)
  {
    // Trying just one target.
    sprintf(text[0], "Single target %d, %s\n",
            tricks,
            "achieved");
  }
  else if (printMode == 1)
  {
    // Looking for best score.
    if (thrp->val)
    {
      sprintf(text[0],
              "Loop target %d, bounds %d .. %d, achieved with move %c%c\n",
              tricks,
              lower,
              upper,
              cardSuit[ thrp->bestMove[thrp->iniDepth].suit ],
              cardRank[ thrp->bestMove[thrp->iniDepth].rank ]);
    }
    else
    {
      sprintf(text[0],
              "Loop target %d, bounds %d .. %d, failed\n",
              tricks,
              lower,
              upper);
    }
  }
  else if (printMode == 2)
  {
    // Looking for other moves with best score.
    if (thrp->val)
    {
      sprintf(text[0],
              "Loop for cards with score %d, achieved with move %c%c\n",
              tricks,
              cardSuit[ thrp->bestMove[thrp->iniDepth].suit ],
              cardRank[ thrp->bestMove[thrp->iniDepth].rank ]);
    }
    else
    {
      sprintf(text[0],
              "Loop for cards with score %d, failed\n",
              tricks);
    }
  }

  size_t l = strlen(text[0]) - 1;

  memset(text[1], '-', l);
  text[1][l] = '\0';
  fprintf(fp, "%s%s\n\n", text[0], text[1]);

  RankToText(posPoint->rankInSuit, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");

  WinnersToText(posPoint->winRanks[ thrp->iniDepth ], text);
  for (int i = 0; i < DDS_SUITS; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");

  fprintf(fp, "%d AB nodes, %d trick nodes\n\n",
          thrp->nodes,
          thrp->trickNodes);
#else
  UNUSED(thrp);
  UNUSED(tricks);
  UNUSED(lower);
  UNUSED(upper);
  UNUSED(printMode);
#endif
}

