/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <iostream>
#include <iomanip>
#include <sstream>

#include "SolverIF.h"
#include "Init.h"
#include "ABsearch.h"
#include "TimerList.h"
#include "System.h"
#include "Scheduler.h"
#include "dump.h"
#include "debug.h"

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;


int BoardRangeChecks(
  const deal& dl,
  const int target,
  const int solutions,
  const int mode);

int BoardValueChecks(
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  ThreadData const * thrp);

void LastTrickWinner(
  const deal& dl,
  ThreadData const * thrp,
  const int handToPlay,
  const int handRelFirst,
  int& leadRank,
  int& leadSuit,
  int& leadSideWins);

bool (* AB_ptr_list[DDS_HANDS])(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
  = { ABsearch, ABsearch1, ABsearch2, ABsearch3 };

bool (* AB_ptr_trace_list[DDS_HANDS])(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp)
  = { ABsearch0, ABsearch1, ABsearch2, ABsearch3 };

void (* Make_ptr_list[3])(
  pos * posPoint,
  const int depth,
  moveType const * mply)
  = { Make0, Make1, Make2 };


int STDCALL SolveBoard(
  deal dl,
  int target,
  int solutions,
  int mode,
  futureTricks * futp,
  int thrId)
{
  if (! sysdep.ThreadOK(thrId))
    return RETURN_THREAD_INDEX;

  return SolveBoardInternal(memory.GetPtr(static_cast<unsigned>(thrId)), 
    dl, target, solutions, mode, futp);
}


int SolveBoardInternal(
  ThreadData * thrp,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  futureTricks * futp)
{
  // ----------------------------------------------------------
  // Formal parameter checks.
  // ----------------------------------------------------------

  int ret = BoardRangeChecks(dl, target, solutions, mode);
  if (ret != RETURN_NO_FAULT)
    return ret;

  // ----------------------------------------------------------
  // Count and classify deal.
  // ----------------------------------------------------------

  bool newDeal = false;
  bool newTrump = false;
  unsigned diffDeal = 0;
  unsigned aggDeal = 0;
  bool similarDeal;
  int cardCount = 0;
  int ind, forb, noMoves;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      unsigned int c = dl.remainCards[h][s] >> 2;

      cardCount += counttable[c];
      diffDeal += (c ^ (thrp->suit[h][s]));
      aggDeal += c;

      if (thrp->suit[h][s] != c)
      {
        thrp->suit[h][s] = static_cast<unsigned short>(c);
        newDeal = true;
      }
    }
  }

  if (newDeal)
  {
    if (diffDeal == 0)
      similarDeal = true;
    else if ((aggDeal / diffDeal) > SIMILARDEALLIMIT)
      similarDeal = true;
    else
      similarDeal = false;
  }
  else
    similarDeal = false;

  if (dl.trump != thrp->trump)
    newTrump = true;

  // ----------------------------------------------------------
  // Generic initialization.
  // ----------------------------------------------------------

  thrp->trump = dl.trump;

  thrp->iniDepth = cardCount - 4;
  int iniDepth = thrp->iniDepth;
  int trick = (iniDepth + 3) >> 2;
  int handRelFirst = (48 - iniDepth) % 4;
  int handToPlay = handId(dl.first, handRelFirst);
  thrp->trickNodes = 0;

  thrp->lookAheadPos.handRelFirst = handRelFirst;
  thrp->lookAheadPos.first[iniDepth] = dl.first;
  thrp->lookAheadPos.tricksMAX = 0;

  moveType mv = {0, 0, 0, 0};

  for (int k = 0; k <= 13; k++)
  {
    thrp->forbiddenMoves[k].rank = 0;
    thrp->forbiddenMoves[k].suit = 0;
  }

  // ----------------------------------------------------------
  // Consistency checks.
  // ----------------------------------------------------------

  ret = BoardValueChecks(dl, target, solutions, mode, thrp);
  if (ret != RETURN_NO_FAULT)
    return ret;


  // ----------------------------------------------------------
  // Last trick, easy to solve.
  // ----------------------------------------------------------

  if (cardCount <= 4)
  {
    int leadRank, leadSuit, leadSideWins;

    LastTrickWinner(dl, thrp, handToPlay, handRelFirst,
      leadRank, leadSuit, leadSideWins);

    futp->nodes = 0;
    futp->cards = 1;
    futp->suit[0] = leadSuit;
    futp->rank[0] = leadRank;
    futp->equals[0] = 0;
    futp->score[0] = (target == 0 && solutions < 3 ? 0 : leadSideWins);

    goto SOLVER_DONE;
  }


  // ----------------------------------------------------------
  // More detailed initialization.
  // ----------------------------------------------------------

  if ((mode != 2) &&
      (((newDeal) && (! similarDeal)) ||
       newTrump ||
       (thrp->nodes > SIMILARMAXWINNODES)))
  {
    TTresetReason reason = TT_RESET_UNKNOWN;
    if (thrp->nodes > SIMILARMAXWINNODES)
      reason = TT_RESET_TOO_MANY_NODES;
    else if (newDeal && ! similarDeal)
      reason = TT_RESET_NEW_DEAL;
    else if (newTrump)
      reason = TT_RESET_NEW_TRUMP;
    thrp->transTable->ResetMemory(reason);
  }

  if (newDeal)
  {
    SetDeal(thrp);
    SetDealTables(thrp);
  }
  else if (thrp->analysisFlag)
  {
    SetDeal(thrp);
  }
  thrp->analysisFlag = false;

  if (handToPlay == 0 || handToPlay == 2)
  {
    thrp->nodeTypeStore[0] = MAXNODE;
    thrp->nodeTypeStore[1] = MINNODE;
    thrp->nodeTypeStore[2] = MAXNODE;
    thrp->nodeTypeStore[3] = MINNODE;
  }
  else
  {
    thrp->nodeTypeStore[0] = MINNODE;
    thrp->nodeTypeStore[1] = MAXNODE;
    thrp->nodeTypeStore[2] = MINNODE;
    thrp->nodeTypeStore[3] = MAXNODE;
  }

  for (int k = 0; k < handRelFirst; k++)
  {
    mv.rank = dl.currentTrickRank[k];
    mv.suit = dl.currentTrickSuit[k];
    mv.sequence = 0;

    thrp->moves.Init(
      trick,
      k,
      dl.currentTrickRank,
      dl.currentTrickSuit,
      thrp->lookAheadPos.rankInSuit,
      thrp->trump,
      thrp->lookAheadPos.first[iniDepth]);

    if (k == 0)
      thrp->moves.MoveGen0(
        trick,
        thrp->lookAheadPos,
        thrp->bestMove[iniDepth],
        thrp->bestMoveTT[iniDepth],
        thrp->rel);
    else
      thrp->moves.MoveGen123(
        trick,
        k,
        thrp->lookAheadPos);

    thrp->lookAheadPos.move[iniDepth + handRelFirst - k] = mv;
    thrp->moves.MakeSpecific(mv, trick, k);
  }

  InitWinners(dl, thrp->lookAheadPos, thrp);

