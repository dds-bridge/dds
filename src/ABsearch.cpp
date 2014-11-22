/* 
   DDS 2.7.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund   
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
   permission to include a C++ adaptation in DDS.
   						
   The PlayAnalyser analyses the played cards of the deal and presents 
   their double dummy values. The par calculation function DealerPar 
   provides an alternative way of calculating and presenting par 
   results.  Both these functions have been written by Soren Hein.
   He has also made numerous contributions to the code, especially in 
   the initialization part.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
   implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "dds.h"
#include "TransTable.h"
#include "Moves.h"
#include "QuickTricks.h"
#include "LaterTricks.h"
#include "ABsearch.h"


#define DDS_POS_LINES	5
#define DDS_HAND_LINES 	12
#define DDS_NODE_LINES	4
#define DDS_FULL_LINE	80
#define DDS_HAND_OFFSET	16
#define DDS_HAND_OFFSET2 12
#define DDS_DIAG_WIDTH  34


bool ABsearch0(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

bool ABsearch1(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

bool ABsearch2(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

bool ABsearch3(
  struct pos		* posPoint,
  int			target,
  int			depth,
  struct localVarType	* thrp);

struct evalType Evaluate(
  struct pos 		* posPoint,
  int 			trump,
  localVarType		* thrp);

void Make(
  struct pos 		* posPoint,
  unsigned short int 	trickCards[DDS_SUITS],
  int 			depth,
  int 			trump,
  struct movePlyType 	* mply,
  struct localVarType	* thrp);

void Undo(
  struct pos 		* posPoint,
  int 			depth,
  struct movePlyType 	* mply,
  struct localVarType	* thrp);

bool NextMove(
  struct pos 		* posPoint, 
  int 			depth, 
  struct movePlyType 	* mply, 
  struct localVarType	* thrp);

bool NextMoveNew(
  unsigned short int	winRanks[DDS_SUITS],
  unsigned short int	lowestWin[DDS_SUITS],
  struct movePlyType 	* mply);

void RankToText(
  unsigned short int	rankInSuit[DDS_HANDS][DDS_SUITS],
  char			text[DDS_HAND_LINES][DDS_FULL_LINE]);

void RankToDiagrams(
  unsigned short int	rankInSuit[DDS_HANDS][DDS_SUITS],
  struct nodeCardsTYpe	* np,
  char			text[DDS_HAND_LINES][DDS_FULL_LINE]);

void WinnersToText(
  unsigned short int	winRanks[DDS_SUITS],
  char			text[DDS_SUITS][DDS_FULL_LINE]);

void PlyToText(
  struct movePlyType	* mply,
  int			depth,
  int			firstHand,
  char			text[DDS_FULL_LINE]);

void NodeToText(
  struct nodeCardsType	* np,
  char			text[DDS_NODE_LINES][DDS_FULL_LINE]);

void FullNodeToText(
  struct nodeCardsType	* np,
  char			text[DDS_NODE_LINES][DDS_FULL_LINE]);

void PosToText(
  struct pos		* posPoint,
  int			target,
  int			depth,
  char			text[DDS_POS_LINES][DDS_FULL_LINE]);

void DumpRetrieved(
  FILE			* fp,
  struct pos		* posPoint,
  struct nodeCardsType * np,
  int			target,
  int			depth);

void DumpStored(
  FILE			* fp,
  struct pos		* posPoint,
  struct movePlyType	mply[],
  struct nodeCardsType * np,
  int			target,
  int			depth);


const int handDelta[DDS_SUITS] = {  256,  16,  1, 0 };


// The top-level debugging should possibly go in SolveBoard.cpp
// at some point, but it's so similar to the code here that I
// leave it in this file for now.

void InitFileTopLevel(int thrId)
{
#ifdef DDS_TOP_LEVEL
  struct localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s", 
    DDS_TOP_LEVEL_PREFIX, 
    thrId,
    DDS_DEBUG_SUFFIX);

  thrp->fpTopLevel = fopen(fname, "w");
  if (! thrp->fpTopLevel)
    thrp->fpTopLevel = stdout;
#endif
}


void InitFileABstats(int thrId)
{
#ifdef DDS_AB_STATS
  struct localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s", 
    DDS_AB_STATS_PREFIX, 
    thrId,
    DDS_DEBUG_SUFFIX);

  thrp->ABstats.SetFile(fname);

  thrp->ABstats.SetName(AB_TARGET_REACHED, "Target decided");
  thrp->ABstats.SetName(AB_DEPTH_ZERO    , "depth == 0");
  thrp->ABstats.SetName(AB_QUICKTRICKS   , "QuickTricks");
  thrp->ABstats.SetName(AB_LATERTRICKS   , "LaterTricks");
  thrp->ABstats.SetName(AB_MAIN_LOOKUP   , "Main lookup");
  thrp->ABstats.SetName(AB_SIDE_LOOKUP   , "Other lookup");
  thrp->ABstats.SetName(AB_MOVE_LOOP     , "Move trial");
#endif
}


void InitFilesABhits(int thrId)
{
#ifdef DDS_AB_HITS
  struct localVarType * thrp = &localVar[thrId];

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
#endif
}


void InitFileTTstats(int thrId)
{
#ifdef DDS_TT_STATS
  struct localVarType * thrp = &localVar[thrId];

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s", 
    DDS_TT_STATS_PREFIX,
    thrId,
    DDS_DEBUG_SUFFIX);

  thrp->transTable.SetFile(fname);
#endif
}


void InitFileTimer(int thrId)
{
#ifdef DDS_TIMING
  Timer * timerp = &localVar[thrId].timer;

  char fname[DDS_FNAME_LEN];
  sprintf(fname, "%s%d%s\0", 
    DDS_TIMING_PREFIX,
    thrId,
    DDS_DEBUG_SUFFIX);

  timerp->SetFile(fname);

  timerp->SetNames();
#endif
}


void CloseFileTopLevel(localVarType * thrp)
{
#ifdef DDS_TOP_LEVEL
  if (thrp->fpTopLevel != stdout)
    fclose(thrp->fpTopLevel);
#endif
}


void CloseFilesABhits(localVarType * thrp)
{
#ifdef DDS_AB_HITS
  if (thrp->fpStored != stdout)
    fclose(thrp->fpStored);
#endif
}


bool ABsearch(
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType	* thrp)
{
  /* posPoint points to the current look-ahead position,
     target is number of tricks to take for the player,
     depth is the remaining search length, must be positive,
     the value of the subtree is returned.  
     This is a specialized AB function for handRelFirst == 0. */

  struct movePlyType 	* mply = &thrp->movePly[depth];
  unsigned short int 	makeWinRank[DDS_SUITS];

  int trump    = thrp->trump;
  int hand     = posPoint->first[depth];
  int tricks   = depth >> 2;
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value   = ! success;

  assert(posPoint->handRelFirst == 0);

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_MOVEGEN + depth);
  int moveExists = MoveGen(posPoint, depth, trump, mply, thrp);
  TIMER_END(TIMER_MOVEGEN + depth);

  mply->current = 0;

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (moveExists)
  {
    TIMER_START(TIMER_MAKE + depth);
    Make(posPoint, makeWinRank, depth, trump, mply, thrp);
    TIMER_END(TIMER_MAKE + depth);

    TIMER_START(TIMER_AB + depth - 1);
    value = ABsearch1(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_AB + depth - 1);

    TIMER_START(TIMER_UNDO + depth);
    Undo(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_UNDO + depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 
	  posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

      thrp->bestMove[depth] = mply->move[mply->current];
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |= 
        posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NEXTMOVE + depth);
    moveExists = NextMove(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_NEXTMOVE + depth);
  }

