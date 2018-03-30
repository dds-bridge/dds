/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "SolverIF.h"
#include "SolveBoard.h"
#include "System.h"
#include "Memory.h"
#include "Scheduler.h"
#include "PBN.h"
#include "debug.h"


paramType param;

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;

bool SameBoard(
  boards const * bop,
  const unsigned index1,
  const unsigned index2);


#include <iostream>
void SolveSingleCommon(
  const int thrId,
  const int bno)
{
  futureTricks fut;

  START_THREAD_TIMER(thrId);
  int res = SolveBoard(
              param.bop->deals[bno],
              param.bop->target[bno],
              param.bop->solutions[bno],
              param.bop->mode[bno],
              &fut,
              thrId);
  END_THREAD_TIMER(thrId);

  if (res == 1)
    param.solvedp->solvedBoard[bno] = fut;
  else
    param.error = res;
}


void CopySolveSingle(const vector<int>& crossrefs)
{
  for (unsigned i = 0; i < crossrefs.size(); i++)
  {
    if (crossrefs[i] == -1)
      continue;

    START_THREAD_TIMER(thrId);
    param.solvedp->solvedBoard[i] = 
      param.solvedp->solvedBoard[crossrefs[i]];
    END_THREAD_TIMER(thrId);
  }
}


void SolveChunkCommon(
  const int thrId)
{
  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thrId);
    index = st.number;
    if (index == -1)
      break;

    // This is not a perfect repeat detector, as the hands in
    // a group might have declarers N, S, N, N. Then the second
    // N would not reuse the first N. However, must reuses are
    // reasonably adjacent, and this is just an optimization anyway.

    if (st.repeatOf != -1 &&
        param.bop->deals[index ].first ==
        param.bop->deals[st.repeatOf].first)
    {
      START_THREAD_TIMER(thrId);
      param.solvedp->solvedBoard[index] = 
        param.solvedp->solvedBoard[st.repeatOf];
      END_THREAD_TIMER(thrId);
      continue;
    }
    else
    {
      SolveSingleCommon(thrId, index);
    }
  }
}


int SolveAllBoardsN(
  boards * bop,
  solvedBoards * solvedp)
{
  param.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  param.bop = bop;
  param.solvedp = solvedp;
  param.noOfBoards = bop->noOfBoards;

  scheduler.RegisterRun(DDS_RUN_SOLVE, bop);
  sysdep.RegisterRun(DDS_RUN_SOLVE, bop);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  START_BLOCK_TIMER;
  int retRun = sysdep.RunThreads();
  END_BLOCK_TIMER;

  if (retRun != RETURN_NO_FAULT)
    return retRun;

  solvedp->noOfBoards = param.noOfBoards;

#ifdef DDS_SCHEDULER 
  scheduler.PrintTiming();
#endif

  if (param.error == 0)
    return RETURN_NO_FAULT;
  else
    return param.error;
}


int STDCALL SolveBoardPBN(
  dealPBN dlpbn, 
  int target,
  int solutions, 
  int mode, 
  futureTricks * futp, 
  int thrId)
{
  deal dl;
  if (ConvertFromPBN(dlpbn.remainCards, dl.remainCards) != RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  for (int k = 0; k <= 2; k++)
  {
    dl.currentTrickRank[k] = dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k] = dlpbn.currentTrickSuit[k];
  }
  dl.first = dlpbn.first;
  dl.trump = dlpbn.trump;

  int res = SolveBoard(dl, target, solutions, mode, futp, thrId);
  return res;
}


int STDCALL SolveAllBoards(
  boardsPBN * bop, 
  solvedBoards * solvedp)
{
  boards bo;
  bo.noOfBoards = bop->noOfBoards;
  if (bo.noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < bop->noOfBoards; k++)
  {
    bo.mode[k] = bop->mode[k];
    bo.solutions[k] = bop->solutions[k];
    bo.target[k] = bop->target[k];
    bo.deals[k].first = bop->deals[k].first;
    bo.deals[k].trump = bop->deals[k].trump;

    for (int i = 0; i <= 2; i++)
    {
      bo.deals[k].currentTrickSuit[i] = bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i] = bop->deals[k].currentTrickRank[i];
    }

    if (ConvertFromPBN(bop->deals[k].remainCards, bo.deals[k].remainCards) 
        != 1)
      return RETURN_PBN_FAULT;
  }

  int res = SolveAllBoardsN(&bo, solvedp);
  return res;
}


int STDCALL SolveAllChunksPBN(
  boardsPBN * bop, 
  solvedBoards * solvedp, 
  int chunkSize)
{
  // Historical aliases.  Don't use -- they may go away.
  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  return SolveAllBoards(bop, solvedp);
}


int STDCALL SolveAllChunks(
  boardsPBN * bop, 
  solvedBoards * solvedp, 
  int chunkSize)
{
  // Historical aliases.  Don't use -- they may go away.
  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  return SolveAllBoards(bop, solvedp);
}


int STDCALL SolveAllChunksBin(
  boards * bop, 
  solvedBoards * solvedp, 
  int chunkSize)
{
  // Historical aliases.  Don't use -- they may go away.
  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  return SolveAllBoardsN(bop, solvedp);
}


void DetectSolveDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs)
{
  uniques.clear();
  crossrefs.resize(bds.noOfBoards);

  const unsigned nu = static_cast<unsigned>(bds.noOfBoards);
  for (unsigned i = 0; i < nu; i++)
    crossrefs[i] = -1;

  for (unsigned i = 0; i < nu; i++)
  {
    if (crossrefs[i] != -1)
      continue;

    uniques.push_back(i);

    for (unsigned index = i+1; index < nu; index++)
    {
      if (SameBoard(&bds, i, index))
        crossrefs[index] = i;
    }
  }
}


bool SameBoard(
  boards const * bop,
  const unsigned index1,
  const unsigned index2)
{
  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      if (bop->deals[index1].remainCards[h][s] !=
          bop->deals[index2].remainCards[h][s])
        return false;
    }
  }

  if (bop->mode[index1] != bop->mode[index2])
    return false;
  if (bop->solutions[index1] != bop->solutions[index2])
    return false;
  if (bop->target[index1] != bop->target[index2])
    return false;
  if (bop->deals[index1].first != bop->deals[index2].first)
    return false;
  if (bop->deals[index1].trump != bop->deals[index2].trump)
    return false;

  for (int k = 0; k < 3; k++)
  {
    if (bop->deals[index1].currentTrickSuit[k] != 
        bop->deals[index2].currentTrickSuit[k])
      return false;
    if (bop->deals[index1].currentTrickRank[k] != 
        bop->deals[index2].currentTrickRank[k])
      return false;
  }
  return true;
}