#ifdef DDS_AB_STATS
  thrp->ABStats.Reset();
  thrp->ABStats.ResetCum();
#endif

#ifdef DDS_TOP_LEVEL
  thrp->nodes = 0;
#endif

  thrp->moves.Init(
    trick,
    handRelFirst,
    dl.currentTrickRank,
    dl.currentTrickSuit,
    thrp->lookAheadPos.rankInSuit,
    thrp->trump,
    thrp->lookAheadPos.first[iniDepth]);

  if (handRelFirst == 0)
    thrp->moves.MoveGen0(
      trick,
      thrp->lookAheadPos,
      thrp->bestMove[iniDepth],
      thrp->bestMoveTT[iniDepth],
      thrp->rel);
  else
    thrp->moves.MoveGen123(
      trick,
      handRelFirst,
      thrp->lookAheadPos);

  noMoves = thrp->moves.GetLength(trick, handRelFirst);

  // ----------------------------------------------------------
  // mode == 0: Check whether there is only one possible move
  // ----------------------------------------------------------

  if (mode == 0 && noMoves == 1)
  {
    moveType const * mp = thrp->moves.MakeNextSimple(trick, handRelFirst);

    futp->nodes = 0;
    futp->cards = 1;

    futp->suit[0] = mp->suit;
    futp->rank[0] = mp->rank;
    futp->equals[0] = mp->sequence << 2;
    futp->score[0] = -2;

    goto SOLVER_DONE;
  }

  // ----------------------------------------------------------
  // solutions == 3: Target and mode don't matter; all cards
  // ----------------------------------------------------------

  if (solutions == 3)
  {
    // 7 for hand 0 and 2, 6 for hand 1 and 3
    int guess = 7 - (handToPlay & 0x1);
    int upperbound = 13;
    int lowerbound = 0;
    futp->cards = noMoves;

    for (int mno = 0; mno < noMoves; mno++)
    {
      do
      {
        ResetBestMoves(thrp);

        TIMER_START(TIMER_NO_AB, iniDepth);
        thrp->val = (* AB_ptr_list[handRelFirst])(
                      &thrp->lookAheadPos,
                      guess,
                      iniDepth,
                      thrp);
        TIMER_END(TIMER_NO_AB, iniDepth);

#ifdef DDS_TOP_LEVEL
        DumpTopLevel(thrp->fileTopLevel.GetStream(), 
          * thrp, guess, lowerbound, upperbound, 1);
#endif

        if (thrp->val)
        {
          mv = thrp->bestMove[iniDepth];
          lowerbound = guess++;
        }
        else
          upperbound = --guess;
      }
      while (lowerbound < upperbound);

      if (lowerbound)
      {
        thrp->bestMove[iniDepth] = mv;

        futp->suit[mno] = mv.suit;
        futp->rank[mno] = mv.rank;
        futp->equals[mno] = mv.sequence << 2;
        futp->score[mno] = lowerbound;

        thrp->forbiddenMoves[mno + 1].suit = mv.suit;
        thrp->forbiddenMoves[mno + 1].rank = mv.rank;

        guess = lowerbound;
        lowerbound = 0;
      }
      else
      {
        int noLeft = thrp->moves.GetLength(trick, handRelFirst);

        thrp->moves.Rewind(trick, handRelFirst);
        for (int j = 0; j < noLeft; j++)
        {
          moveType const * mp = 
            thrp->moves.MakeNextSimple(trick, handRelFirst);

          futp->suit[mno + j] = mp->suit;
          futp->rank[mno + j] = mp->rank;
          futp->equals[mno + j] = mp->sequence << 2;
          futp->score[mno + j] = 0;
        }

        break;
      }
    }
    goto SOLVER_STATS;
  }

  // ----------------------------------------------------------
  // target == 0: Only cards required, no scoring
  // ----------------------------------------------------------

  else if (target == 0)
  {
    futp->nodes = 0;
    futp->cards = (solutions == 1 ? 1 : noMoves);

    for (int mno = 0; mno < noMoves; mno++)
    {
      moveType const * mp = 
        thrp->moves.MakeNextSimple(trick, handRelFirst);

      futp->suit[mno] = mp->suit;
      futp->rank[mno] = mp->rank;
      futp->equals[mno] = mp->sequence << 2;
      futp->score[mno] = 0;
    }

    goto SOLVER_DONE;
  }

  // ----------------------------------------------------------
  // target == -1: Find optimum score and 1 or more cards
  // ----------------------------------------------------------

  else if (target == -1)
  {
    // 7 for hand 0 and 2, 6 for hand 1 and 3
    int guess = 7 - (handToPlay & 0x1);
    int upperbound = 13;
    int lowerbound = 0;
    do
    {
      ResetBestMoves(thrp);

      TIMER_START(TIMER_NO_AB, iniDepth);
      thrp->val = (* AB_ptr_list[handRelFirst])(&thrp->lookAheadPos,
                  guess,
                  iniDepth,
                  thrp);
      TIMER_END(TIMER_NO_AB, iniDepth);

#ifdef DDS_TOP_LEVEL
      DumpTopLevel(thrp->fileTopLevel.GetStream(),
        * thrp, guess, lowerbound, upperbound, 1);
#endif

      if (thrp->val)
      {
        mv = thrp->bestMove[iniDepth];
        lowerbound = guess++;
      }
      else
        upperbound = --guess;

    }
    while (lowerbound < upperbound);

    thrp->bestMove[iniDepth] = mv;
    if (lowerbound == 0)
    {
      // ALL the other moves must also have payoff 0.

      if (solutions == 1) // We only need one of them
        futp->cards = 1;
      else // solutions == 2, so return all cards
        futp->cards = noMoves;

      thrp->moves.Rewind(trick, handRelFirst);
      for (int i = 0; i < noMoves; i++)
      {
        moveType const * mp = 
          thrp->moves.MakeNextSimple(trick, handRelFirst);

        futp->score[i] = 0;
        futp->suit[i] = mp->suit;
        futp->rank[i] = mp->rank;
        futp->equals[i] = mp->sequence << 2;
      }

      goto SOLVER_STATS;
    }
    else // payoff > 0
    {
      futp->cards = 1;
      futp->score[0] = lowerbound;
      futp->suit[0] = mv.suit;
      futp->rank[0] = mv.rank;
      futp->equals[0] = mv.sequence << 2;

      if (solutions != 2)
        goto SOLVER_STATS;
    }
  }

  // ----------------------------------------------------------
  // target >= 1: Find optimum card(s) achieving user's target
  // ----------------------------------------------------------

  else
  {
    TIMER_START(TIMER_NO_AB, iniDepth);
    thrp->val = (* AB_ptr_list[handRelFirst])(
                  &thrp->lookAheadPos,
                  target,
                  iniDepth,
                  thrp);
    TIMER_END(TIMER_NO_AB, iniDepth);

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp->fileTopLevel.GetStream(), 
      * thrp, target, -1, -1, 0);