ABexit:

  AB_COUNT(AB_MOVE_LOOP, value, depth);
#ifdef DDS_AB_STATS
  thrp->ABstats.PrintStats();
#endif

  return value;
}


bool ABsearch0(
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType	* thrp)
{
  /* posPoint points to the current look-ahead position,
     target is number of tricks to take for the player,
     depth is the remaining search length, must be positive,
     the value of the subtree is returned.  
     This is a specialized AB function for handRelFirst == 0. */

  struct movePlyType 	* mply = &thrp->movePly[depth];
  unsigned short int 	makeWinRank[DDS_SUITS];

  int trump  = thrp->trump;
  int hand   = posPoint->first[depth];
  int tricks = depth >> 2;

  assert(posPoint->handRelFirst == 0);

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

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
    TIMER_START(TIMER_EVALUATE + depth);
    struct evalType evalData = Evaluate(posPoint, trump, thrp);
    TIMER_END(TIMER_EVALUATE + depth);

    bool value = (evalData.tricks >= target ? true : false);

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] = evalData.winRanks[ss];

    AB_COUNT(AB_DEPTH_ZERO, value, depth);
    return value;
  }

  bool res;
  TIMER_START(TIMER_QT + depth);
  int qtricks = QuickTricks(posPoint, hand, depth, target, 
		  trump, &res, thrp);
  TIMER_END(TIMER_QT + depth);

  if (thrp->nodeTypeStore[hand] == MAXNODE)
  {
    if (res)
    {
      AB_COUNT(AB_QUICKTRICKS, 1, depth);
      return (qtricks == 0 ? false : true);
    }

    TIMER_START(TIMER_LT + depth);
    res = LaterTricksMIN(posPoint, hand, depth, target, trump, thrp);
    TIMER_END(TIMER_LT + depth);

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

    TIMER_START(TIMER_LT + depth);
    res = LaterTricksMAX(posPoint, hand, depth, target, trump, thrp);
    TIMER_END(TIMER_LT + depth);

    if (res)
    {
      AB_COUNT(AB_LATERTRICKS, false, depth);
      return true;
    }
  }

  /* Find node that fits the suit lengths */
  int limit;
  if (thrp->nodeTypeStore[0] == MAXNODE)
    limit = target - posPoint->tricksMAX - 1;
  else
    limit = tricks - (target - posPoint->tricksMAX - 1);

  bool lowerFlag;
  TIMER_START(TIMER_LOOKUP + depth);
  struct nodeCardsType * cardsP =
    thrp->transTable.Lookup(
      tricks, hand, posPoint->aggr, posPoint->handDist, 
      limit, &lowerFlag);
  TIMER_END(TIMER_LOOKUP + depth);

  if (cardsP)
  {
AB_LOOKUP_MATCH:

#ifdef DDS_AB_HITS
    DumpRetrieved(thrp->fpRetrieved, posPoint, cardsP, target, depth);
#endif

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] =
        winRanks[ posPoint->aggr[ss] ][ (int)cardsP->leastWin[ss] ];

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

  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value   = ! success;

  TIMER_START(TIMER_MOVEGEN + depth);
  int moveExists = MoveGen(posPoint, depth, trump, mply, thrp);
  TIMER_END(TIMER_MOVEGEN + depth);

  mply->current = 0;

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (moveExists)
  {
    TIMER_START(TIMER_MAKE + depth);
    Make(posPoint, makeWinRank, depth, trump, mply, thrp);
    TIMER_END(TIMER_MAKE + depth);

    TIMER_START(TIMER_AB + depth - 1);
    value = ABsearch1(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_AB + depth - 1);

    TIMER_START(TIMER_UNDO + depth);
    Undo(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_UNDO + depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 
	  posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

      thrp->bestMove[depth] = mply->move[mply->current];
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |= 
        posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NEXTMOVE + depth);
    moveExists = NextMove(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_NEXTMOVE + depth);
  }

