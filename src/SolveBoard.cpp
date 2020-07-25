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

int SolveAllBoardsN(
  boards& bds,
  solvedBoards& solved);

bool SameBoard(
  const boards& bds,
  const unsigned index1,
  const unsigned index2);


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
  boards& bds,
  solvedBoards& solved)
{
  param.error = 0;

  if (bds.noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  param.bop = &bds;
  param.solvedp = &solved;
  param.noOfBoards = bds.noOfBoards;

  scheduler.RegisterRun(DDS_RUN_SOLVE, bds);
  sysdep.RegisterRun(DDS_RUN_SOLVE, bds);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solved.solvedBoard[k].cards = 0;

  START_BLOCK_TIMER;
  int retRun = sysdep.RunThreads();
  END_BLOCK_TIMER;

  if (retRun != RETURN_NO_FAULT)
    return retRun;

  solved.noOfBoards = param.noOfBoards;

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

  int res = SolveAllBoardsN(bo, * solvedp);
  return res;
}


int STDCALL SolveAllBoardsBin(
  boards * bop,
  solvedBoards * solvedp)
{
  return SolveAllBoardsN(* bop, * solvedp);
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

  return SolveAllBoardsN(* bop, * solvedp);
}


void DetectSolveDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs)
{
  const unsigned nu = static_cast<unsigned>(bds.noOfBoards);

  uniques.clear();
  crossrefs.resize(nu);

  for (unsigned i = 0; i < nu; i++)
    crossrefs[i] = -1;

  for (unsigned i = 0; i < nu; i++)
  {
    if (crossrefs[i] != -1)
      continue;

    uniques.push_back(static_cast<int>(i));

    for (unsigned index = i+1; index < nu; index++)
    {
      if (SameBoard(bds, i, index))
        crossrefs[index] = static_cast<int>(i);
    }
  }
}


bool SameBoard(
  const boards& bds,
  const unsigned index1,
  const unsigned index2)
{
  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      if (bds.deals[index1].remainCards[h][s] !=
          bds.deals[index2].remainCards[h][s])
        return false;
    }
  }

  if (bds.mode[index1] != bds.mode[index2])
    return false;
  if (bds.solutions[index1] != bds.solutions[index2])
    return false;
  if (bds.target[index1] != bds.target[index2])
    return false;
  if (bds.deals[index1].first != bds.deals[index2].first)
    return false;
  if (bds.deals[index1].trump != bds.deals[index2].trump)
    return false;

  for (int k = 0; k < 3; k++)
  {
    if (bds.deals[index1].currentTrickSuit[k] != 
        bds.deals[index2].currentTrickSuit[k])
      return false;
    if (bds.deals[index1].currentTrickRank[k] != 
        bds.deals[index2].currentTrickRank[k])
      return false;
  }
  return true;
}