#endif

    if (! thrp->val)
    {
      // No move. If target was 1, then we are sure that in
      // fact no tricks can be won. If target was > 1, then
      // it is still possible that no tricks can't be won.
      // We don't know. So that's arguably a small bug.
      futp->cards = 0;
      futp->score[0] = (target > 1 ? -1 : 0);

      goto SOLVER_STATS;
    }
    else
    {
      futp->cards = 1;
      futp->suit[0] = thrp->bestMove[iniDepth].suit;
      futp->rank[0] = thrp->bestMove[iniDepth].rank;
      futp->equals[0] = thrp->bestMove[iniDepth].sequence << 2;
      futp->score[0] = target;

      if (solutions != 2)
        goto SOLVER_STATS;
    }
  }

  // ----------------------------------------------------------
  // solution == 2 && payoff > 0: Find other cards with score.
  // This applies both to target == -1 and target >= 1.
  // ----------------------------------------------------------

  forb = 1;
  ind = 1;

  while (ind < noMoves)
  {
    // Moves up to and including bestMove are now forbidden.

    thrp->moves.Rewind(trick, handRelFirst);
    int num = thrp->moves.GetLength(trick, handRelFirst);

    for (int k = 0; k < num; k++)
    {
      moveType const * mp = 
        thrp->moves.MakeNextSimple(trick, handRelFirst);
      thrp->forbiddenMoves[forb] = * mp;
      forb++;

      if ((thrp->bestMove[iniDepth].suit == mp->suit) &&
          (thrp->bestMove[iniDepth].rank == mp->rank))
        break;
    }

    ResetBestMoves(thrp);

    TIMER_START(TIMER_NO_AB, iniDepth);
    thrp->val = (* AB_ptr_list[handRelFirst])(
                  &thrp->lookAheadPos,
                  futp->score[0],
                  iniDepth,
                  thrp);
    TIMER_END(TIMER_NO_AB, iniDepth);

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp->fileTopLevel.GetStream(),
      * thrp, target, -1, -1, 2);