ABexit:
  struct nodeCardsType first;
  if (value)
  {
    if (thrp->nodeTypeStore[0] == MAXNODE)
    {
      first.ubound = tricks + 1;
      first.lbound = target - posPoint->tricksMAX;
    }
    else
    {
      first.ubound = tricks + 1 - target + posPoint->tricksMAX;
      first.lbound = 0;
    }
  }
  else
  {
    if (thrp->nodeTypeStore[0] == MAXNODE)
    {
      first.ubound = target - posPoint->tricksMAX - 1;
      first.lbound = 0;
    }
    else
    {
      first.ubound = tricks + 1;
      first.lbound = tricks + 1 - target + posPoint->tricksMAX + 1;
    }
  }

  first.bestMoveSuit = thrp->bestMove[depth].suit;
  first.bestMoveRank = thrp->bestMove[depth].rank;

  bool flag = 
    ((thrp->nodeTypeStore[hand] == MAXNODE && value) ||
     (thrp->nodeTypeStore[hand] == MINNODE && !value)) 
     ? true : false;

  TIMER_START(TIMER_BUILD + depth);
  thrp->transTable.Add(
    tricks,
    hand,
    posPoint->aggr,
    posPoint->winRanks[depth],
    &first,
    flag);
  TIMER_END(TIMER_BUILD + depth);

#ifdef DDS_AB_HITS
  DumpStored(thrp->fpStored, posPoint, thrp->movePly, 
    &first, target, depth);
