/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Disable some boost header warnings.
#ifdef DDS_THREADS_BOOST
  #if (defined(_WIN32) && !defined(__CYGWIN__))
    #pragma warning(push)
    #pragma warning(disable: 4061 4191 4365 4571 4619 4625 4626 5026 5027 5031)
  #endif

  #include <boost/thread.hpp>
  using boost::thread;

  #ifdef _WIN32
    #pragma warning(pop)
  #endif
#endif

#include <vector>
using namespace std;


#include "dds.h"
#include "threadmem.h"
#include "SolverIF.h"
#include "SolveBoard.h"
#include "Scheduler.h"
#include "PBN.h"
#include "debug.h"


#ifdef DDS_SCHEDULER
  #define START_BLOCK_TIMER scheduler.StartBlockTimer()
  #define END_BLOCK_TIMER scheduler.EndBlockTimer()
  #define START_THREAD_TIMER(a) scheduler.StartThreadTimer(a)
  #define END_THREAD_TIMER(a) scheduler.EndThreadTimer(a)
#else
  #define START_BLOCK_TIMER 1
  #define END_BLOCK_TIMER 1
  #define START_THREAD_TIMER(a) 1
  #define END_THREAD_TIMER(a) 1
#endif

extern int noOfThreads;
long chunk;
paramType param;


void SolveChunkCommon(const int thid);
void SolveChunkDDtableCommon(const int thid);


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

#if (defined(DDS_THREADS_SINGLE))

int SolveInitThreadsNone();
int SolveRunThreadsNone(const int chunkSize);


int SolveInitThreadsNone()
{
  return RETURN_NO_FAULT;
}


int SolveRunThreadsNone(
  const int chunkSize)
{
  START_BLOCK_TIMER;

  if (chunkSize == 1)
    SolveChunkCommon(0);
  else
    SolveChunkDDtableCommon(0);
  END_BLOCK_TIMER;

  return RETURN_NO_FAULT;
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

  int retInit = SolveInitThreadsNone();
  if (retInit != RETURN_NO_FAULT)
    return retInit;

  param.bop = bop;
  param.solvedp = solvedp;
  param.noOfBoards = bop->noOfBoards;

  if (source == 0)
    scheduler.Register(bop, SCHEDULER_SOLVE);
  else
    scheduler.Register(bop, SCHEDULER_CALC);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  int retRun = SolveRunThreadsNone(chunkSize);
  if (retRun != RETURN_NO_FAULT)
    return retRun;

  /* Calculate number of solved boards. */
  solvedp->noOfBoards = param.noOfBoards;

  if (param.error == 0)
    return 1;
  else
    return param.error;
}


#elif (defined(_WIN32) || defined(__CYGWIN__)) && \
     (!defined(_OPENMP)) && \
     !defined(DDS_THREADS_BOOST)

HANDLE solveAllEvents[MAXNOOFTHREADS];
LONG volatile threadIndex;

DWORD CALLBACK SolveChunk (void *);
DWORD CALLBACK SolveChunkDDtable (void *);
int SolveInitThreadsWindows();
int SolveRunThreadsWindows(const int chunkSize);

DWORD CALLBACK SolveChunk (void *)
{
  int thid;
  thid = InterlockedIncrement(&threadIndex);

  SolveChunkCommon(thid);

  if (SetEvent(solveAllEvents[thid]) == 0)
    return 0;

  return 1;
}


DWORD CALLBACK SolveChunkDDtable (void *)
{
  int thid;
  thid = InterlockedIncrement(&threadIndex);

  SolveChunkDDtableCommon(thid);

  if (SetEvent(solveAllEvents[thid]) == 0)
    return 0;

  return 1;
}


int SolveInitThreadsWindows()
{
  for (int k = 0; k < noOfThreads; k++)
  {
    solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }
  return RETURN_NO_FAULT;
}


int SolveRunThreadsWindows(
  const int chunkSize)
{
  if (chunkSize == 1)
  {
    for (int k = 0; k < noOfThreads; k++)
    {
      int res = QueueUserWorkItem(SolveChunk, NULL,
                              WT_EXECUTELONGFUNCTION);
      if (res != 1)
        return res;
    }
  }
  else
  {
    for (int k = 0; k < noOfThreads; k++)
    {
      int res = QueueUserWorkItem(SolveChunkDDtable, NULL,
                              WT_EXECUTELONGFUNCTION);
      if (res != 1)
        return res;
    }
  }

  START_BLOCK_TIMER;
  DWORD solveAllWaitResult;
  solveAllWaitResult = WaitForMultipleObjects(
                         static_cast<unsigned>(noOfThreads),
                         solveAllEvents, TRUE, INFINITE);
  END_BLOCK_TIMER;

  if (solveAllWaitResult != WAIT_OBJECT_0)
    return RETURN_THREAD_WAIT;

  for (int k = 0; k < noOfThreads; k++)
    CloseHandle(solveAllEvents[k]);

  return RETURN_NO_FAULT;
}