#endif

    if (! thrp->val)
      break;

    futp->cards = ind + 1;
    futp->suit[ind] = thrp->bestMove[iniDepth].suit;
    futp->rank[ind] = thrp->bestMove[iniDepth].rank;
    futp->equals[ind] = thrp->bestMove[iniDepth].sequence << 2;
    futp->score[ind] = futp->score[0];
    ind++;
  }


SOLVER_STATS:

  for (int k = 0; k <= 13; k++)
  {
    thrp->forbiddenMoves[k].rank = 0;
    thrp->forbiddenMoves[k].suit = 0;
  }

#ifdef DDS_TIMING
  thrp->timerList.PrintStats(thrp->fileTimerList.GetStream());
#endif

#ifdef DDS_TT_STATS
  // These are for the large TT -- empty if not.
  // thrp->transTable->PrintAllSuits(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllSuitStats(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllEntries(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllEntryStats(thrp->fileTTstats.GetStream());

  thrp->transTable->PrintSummarySuitStats(thrp->fileTTstats.GetStream());
  thrp->transTable->PrintSummaryEntryStats(thrp->fileTTstats.GetStream());

  // These are for the small TT -- empty if not.
  thrp->transTable->PrintNodeStats(thrp->fileTTstats.GetStream());
  thrp->transTable->PrintResetStats(thrp->fileTTstats.GetStream());