#endif

  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch1(
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType	* thrp)
{
  /* This is a specialized AB function for handRelFirst == 1. */

  struct movePlyType 	* mply = &thrp->movePly[depth];
  unsigned short int 	makeWinRank[DDS_SUITS];

  int  trump   = thrp->trump;
  int  hand    = handId(posPoint->first[depth], 1);
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value   = ! success;

  assert(posPoint->handRelFirst == 1);

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_QT + depth);
  int res = QuickTricksSecondHand(posPoint, hand, depth, target, 
    trump, thrp);
  TIMER_END(TIMER_QT + depth);

  if (res)
  {
    return success;
  }

  TIMER_START(TIMER_MOVEGEN + depth);
  bool moveExists = 
    (MoveGen(posPoint, depth, trump, mply, thrp) ? true : false);
  TIMER_END(TIMER_MOVEGEN + depth);

  mply->current = 0;

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (moveExists)
  {
    TIMER_START(TIMER_MAKE + depth);
    Make(posPoint, makeWinRank, depth, trump, mply, thrp);
    TIMER_END(TIMER_MAKE + depth);

    TIMER_START(TIMER_AB + depth - 1);
    value = ABsearch2(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_AB + depth - 1);

    TIMER_START(TIMER_UNDO + depth);
    Undo(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_UNDO + depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 
          posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

      thrp->bestMove[depth] = mply->move[mply->current];
      goto ABexit;
    }

    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |= 
        posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NEXTMOVE + depth);
    moveExists = NextMove(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_NEXTMOVE + depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch2(
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType	* thrp)
{
  /* This is a specialized AB function for handRelFirst == 2. */

  struct movePlyType 	* mply = &thrp->movePly[depth];
  unsigned short int 	makeWinRank[DDS_SUITS];

  int  trump   = thrp->trump;
  int  hand    = handId(posPoint->first[depth], 2);
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value   = ! success;

  assert(posPoint->handRelFirst == 2);

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_MOVEGEN + depth);
  bool moveExists = 
    (MoveGen(posPoint, depth, trump, mply, thrp) ? true : false);
  TIMER_END(TIMER_MOVEGEN + depth);

  mply->current = 0;

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

  while (moveExists)
  {
    TIMER_START(TIMER_MAKE + depth);
    Make(posPoint, makeWinRank, depth, trump, mply, thrp);
    TIMER_END(TIMER_MAKE + depth);

    TIMER_START(TIMER_AB + depth - 1);
    value = ABsearch3(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_AB + depth - 1);

    TIMER_START(TIMER_UNDO + depth);
    Undo(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_UNDO + depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 
          posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

      thrp->bestMove[depth] = mply->move[mply->current];
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |= 
        posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NEXTMOVE + depth);
    moveExists = NextMove(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_NEXTMOVE + depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


bool ABsearch3(
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType	* thrp)
{
  /* This is a specialized AB function for handRelFirst == 3. */

  struct movePlyType 	* mply = &thrp->movePly[depth];
  unsigned short int 	makeWinRank[DDS_SUITS];

  int  trump   = thrp->trump;
  int  hand    = handId(posPoint->first[depth], 3);
  bool success = (thrp->nodeTypeStore[hand] == MAXNODE ? true : false);
  bool value   = ! success;

  assert(posPoint->handRelFirst == 3);

#ifdef DDS_TOP_LEVEL
  thrp->nodes++;
#endif

  TIMER_START(TIMER_MOVEGEN + depth);
  bool moveExists = 
    (MoveGen(posPoint, depth, trump, mply, thrp) ? 1 : 0);
  TIMER_END(TIMER_MOVEGEN + depth);

  if ((depth >= 37) && (thrp->iniDepth != depth))
  {
    struct nodeCardsType * cardsP;
    int	   tricks  = depth >> 2;
    bool   mexists = true;
    bool   ready   = false;
    bool   scoreFlag, lowerFlag;

    mply->current = 0;
    while (mexists)
    {
      TIMER_START(TIMER_MAKE + depth);
      Make(posPoint, makeWinRank, depth, trump, mply, thrp);
      TIMER_END(TIMER_MAKE + depth);

      int hfirst = posPoint->first[depth - 1];

      int limit;
      if (thrp->nodeTypeStore[0] == MAXNODE)
        limit = target - posPoint->tricksMAX - 1;
      else
        limit = tricks - (target - posPoint->tricksMAX - 1);

      TIMER_START(TIMER_LOOKUP + depth);
      cardsP = thrp->transTable.Lookup(
	tricks, hfirst, posPoint->aggr, posPoint->handDist, 
	limit, &lowerFlag);
      TIMER_END(TIMER_LOOKUP + depth);

      if (cardsP)
      {
        scoreFlag = 
          (thrp->nodeTypeStore[0] == MAXNODE ? lowerFlag : ! lowerFlag);

        if (((thrp->nodeTypeStore[hand] == MAXNODE) && scoreFlag) ||
            ((thrp->nodeTypeStore[hand] == MINNODE) && ! scoreFlag))
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            posPoint->winRanks[depth][ss] =
              winRanks[ posPoint->aggr[ss] ][ (int)cardsP->leastWin[ss] ];

          if (cardsP->bestMoveRank != 0)
          {
            thrp->bestMoveTT[depth].suit = cardsP->bestMoveSuit;
            thrp->bestMoveTT[depth].rank = cardsP->bestMoveRank;
          }
          for (int ss = 0; ss < DDS_SUITS; ss++)
            posPoint->winRanks[depth][ss] |= makeWinRank[ss];

          TIMER_START(TIMER_UNDO + depth);
          Undo(posPoint, depth, mply, thrp);
          TIMER_END(TIMER_UNDO + depth);

          AB_COUNT(AB_SIDE_LOOKUP, value, depth);
          return scoreFlag;
        }
        else
        {
          mply->move[mply->current].weight += 100;
          ready = true;
        }
      }

      TIMER_START(TIMER_UNDO + depth);
      Undo(posPoint, depth, mply, thrp);
      TIMER_END(TIMER_UNDO + depth);

      if (ready)
        break;

      if (mply->current <= (mply->last - 1))
      {
        mply->current++;
        mexists = true;
      }
      else
        mexists = false;
    }
    if (ready)
      MergeSort(mply->last + 1, mply->move);
  }

  mply->current = 0;

  for (int ss = 0; ss < DDS_SUITS; ss++)
    posPoint->winRanks[depth][ss] = 0;

if (moveExists)

  while (moveExists)
  {
    TIMER_START(TIMER_MAKE + depth);
    Make(posPoint, makeWinRank, depth, trump, mply, thrp);
    TIMER_END(TIMER_MAKE + depth);

    thrp->trickNodes++; // As handRelFirst == 0

    TIMER_START(TIMER_AB + depth - 1);
    value = ABsearch0(posPoint, target, depth - 1, thrp);
    TIMER_END(TIMER_AB + depth - 1);

    TIMER_START(TIMER_UNDO + depth);
    Undo(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_UNDO + depth);

    if (value == success) /* A cut-off? */
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 
          posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

      thrp->bestMove[depth] = mply->move[mply->current];
      goto ABexit;
    }
    for (int ss = 0; ss < DDS_SUITS; ss++)
      posPoint->winRanks[depth][ss] |= 
        posPoint->winRanks[depth - 1][ss] | makeWinRank[ss];

    TIMER_START(TIMER_NEXTMOVE + depth);
    moveExists = NextMove(posPoint, depth, mply, thrp);
    TIMER_END(TIMER_NEXTMOVE + depth);
  }

ABexit:
  AB_COUNT(AB_MOVE_LOOP, value, depth);
  return value;
}


void Make(
  struct pos 		* posPoint, 
  unsigned short int 	trickCards[DDS_SUITS],
  int 			depth, 
  int 			trump, 
  struct movePlyType 	* mply, 
  struct localVarType	* thrp)
{
  int t, u, w;
  int mcurr, q;

  assert((posPoint->handRelFirst >= 0) && 
         (posPoint->handRelFirst <= 3));

  for (int suit = 0; suit < DDS_SUITS; suit++)
    trickCards[suit] = 0;

  int firstHand = posPoint->first[depth];
  int r = mply->current;

  if (posPoint->handRelFirst == 3)          /* This hand is last hand */
  {
    if (mply->move[r].suit == posPoint->move[depth + 1].suit)
    {
      if (mply->move[r].rank > posPoint->move[depth + 1].rank)
      {
        posPoint->move[depth] = mply->move[r];
        posPoint->high[depth] = handId(firstHand, 3);
      }
      else
      {
        posPoint->move[depth] = posPoint->move[depth + 1];
        posPoint->high[depth] = posPoint->high[depth + 1];
      }
    }
    else if (mply->move[r].suit == trump)
    {
      posPoint->move[depth] = mply->move[r];
      posPoint->high[depth] = handId(firstHand, 3);
    }
    else
    {
      posPoint->move[depth] = posPoint->move[depth + 1];
      posPoint->high[depth] = posPoint->high[depth + 1];
    }

    /* Is the trick won by rank? */
    int s = posPoint->move[depth].suit;
    int count = 0;
    if (mply->move[r].suit == s)
      count++;

    for (int e = 1; e <= 3; e++)
    {
      mcurr = thrp->movePly[depth + e].current;
      if (thrp->movePly[depth + e].move[mcurr].suit == s)
      {
        count++;
      }
    }

    if (thrp->nodeTypeStore[posPoint->high[depth]] == MAXNODE)
      posPoint->tricksMAX++;
    posPoint->first[depth - 1] = posPoint->high[depth];   
    /* Defines who is first in the next move */

    t = handId(firstHand, 3);
    posPoint->handRelFirst = 0;      
    /* Hand pointed to by posPoint->first
       will lead the next trick */


    bool done = false;
    for (int d = 3; d >= 0; d--)
    {
      q = handId(firstHand, 3 - d);
      /* Add the moves to removed ranks */
      r = thrp->movePly[depth + d].current;
      w = thrp->movePly[depth + d].move[r].rank;
      u = thrp->movePly[depth + d].move[r].suit;
      posPoint->removedRanks[u] |= bitMapRank[w];

      if (d == 0)
      {
        posPoint->rankInSuit[t][u] &= (~bitMapRank[w]);
	posPoint->aggr[u] ^= bitMapRank[w];
      }

      if ((w == posPoint->winner[u].rank) || 
          (w == posPoint->secondBest[u].rank))
      {
        int aggr = posPoint->aggr[u];

        posPoint->winner[u].rank = thrp->rel[aggr].absRank[1][u].rank;
        posPoint->winner[u].hand = thrp->rel[aggr].absRank[1][u].hand;
        posPoint->secondBest[u].rank = thrp->rel[aggr].absRank[2][u].rank;
        posPoint->secondBest[u].hand = thrp->rel[aggr].absRank[2][u].hand;
      }

      /* Determine win-ranked cards */
      if ((q == posPoint->high[depth]) && (!done))
      {
        done = true;
        if (count >= 2)
        {
          trickCards[u] = bitMapRank[w];
          /* Mark ranks as winning if they are part of a sequence */
          trickCards[u] |= thrp->movePly[depth + d].move[r].sequence;
        }
      }
    }
  }
  else if (posPoint->handRelFirst == 0)   /* Is it the 1st hand? */
  {
    posPoint->first[depth - 1] = firstHand;   
    /* First hand is not changed in next move */

    posPoint->high[depth] = firstHand;
    posPoint->move[depth] = mply->move[r];
    t = firstHand;
    posPoint->handRelFirst = 1;
    r = mply->current;
    u = mply->move[r].suit;
    w = mply->move[r].rank;
    posPoint->rankInSuit[t][u] &= (~bitMapRank[w]);

    posPoint->aggr[u] ^= bitMapRank[w];
  }
  else
  {
    r = mply->current;
    u = mply->move[r].suit;
    w = mply->move[r].rank;
    if (u == posPoint->move[depth + 1].suit)
    {
      if (w > posPoint->move[depth + 1].rank)
      {
        posPoint->move[depth] = mply->move[r];
        posPoint->high[depth] = handId(firstHand, posPoint->handRelFirst);
      }
      else
      {
        posPoint->move[depth] = posPoint->move[depth + 1];
        posPoint->high[depth] = posPoint->high[depth + 1];
      }
    }
    else if (u == trump)
    {
      posPoint->move[depth] = mply->move[r];
      posPoint->high[depth] = handId(firstHand, posPoint->handRelFirst);
    }
    else
    {
      posPoint->move[depth] = posPoint->move[depth + 1];
      posPoint->high[depth] = posPoint->high[depth + 1];
    }

    t = handId(firstHand, posPoint->handRelFirst);
    posPoint->handRelFirst++; /* Current hand is stepped */
    // assert((posPoint->handRelFirst >= 0) && 
           // (posPoint->handRelFirst <= 3));

    posPoint->first[depth - 1] = firstHand;     
    /* First hand is not changed in next move */

    posPoint->rankInSuit[t][u] &= (~bitMapRank[w]);

    posPoint->aggr[u] ^= bitMapRank[w];

  }

  posPoint->length[t][u]--;

  posPoint->handDist[t]   -= handDelta[u];

#ifdef DDS_AB_STATS
  thrp->ABstats.IncrNode(depth);
#endif

  return;
}


void Undo(
  struct pos 		* posPoint, 
  int 			depth, 
  struct movePlyType 	* mply, 
  struct localVarType	* thrp)
{
  int c, h, s, r;

  // One hand backwards.
  posPoint->handRelFirst = (posPoint->handRelFirst + 3) & 0x3;

  int firstHand = posPoint->first[depth];

  if (posPoint->handRelFirst == 0)
  {
    /* 1st hand which won the previous trick */
    h = firstHand;
    c = mply->current;
    s = mply->move[c].suit;
    r = mply->move[c].rank;
  }
  else if (posPoint->handRelFirst == 3)     /* Last hand */
  {
    for (int d = 3; d >= 0; d--)
    {
      /* Delete the moves from removed ranks */
      c = thrp->movePly[depth + d].current;
      r = thrp->movePly[depth + d].move[c].rank;
      s = thrp->movePly[depth + d].move[c].suit;

      posPoint->removedRanks[s] &= (~bitMapRank[r]);

      if (r > posPoint->winner[s].rank)
      {
        posPoint->secondBest[s] = posPoint->winner[s];
        posPoint->winner[s].rank = r;
        posPoint->winner[s].hand = handId(firstHand, 3 - d);
      }
      else if (r > posPoint->secondBest[s].rank)
      {
        posPoint->secondBest[s].rank = r;
        posPoint->secondBest[s].hand = handId(firstHand, 3 - d);
      }
    }
    h = handId(firstHand, 3);


    if (thrp->nodeTypeStore[posPoint->first[depth - 1]] == MAXNODE)
      /* First hand of next trick is winner of the current trick */
      posPoint->tricksMAX--;
  }
  else
  {
    h = handId(firstHand, posPoint->handRelFirst);
    c = mply->current;
    s = mply->move[c].suit;
    r = mply->move[c].rank;
  }

  posPoint->length[h][s]++;
  posPoint->rankInSuit[h][s] |= bitMapRank[r];
  posPoint->aggr[s]          |= bitMapRank[r];
  posPoint->handDist[h]      += handDelta[s];

  return;
}


struct evalType Evaluate(
  struct pos 		* posPoint,
  int 			trump,
  struct localVarType	* thrp)
{
  int s, h, hmax = 0, count = 0, k = 0;
  unsigned short rmax = 0;
  struct evalType eval;

  int firstHand = posPoint->first[0];
  assert((firstHand >= 0) && (firstHand <= 3));

  for (s = 0; s < DDS_SUITS; s++)
    eval.winRanks[s] = 0;

  /* Who wins the last trick? */
  if (trump != DDS_NOTRUMP)             /* Highest trump card wins */
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

    if (rmax > 0)        /* Trumpcard wins */
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
  while (k <= 3)            /* Find the card the 1st hand played */
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


bool NextMove(
  struct pos 		* posPoint, 
  int 			depth, 
  struct movePlyType 	* mply, 
  struct localVarType	* thrp)
{
  /* Returns true if at least one move remains to be
  searched, otherwise FALSE is returned. */

  unsigned short int * lw_list = thrp->lowestWin[depth];
  unsigned short int lw;
  int suit;
  struct moveType currMove = mply->move[mply->current];

  if (lw_list[currMove.suit] == 0)
  {
    /* A small card has not yet been identified for this suit. */
    lw = posPoint->winRanks[depth][currMove.suit];
    if (lw != 0)
      lw = lw & (-lw); /* LSB */
    else
      lw = bitMapRank[15];

    if (bitMapRank[currMove.rank] < lw)
    {
      /* The current move has a small card. */
      lw_list[currMove.suit] = lw;
      while (mply->current <= (mply->last - 1))
      {
        mply->current++;
        if (bitMapRank[mply->move[mply->current].rank] >=
            lw_list[mply->move[mply->current].suit])
          return true;
      }
      return false;
    }
    else
    {
      while (mply->current <= (mply->last - 1))
      {
        mply->current++;
        suit = mply->move[mply->current].suit;
        if ((currMove.suit == suit) || 
	    (bitMapRank[mply->move[mply->current].rank] >=
              lw_list[suit]))
          return true;
      }
      return false;
    }
  }
  else
  {
    while (mply->current <= (mply->last - 1))
    {
      mply->current++;
      if (bitMapRank[mply->move[mply->current].rank] >=
          lw_list[mply->move[mply->current].suit])
        return true;
    }
    return false;
  }
}


void RankToText(
  unsigned short int	rankInSuit[DDS_HANDS][DDS_SUITS],
  char			text[DDS_HAND_LINES][DDS_FULL_LINE])
{
  int c, h, s, r;

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE-1] = '\0';
  }

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET;
      line   = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET;
      line   = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET;
      line   = 8;
    }
    else
    {
      offset = 0;
      line   = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if (rankInSuit[h][s] & bitMapRank[r])
          text[line + s][c++] = cardRank[r];
      }

      if (c == offset)
        text[line + s][c++] = '-';

      if (h != 3)
        text[line + s][c] = '\0';
    }
  }
}