int SolveAllBoardsN(
  boards * bop,
  solvedBoards * solvedp,
  int chunkSize,
  int source) // 0 solve, 1 calc
{
  threadIndex = -1;
  chunk = chunkSize;
  param.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  int retInit = SolveInitThreadsWindows();
  if (retInit != RETURN_NO_FAULT)
    return retInit;

  param.bop = bop;
  param.solvedp = solvedp;
  param.noOfBoards = bop->noOfBoards;

  if (source == 0)
    scheduler.Register(bop, SCHEDULER_SOLVE);
  else
    scheduler.Register(bop, SCHEDULER_CALC);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  int retRun = SolveRunThreadsWindows(chunkSize);
  if (retRun != RETURN_NO_FAULT)
    return retRun;

  /* Calculate number of solved boards. */
  solvedp->noOfBoards = param.noOfBoards;

  if (param.error == 0)
    return 1;
  else
    return param.error;
}

#elif (defined(DDS_THREADS_BOOST))

// TODO

vector<thread *> threads;

int SolveInitThreadsBoost()
{
  threads.resize(static_cast<unsigned>(noOfThreads));
  return RETURN_NO_FAULT;
}


int SolveRunThreadsBoost(
  const int chunkSize)
{
  const unsigned noth = static_cast<unsigned>(noOfThreads);
  START_BLOCK_TIMER;
  if (chunkSize == 1)
  {
    for (unsigned k = 0; k < noth; k++)
      threads[k] = new thread(SolveChunkCommon, k);
  }
  else
  {
    for (unsigned k = 0; k < noth; k++)
      threads[k] = new thread(SolveChunkDDtableCommon, k);
  }
  END_BLOCK_TIMER;

  for (unsigned k = 0; k < noth; k++)
  {
   threads[k]->join();
   delete threads[k];
  }
  return RETURN_NO_FAULT;
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

  int retInit = SolveInitThreadsBoost();
  if (retInit != RETURN_NO_FAULT)
    return retInit;

  param.bop = bop;
  param.solvedp = solvedp;
  param.noOfBoards = bop->noOfBoards;

  if (source == 0)
    scheduler.Register(bop, SCHEDULER_SOLVE);
  else
    scheduler.Register(bop, SCHEDULER_CALC);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  int retRun = SolveRunThreadsBoost(chunkSize);
  if (retRun != RETURN_NO_FAULT)
    return retRun;

  /* Calculate number of solved boards. */
  solvedp->noOfBoards = param.noOfBoards;

  if (param.error == 0)
    return 1;
  else
    return param.error;
}


#elif (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__MAC_OS_X_VERSION_MAX_ALLOWED)) && !defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)

// This code for LLVM multi-threading on the Mac was kindly
/// contributed by Pierre Cossard.