#endif

#ifdef DDS_MOVES
  thrp->moves.PrintTrickStats(thrp->fileMoves.GetStream());
#ifdef DDS_MOVES_DETAILS
  thrp->moves.PrintTrickDetails(thrp->fileMoves.GetStream());
#endif
  thrp->moves.PrintFunctionStats(thrp->fileMoves.GetStream());
#endif

SOLVER_DONE:

  thrp->memUsed = thrp->transTable->MemoryInUse() + ThreadMemoryUsed();
  futp->nodes = thrp->trickNodes;

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif

  return RETURN_NO_FAULT;
}


int SolveSameBoard(
  ThreadData * thrp,
  const deal& dl,
  futureTricks * futp,
  const int hint)
{
  // Specialized function for SolveChunkDDtable for repeat solves.
  // No further parameter checks! This function makes heavy reuse
  // of parameters that are already stored in various places. It
  // corresponds to:
  // target == -1, solutions == 1, mode == 2.
  // The function only needs to return fut.score[0].

  int iniDepth = thrp->iniDepth;
  int trick = (iniDepth + 3) >> 2;
  thrp->trickNodes = 0;

  thrp->lookAheadPos.first[iniDepth] = dl.first;

  if (dl.first == 0 || dl.first == 2)
  {
    thrp->nodeTypeStore[0] = MAXNODE;
    thrp->nodeTypeStore[1] = MINNODE;
    thrp->nodeTypeStore[2] = MAXNODE;
    thrp->nodeTypeStore[3] = MINNODE;
  }
  else
  {
    thrp->nodeTypeStore[0] = MINNODE;
    thrp->nodeTypeStore[1] = MAXNODE;
    thrp->nodeTypeStore[2] = MINNODE;
    thrp->nodeTypeStore[3] = MAXNODE;
  }

#ifdef DDS_AB_STATS
  thrp->ABStats.Reset();
  thrp->ABStats.ResetCum();
#endif

#ifdef DDS_TOP_LEVEL
  thrp->nodes = 0;
#endif

  thrp->moves.Reinit(trick, dl.first);

  int guess = hint;
  int lowerbound = 0;
  int upperbound = 13;

  do
  {
    ResetBestMoves(thrp);

    TIMER_START(TIMER_NO_AB, iniDepth);
    thrp->val = ABsearch(
                  &thrp->lookAheadPos,
                  guess,
                  iniDepth,
                  thrp);
    TIMER_END(TIMER_NO_AB, iniDepth);

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp->fileTopLevel.GetStream(),
      * thrp, guess, lowerbound, upperbound, 1);
#endif

    if (thrp->val)
      lowerbound = guess++;
    else
      upperbound = --guess;
  }
  while (lowerbound < upperbound);

  futp->cards = 1;
  futp->score[0] = lowerbound;

  thrp->memUsed = thrp->transTable->MemoryInUse() +
                  ThreadMemoryUsed();

#ifdef DDS_TIMING
  thrp->timerList.PrintStats(thrp->fileTimerList.GetStream());
#endif

#ifdef DDS_TT_STATS
  // These are for the large TT -- empty if not.
  // thrp->transTable->PrintAllSuits(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllSuitStats(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllEntries(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllEntryStats(thrp->fileTTstats.GetStream());

  thrp->transTable->PrintSummarySuitStats(thrp->fileTTstats.GetStream());
  thrp->transTable->PrintSummaryEntryStats(thrp->fileTTstats.GetStream());

  // These are for the small TT -- empty if not.
  thrp->transTable->PrintNodeStats(thrp->fileTTstats.GetStream());
  thrp->transTable->PrintResetStats(thrp->fileTTstats.GetStream());
#endif

#ifdef DDS_MOVES
  thrp->moves.PrintTrickStats(thrp->fileMoves.GetStream());
