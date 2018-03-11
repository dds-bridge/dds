/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "SolveBoard.h"
#include "SolveBoard_WinAPI.h"


extern int noOfThreads;

#ifdef _WIN32
HANDLE solveAllEvents[MAXNOOFTHREADS];
LONG threadIndex;

DWORD CALLBACK SolveChunkWinAPI(void *);
DWORD CALLBACK SolveChunkDDtableWinAPI(void *);
#endif


#ifdef _WIN32
DWORD CALLBACK SolveChunkWinAPI(void *)
{
  int thid;
  thid = InterlockedIncrement(&threadIndex);

  SolveChunkCommon(thid);

  if (SetEvent(solveAllEvents[thid]) == 0)
    return 0;

  return 1;
}
#endif


#ifdef _WIN32
DWORD CALLBACK SolveChunkDDtableWinAPI(void *)
{
  int thid;
  thid = InterlockedIncrement(&threadIndex);

  SolveChunkDDtableCommon(thid);

  if (SetEvent(solveAllEvents[thid]) == 0)
    return 0;

  return 1;
}
#endif


int SolveInitThreadsWinAPI()
{
#ifdef _WIN32
  threadIndex = -1;

  for (int k = 0; k < noOfThreads; k++)
  {
    solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k] == 0)
      return RETURN_THREAD_CREATE;
  }
#endif

  return RETURN_NO_FAULT;
}


int SolveRunThreadsWinAPI(
  const int chunkSize)
{
#ifdef _WIN32
  if (chunkSize == 1)
  {
    for (int k = 0; k < noOfThreads; k++)
    {
      int res = QueueUserWorkItem(SolveChunkWinAPI, NULL,
                              WT_EXECUTELONGFUNCTION);
      if (res != 1)
        return res;
    }
  }
  else
  {
    for (int k = 0; k < noOfThreads; k++)
    {
      int res = QueueUserWorkItem(SolveChunkDDtableWinAPI, NULL,
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
#endif

  return RETURN_NO_FAULT;
}