int SolveAllBoardsN(
  boards * bop,
  solvedBoards * solvedp,
  int chunkSize,
  int source) // 0 solve, 1 calc
{
  __block int chunk;
  __block int fail;
    
  chunk = chunkSize;
  fail = 1;
    
  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;
    
  futureTricks *fut = static_cast<futureTricks *>
    (calloc(MAXNOOFBOARDS, sizeof(futureTricks)));
    
  for (int i = 0; i < MAXNOOFBOARDS; i++)
      solvedp->solvedBoard[i].cards = 0;
    
  START_BLOCK_TIMER;
    
  if (source == 0)
    scheduler.Register(bop, SCHEDULER_SOLVE);
  else
    scheduler.Register(bop, SCHEDULER_CALC);
    
  if (chunkSize == 1)
  {
    dispatch_apply(static_cast<size_t>(noOfThreads),
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), 
      ^(size_t t)
    {
      while (1)
      {
        int thid = static_cast<int>(t);
        schedType st = scheduler.GetNumber(thid);
        int index = st.number;
        if (index == -1)
          break;
                
        // This is not a perfect repeat detector, as the hands in
        // a group might have declarers N, S, N, N. Then the second
        // N would not reuse the first N. However, most reuses are
        // reasonably adjacent, and this is just an optimization anyway.
                
        if (st.repeatOf != -1 &&
          (bop->deals[index ].first ==
          bop->deals[st.repeatOf].first))
        {
          START_THREAD_TIMER(thid);
          solvedp->solvedBoard[index] = fut[ st.repeatOf ];
          END_THREAD_TIMER(thid);
          continue;
        }
        else
        {
          START_THREAD_TIMER(thid);
          int res = SolveBoard(
            bop->deals[index],
            bop->target[index],
            bop->solutions[index],
            bop->mode[index],
            &fut[index],
            thid);
          END_THREAD_TIMER(thid);
                    
          if (res == 1)
            solvedp->solvedBoard[index] = fut[index];
          else
            fail = res;
             
        }
      }
    });
        
  }
  else
  {
    dispatch_apply(static_cast<size_t>(noOfThreads),
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), 
      ^(size_t t)
    {
      while (1)
      {
        int thid = static_cast<int>(t);
               
        schedType st = scheduler.GetNumber(thid);
        int index = st.number;
        if (index == -1)
          break;
                
        if (st.repeatOf != -1)
        {
          START_THREAD_TIMER(thid);
          for (int k = 0; k < chunk; k++)
          {
            bop->deals[index].first = k;
                       
            solvedp->solvedBoard[index].score[k] =
            solvedp->solvedBoard[ st.repeatOf ].score[k];
          }
          END_THREAD_TIMER(thid);
          continue;
        }
                
        bop->deals[index].first = 0;
                
        START_THREAD_TIMER(thid);
        int res = SolveBoard(
          bop->deals[index],
          bop->target[index],
          bop->solutions[index],
          bop->mode[index],
          &fut[index],
          thid);
                
        // SH: I'm making a terrible use of the fut structure here.
                
        if (res == 1)
          solvedp->solvedBoard[index].score[0] = fut[index].score[0];
        else
          fail = res;
                
        for (int k = 1; k < chunk; k++)
        {
          int hint = (k == 2 ? fut[index].score[0] :
            13 - fut[index].score[0]);
                    
          bop->deals[index].first = k; // Next declarer
                    
          res = SolveSameBoard(
            bop->deals[index],
            &fut[index],
            hint,
            thid);
                    
          if (res == 1)
            solvedp->solvedBoard[index].score[k] =
            fut[index].score[0];
          else
            fail = res;
        }
        END_THREAD_TIMER(thid);
      }
    });
  }
    
  END_BLOCK_TIMER;
    
  free(fut);
    
  if (fail != 1)
    return fail;
    
  solvedp->noOfBoards = 0;
  for (int i = 0; i < MAXNOOFBOARDS; i++)
    if (solvedp->solvedBoard[i].cards != 0)
      solvedp->noOfBoards++;
    
  return 1;
}

#elif (defined(_OPENMP))

int SolveInitThreadsOpenMP();
int SolveRunThreadsOpenMP(const int chunkSize);


int SolveInitThreadsOpenMP()
{
  /* Added after suggestion by Dirk Willecke. */
  if (omp_get_dynamic())
    omp_set_dynamic(0);

  omp_set_num_threads(noOfThreads);

  return RETURN_NO_FAULT;
}


int SolveRunThreadsOpenMP(
  const int chunkSize)
{
  int thid;

  START_BLOCK_TIMER;
  if (chunkSize == 1)
  {
    #pragma omp parallel default(none) shared(scheduler) private(thid)
    {
      #pragma omp while schedule(dynamic, chunk)
      thid = omp_get_thread_num();
      SolveChunkCommon(thid);
    }
  }
  else
  {
    #pragma omp parallel default(none) shared(scheduler) private(thid)
    {
      #pragma omp while schedule(dynamic, chunk)
      thid = omp_get_thread_num();
      SolveChunkDDtableCommon(thid);
    }
  }
  END_BLOCK_TIMER;

  return RETURN_NO_FAULT;
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

  int retInit = SolveInitThreadsOpenMP();
  if (retInit != RETURN_NO_FAULT)
    return retInit;

  param.bop = bop;
  param.solvedp = solvedp;
  param.noOfBoards = bop->noOfBoards;

  if (source == 0)
    scheduler.Register(bop, SCHEDULER_SOLVE);
  else
    scheduler.Register(bop, SCHEDULER_CALC);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  int retRun = SolveRunThreadsOpenMP(chunkSize);
  if (retRun != RETURN_NO_FAULT)
    return retRun;

  /* Calculate number of solved boards. */
  solvedp->noOfBoards = param.noOfBoards;

  if (param.error == 0)
    return 1;
  else
    return param.error;
}

#endif


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