#ifdef DDS_MOVES_DETAILS
  thrp->moves.PrintTrickDetails(thrp->fileMoves.GetStream());
#endif
  thrp->moves.PrintFunctionStats(thrp->fileMoves.GetStream());
#endif

  futp->nodes = thrp->trickNodes;

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif

  return RETURN_NO_FAULT;
}


int AnalyseLaterBoard(
  ThreadData * thrp,
  const int leadHand,
  moveType const * move,
  const int hint,
  const int hintDir,
  futureTricks * futp)
{
  // Specialized function for PlayAnalyser for cards after the
  // opening lead. No further parameter checks! This function
  // makes heavy reuse of parameters that are already stored in
  // various places. It corresponds to:
  // target == -1, solutions == 1, mode == 2.
  // The function only needs to return fut.score[0].

  int iniDepth = --thrp->iniDepth;
  int cardCount = iniDepth + 4;
  int trick = (iniDepth + 3) >> 2;
  int handRelFirst = (48 - iniDepth) % 4;
  thrp->trickNodes = 0;
  thrp->analysisFlag = true;
  int handToPlay = handId(leadHand, handRelFirst);

  if (handToPlay == 0 || handToPlay == 2)
  {
    thrp->nodeTypeStore[0] = MAXNODE;
    thrp->nodeTypeStore[1] = MINNODE;
    thrp->nodeTypeStore[2] = MAXNODE;
    thrp->nodeTypeStore[3] = MINNODE;
  }
  else
  {
    thrp->nodeTypeStore[0] = MINNODE;
    thrp->nodeTypeStore[1] = MAXNODE;
    thrp->nodeTypeStore[2] = MINNODE;
    thrp->nodeTypeStore[3] = MAXNODE;
  }

  if (handRelFirst == 0)
  {
    thrp->moves.MakeSpecific(* move, trick + 1, 3);
    unsigned short int ourWinRanks[DDS_SUITS]; // Unused here
    Make3(&thrp->lookAheadPos, ourWinRanks, iniDepth + 1, move, thrp);
  }
  else if (handRelFirst == 1)
  {
    thrp->moves.MakeSpecific(* move, trick, 0);
    Make0(&thrp->lookAheadPos, iniDepth + 1, move);
  }
  else if (handRelFirst == 2)
  {
    thrp->moves.MakeSpecific(* move, trick, 1);
    Make1(&thrp->lookAheadPos, iniDepth + 1, move);
  }
  else
  {
    thrp->moves.MakeSpecific(* move, trick, 2);
    Make2(&thrp->lookAheadPos, iniDepth + 1, move);
  }

  if (cardCount <= 4)
  {
    // Last trick.
    evalType eval = Evaluate(&thrp->lookAheadPos, thrp->trump, thrp);
    futp->score[0] = eval.tricks;
    futp->nodes = 0;

    return RETURN_NO_FAULT;
  }

#ifdef DDS_AB_STATS
  thrp->ABStats.Reset();
  thrp->ABStats.ResetCum();
#endif

#ifdef DDS_TOP_LEVEL
  thrp->nodes = 0;
#endif

  int guess = hint,
      lowerbound,
      upperbound;

  if (hintDir == 0)
  {
    lowerbound = hint;
    upperbound = 13;
  }
  else
  {
    lowerbound = 0;
    upperbound = hint;
  }

  do
  {
    ResetBestMoves(thrp);

    TIMER_START(TIMER_NO_AB, iniDepth);
    thrp->val = (* AB_ptr_trace_list[handRelFirst])(
                  &thrp->lookAheadPos,
                  guess,
                  iniDepth,
                  thrp);
    TIMER_END(TIMER_NO_AB, iniDepth);

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp->fileTopLevel.GetStream(),
      * thrp, guess, lowerbound, upperbound, 1);
#endif

    if (thrp->val)
      lowerbound = guess++;
    else
      upperbound = --guess;

  }
  while (lowerbound < upperbound);

  futp->score[0] = lowerbound;
  futp->nodes = thrp->trickNodes;

  thrp->memUsed = thrp->transTable->MemoryInUse() +
                  ThreadMemoryUsed();

#ifdef DDS_TIMING
  thrp->timerList.PrintStats(thrp->fileTimerList.GetStream());
