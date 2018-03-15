/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "threadmem.h"
#include "SolverIF.h"
#include "SolveBoard.h"
#include "Scheduler.h"
#include "System.h"
#include "PBN.h"
#include "debug.h"


extern int noOfThreads;
long chunk;
paramType param;
extern System sysdep;


void SolveChunkCommon(
  const int thid)
{
  futureTricks fut[MAXNOOFBOARDS];
  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thid);
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
      START_THREAD_TIMER(thid);
      param.solvedp->solvedBoard[index] = fut[ st.repeatOf ];
      END_THREAD_TIMER(thid);
      continue;
    }
    else
    {
      START_THREAD_TIMER(thid);
      int res = SolveBoard(
                  param.bop->deals[index],
                  param.bop->target[index],
                  param.bop->solutions[index],
                  param.bop->mode[index],
                  &fut[index],
                  thid);
      END_THREAD_TIMER(thid);

      if (res == 1)
        param.solvedp->solvedBoard[index] = fut[index];
      else
        param.error = res;
    }
  }
}


void SolveChunkDDtableCommon(
  const int thid)
{
  futureTricks fut[MAXNOOFBOARDS];
  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thid);
    index = st.number;
    if (index == -1)
      break;

    if (st.repeatOf != -1)
    {
      START_THREAD_TIMER(thid);
      for (int k = 0; k < chunk; k++)
      {
        param.bop->deals[index].first = k;

        param.solvedp->solvedBoard[index].score[k] =
          param.solvedp->solvedBoard[ st.repeatOf ].score[k];
      }
      END_THREAD_TIMER(thid);
      continue;
    }

    param.bop->deals[index].first = 0;

    START_THREAD_TIMER(thid);
    int res = SolveBoard(
                param.bop->deals[index],
                param.bop->target[index],
                param.bop->solutions[index],
                param.bop->mode[index],
                &fut[index],
                thid);

    // SH: I'm making a terrible use of the fut structure here.

    if (res == 1)
      param.solvedp->solvedBoard[index].score[0] = fut[index].score[0];
    else
      param.error = res;

    for (int k = 1; k < chunk; k++)
    {
      int hint = (k == 2 ? fut[index].score[0] :
                  13 - fut[index].score[0]);

      param.bop->deals[index].first = k; // Next declarer

      res = SolveSameBoard(
              param.bop->deals[index],
              &fut[index],
              hint,
              thid);

      if (res == 1)
        param.solvedp->solvedBoard[index].score[k] =
          fut[index].score[0];
      else
        param.error = res;
    }
    END_THREAD_TIMER(thid);
  }
}


int SolveAllBoardsN(
  boards * bop,
  solvedBoards * solvedp,
  int chunkSize,
  int source) // 0 solve, 1 calc
{
  chunk = chunkSize;
  param.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  param.bop = bop;
  param.solvedp = solvedp;
  param.noOfBoards = bop->noOfBoards;

  if (source == 0)
  {
    scheduler.Register(bop, SCHEDULER_SOLVE);
    sysdep.Register(DDS_SYSTEM_SOLVE, noOfThreads);
  }
  else
  {
    scheduler.Register(bop, SCHEDULER_CALC);
    sysdep.Register(DDS_SYSTEM_CALC, noOfThreads);
  }

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  int retInit = sysdep.InitThreads();
  if (retInit != RETURN_NO_FAULT)
    return retInit;


  START_BLOCK_TIMER;
  int retRun = sysdep.RunThreads(chunkSize);
  END_BLOCK_TIMER;

  if (retRun != RETURN_NO_FAULT)
    return retRun;

  /* Calculate number of solved boards. */
  solvedp->noOfBoards = param.noOfBoards;

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
  int thrIndex)
{

  int res, k;
  deal dl;

  if (ConvertFromPBN(dlpbn.remainCards, dl.remainCards) != RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  for (k = 0; k <= 2; k++)
  {
    dl.currentTrickRank[k] = dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k] = dlpbn.currentTrickSuit[k];
  }
  dl.first = dlpbn.first;
  dl.trump = dlpbn.trump;

  res = SolveBoard(dl, target, solutions, mode, futp, thrIndex);

  return res;
}


int STDCALL SolveAllBoards(
  boardsPBN * bop, 
  solvedBoards * solvedp)
{
  boards bo;
  int k, i, res;

  bo.noOfBoards = bop->noOfBoards;
  if (bo.noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (k = 0; k < bop->noOfBoards; k++)
  {
    bo.mode[k] = bop->mode[k];
    bo.solutions[k] = bop->solutions[k];
    bo.target[k] = bop->target[k];
    bo.deals[k].first = bop->deals[k].first;
    bo.deals[k].trump = bop->deals[k].trump;
    for (i = 0; i <= 2; i++)
    {
      bo.deals[k].currentTrickSuit[i] = bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i] = bop->deals[k].currentTrickRank[i];
    }
    if (ConvertFromPBN(bop->deals[k].remainCards, bo.deals[k].remainCards) != 1)
      return RETURN_PBN_FAULT;
  }

  res = SolveAllBoardsN(&bo, solvedp, 1, 0);

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

  return SolveAllBoardsN(bop, solvedp, 1, 0);
}