void RankToDiagrams(
  unsigned short int	rankInSuit[DDS_HANDS][DDS_SUITS],
  struct nodeCardsType	* np,
  char			text[DDS_HAND_LINES][DDS_FULL_LINE])
{
  int c, h, s, r;

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE-1] = '\0';
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
      line   = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET2;
      line   = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET2;
      line   = 8;
    }
    else
    {
      offset = 0;
      line   = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if (rankInSuit[h][s] & bitMapRank[r])
	{
          text[line + s][c] = cardRank[r];
          text[line + s][c + DDS_DIAG_WIDTH + 5] = 
	    (r >= 15 - np->leastWin[s] ? cardRank[r] : 'x');
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
  unsigned short int	winRanks[DDS_SUITS],
  char			text[DDS_SUITS][DDS_FULL_LINE])
{
  int c, s, r;

  for (int l = 0; l < DDS_SUITS; l++)
    memset(text[l], ' ', DDS_FULL_LINE);

  for (s = 0; s < DDS_SUITS; s++)
  {
    text[s][0] = cardSuit[s];

    c = 2;
    for (r = 14; r >= 2; r--)
    {
      if (winRanks[s] & bitMapRank[r])
        text[s][c++] = cardRank[r];
    }
    text[s][c] = '\0';
  }
}


void PlyToText(
  struct movePlyType	mply[],
  int			depth,
  int			firstHand,
  char			text[DDS_FULL_LINE])
{
  sprintf(text, "Last trick\t%c: %c%c - %c%c - %c%c - %c%c\n",
    cardHand[firstHand],
     cardSuit[ mply[depth+4].move[ mply[depth+4].current ].suit ],
     cardRank[ mply[depth+4].move[ mply[depth+4].current ].rank ],

     cardSuit[ mply[depth+3].move[ mply[depth+3].current ].suit ],
     cardRank[ mply[depth+3].move[ mply[depth+3].current ].rank ],

     cardSuit[ mply[depth+2].move[ mply[depth+2].current ].suit ],
     cardRank[ mply[depth+2].move[ mply[depth+2].current ].rank ],
    
     cardSuit[ mply[depth+1].move[ mply[depth+1].current ].suit ],
     cardRank[ mply[depth+1].move[ mply[depth+1].current ].rank ]);
}


void NodeToText(
  struct nodeCardsType	* np,
  char			text[DDS_NODE_LINES-1][DDS_FULL_LINE])

{
  sprintf(text[0], "Address\t\t0x%08x\n", np);

  sprintf(text[1], "Bounds\t\t%d to %d tricks\n",
    (int) np->lbound, (int) np->ubound);

  sprintf(text[2], "Best move\t%c%c\n",
    cardSuit[ (int) np->bestMoveSuit ],
    cardRank[ (int) np->bestMoveRank ]);

}


void FullNodeToText(
  struct nodeCardsType	* np,
  char			text[DDS_NODE_LINES][DDS_FULL_LINE])

{
  sprintf(text[0], "Address\t\t0x%08x\n", np);

  sprintf(text[1], "Lowest used\t%c%c, %c%c, %c%c, %c%c\n",
    cardSuit[0], cardRank[ 15 - (int) np->leastWin[0] ],
    cardSuit[1], cardRank[ 15 - (int) np->leastWin[1] ],
    cardSuit[2], cardRank[ 15 - (int) np->leastWin[2] ],
    cardSuit[3], cardRank[ 15 - (int) np->leastWin[3] ]);

  sprintf(text[2], "Bounds\t\t%d to %d tricks\n",
    (int) np->lbound, (int) np->ubound);

  sprintf(text[3], "Best move\t%c%c\n",
    cardSuit[ (int) np->bestMoveSuit ],
    cardRank[ (int) np->bestMoveRank ]);

}


void PosToText(
  struct pos		* posPoint,
  int			target,
  int			depth,
  char			text[DDS_POS_LINES][DDS_FULL_LINE])
{
  sprintf(text[0], "Target\t\t%d\n"  , target);
  sprintf(text[1], "Depth\t\t%d\n"   , depth);
  sprintf(text[2], "tricksMAX\t%d\n" , posPoint->tricksMAX);
  sprintf(text[3], "First hand\t%c\n", 
    cardHand[ posPoint->first[depth] ]);
  sprintf(text[4], "Next first\t%c\n", 
    cardHand[ posPoint->first[depth-1] ]);
}


void DumpRetrieved(
  FILE			* fp,
  struct pos		* posPoint,
  struct nodeCardsType * np,
  int			target,
  int			depth)
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
  FILE			* fp,
  struct pos		* posPoint,
  struct movePlyType	mply[],
  struct nodeCardsType * np,
  int			target,
  int			depth)
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
  for (int i = 0; i < DDS_NODE_LINES-1; i++)
    fprintf(fp, "%s", text[i]);
  fprintf(fp, "\n");

  PlyToText(mply, depth, posPoint->first[depth+3], text[0]);
  fprintf(fp, "%s", text[0]);
  fprintf(fp, "\n");

  RankToText(posPoint->rankInSuit, text);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    fprintf(fp, "%s\n", text[i]);
  fprintf(fp, "\n");
}


void DumpTopLevel(
  struct localVarType	* thrp,
  int			tricks,
  int			lower,
  int 			upper,
  int			printMode)
{
#ifdef DDS_TOP_LEVEL
  char text[DDS_HAND_LINES][DDS_FULL_LINE];
  struct pos * posPoint = &thrp->lookAheadPos;
  FILE   * fp = thrp->fpTopLevel;

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
        cardSuit[ thrp->bestMove[thrp->game.noOfCards-4].suit ],
        cardRank[ thrp->bestMove[thrp->game.noOfCards-4].rank ]);
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
        cardSuit[ thrp->bestMove[thrp->game.noOfCards-4].suit ],
        cardRank[ thrp->bestMove[thrp->game.noOfCards-4].rank ]);
    }
    else
    {
      sprintf(text[0], 
        "Loop for cards with score %d, failed\n",
        tricks);
    }
  }

  int l = strlen(text[0]) - 1;

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
#endif
}