#endif

#ifdef DDS_TT_STATS
  // These are for the large TT -- empty if not.
  // thrp->transTable->PrintAllSuits(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllSuitStats(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllEntries(thrp->fileTTstats.GetStream());
  // thrp->transTable->PrintAllEntryStats(thrp->fileTTstats.GetStream());

  thrp->transTable->PrintSummarySuitStats(thrp->fileTTstats.GetStream());
  thrp->transTable->PrintSummaryEntryStats(thrp->fileTTstats.GetStream());

  // These are for the small TT -- empty if not.
  thrp->transTable->PrintNodeStats(thrp->fileTTstats.GetStream());
  thrp->transTable->PrintResetStats(thrp->fileTTstats.GetStream());
#endif

#ifdef DDS_MOVES
  thrp->moves.PrintTrickStats(thrp->fileMoves.GetStream());
#ifdef DDS_MOVES_DETAILS
  thrp->moves.PrintTrickDetails(thrp->fileMoves.GetStream());
#endif
  thrp->moves.PrintFunctionStats(thrp->fileMoves.GetStream());
#endif

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif

  return RETURN_NO_FAULT;
}


int BoardRangeChecks(
  const deal& dl,
  const int target,
  const int solutions,
  const int mode)
{
  if (target < -1)
  {
    DumpInput(RETURN_TARGET_WRONG_LO, dl, target, solutions, mode);
    return RETURN_TARGET_WRONG_LO;
  }

  if (target > 13)
  {
    DumpInput(RETURN_TARGET_WRONG_HI, dl, target, solutions, mode);
    return RETURN_TARGET_WRONG_HI;
  }

  if (solutions < 1)
  {
    DumpInput(RETURN_SOLNS_WRONG_LO, dl, target, solutions, mode);
    return RETURN_SOLNS_WRONG_LO;
  }

  if (solutions > 3)
  {
    DumpInput(RETURN_SOLNS_WRONG_HI, dl, target, solutions, mode);
    return RETURN_SOLNS_WRONG_HI;
  }

  if (mode < 0)
  {
    DumpInput(RETURN_MODE_WRONG_LO, dl, target, solutions, mode);
    return RETURN_MODE_WRONG_LO;
  }

  if (mode > 2)
  {
    DumpInput(RETURN_MODE_WRONG_HI, dl, target, solutions, mode);
    return RETURN_MODE_WRONG_HI;
  }

  if (dl.trump < 0 || dl.trump > 4)
  {
    DumpInput(RETURN_TRUMP_WRONG, dl, target, solutions, mode);
    return RETURN_TRUMP_WRONG;
  }

  if (dl.first < 0 || dl.first > 3)
  {
    DumpInput(RETURN_FIRST_WRONG, dl, target, solutions, mode);
    return RETURN_FIRST_WRONG;
  }

  int rankSeen[3] = {0, 0, 0};
  for (int k = 0; k < 3; k++)
  {
    int r = dl.currentTrickRank[k];
    if (r == 0)
      continue;

    rankSeen[k] = 1;

    if (r < 2 || r > 14)
    {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }

    if (dl.currentTrickSuit[k] < 0 || dl.currentTrickSuit[k] > 3)
    {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }
  }

  if ((rankSeen[2] && (! rankSeen[1] || ! rankSeen[0])) ||
      (rankSeen[1] && ! rankSeen[0]))
  {
    DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
    return RETURN_SUIT_OR_RANK;
  }

  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      unsigned c = dl.remainCards[h][s];
      if (c != 0 && (c < 0x0004 || c >= 0x8000))
      {
        DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
        return RETURN_SUIT_OR_RANK;
      }
    }
  }

  return RETURN_NO_FAULT;
}


int BoardValueChecks(
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  ThreadData const * thrp)
{
  int cardCount = thrp->iniDepth + 4;
  if (cardCount <= 0)
  {
    DumpInput(RETURN_ZERO_CARDS, dl, target, solutions, mode);
    return RETURN_ZERO_CARDS;
  }

  if (cardCount > 52)
  {
    DumpInput(RETURN_TOO_MANY_CARDS, dl, target, solutions, mode);
    return RETURN_TOO_MANY_CARDS;
  }

  int totalTricks;
  if (cardCount % 4)
    totalTricks = ((cardCount - 4) >> 2) + 2;
  else
    totalTricks = ((cardCount - 4) >> 2) + 1;

  if (totalTricks < target)
  {
    DumpInput(RETURN_TARGET_TOO_HIGH, dl, target, solutions, mode);
    return RETURN_TARGET_TOO_HIGH;
  }

  int handRelFirst = thrp->lookAheadPos.handRelFirst;

  int noOfCardsPerHand[DDS_HANDS] = {0, 0, 0, 0};
  for (int k = 0; k < handRelFirst; k++)
    noOfCardsPerHand[handId(dl.first, k)] = 1;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      noOfCardsPerHand[h] += counttable[thrp->suit[h][s]];

  for (int h = 1; h < DDS_HANDS; h++)
  {
    if (noOfCardsPerHand[h] != noOfCardsPerHand[0])
    {
      DumpInput(RETURN_CARD_COUNT, dl, target, solutions, mode);
      return RETURN_CARD_COUNT;
    }
  }

  for (int k = 0; k < handRelFirst; k++)
  {
    unsigned short int aggrRemain = 0;
    for (int h = 0; h < DDS_HANDS; h++)
      aggrRemain |= (dl.remainCards[h][dl.currentTrickSuit[k]] >> 2);

    if ((aggrRemain & bitMapRank[dl.currentTrickRank[k]]) != 0)
    {
      DumpInput(RETURN_PLAYED_CARD, dl, target, solutions, mode);
      return RETURN_PLAYED_CARD;
    }
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    for (int r = 2; r <= 14; r++)
    {
      bool found = false;
      for (int h = 0; h < DDS_HANDS; h++)
      {
        if ((thrp->suit[h][s] & bitMapRank[r]) != 0)
        {
          if (found)
          {
            DumpInput(RETURN_DUPLICATE_CARDS, dl,
                      target, solutions, mode);
            return RETURN_DUPLICATE_CARDS;
          }
          else
            found = true;
        }
      }
    }
  }

  return RETURN_NO_FAULT;
}


void LastTrickWinner(
  const deal& dl,
  ThreadData const * thrp,
  const int handToPlay,
  const int handRelFirst,
  int& leadRank,
  int& leadSuit,
  int& leadSideWins)
{
  int lastTrickSuit[DDS_HANDS],
      lastTrickRank[DDS_HANDS],
      h,
      hp;

  for (h = 0; h < handRelFirst; h++)
  {
    hp = handId(dl.first, h);
    lastTrickSuit[hp] = dl.currentTrickSuit[h];
    lastTrickRank[hp] = dl.currentTrickRank[h];
  }

  for (h = handRelFirst; h < DDS_HANDS; h++)
  {
    hp = handId(dl.first, h);
    for (int s = 0; s < DDS_SUITS; s++)
    {
      if (thrp->suit[hp][s] != 0)
      {
        lastTrickSuit[hp] = s;
        lastTrickRank[hp] = highestRank[thrp->suit[hp][s]];
        break;
      }
    }
  }

  int maxRank = 0,
      maxSuit,
      maxHand = -1;

  /* Highest trump? */
  if (dl.trump != DDS_NOTRUMP)
  {
    for (h = 0; h < DDS_HANDS; h++)
    {
      if ((lastTrickSuit[h] == dl.trump) &&
          (lastTrickRank[h] > maxRank))
      {
        maxRank = lastTrickRank[h];
        maxSuit = dl.trump;
        maxHand = h;
      }
    }
  }

  /* Highest card in leading suit */
  if (maxRank == 0)
  {
    maxRank = lastTrickRank[dl.first];
    maxSuit = lastTrickSuit[dl.first];
    maxHand = dl.first;

    for (h = 0; h < DDS_HANDS; h++)
    {
      if (lastTrickSuit[h] == maxSuit &&
          lastTrickRank[h] > maxRank)
      {
        maxHand = h;
        maxRank = lastTrickRank[h];
      }
    }
  }

  hp = handId(dl.first, handRelFirst);
  leadRank = lastTrickRank[hp];
  leadSuit = lastTrickSuit[hp];
  leadSideWins = ((handToPlay == maxHand ||
    partner[handToPlay] == maxHand) ? 1 : 0);
}

