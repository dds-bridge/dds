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
#include "PBN.h"

extern int noOfThreads;

#if (defined(_WIN32) || defined(__CYGWIN__)) && \
     !defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)
HANDLE solveAllEvents[MAXNOOFTHREADS];
struct paramType param;
LONG volatile threadIndex;
LONG volatile current;

long chunk;

DWORD CALLBACK SolveChunk (void *) {
  struct futureTricks fut[MAXNOOFBOARDS];
  int thid;
  long j;

  thid=InterlockedIncrement(&threadIndex);

  while ((j=(InterlockedIncrement(&current)-1))<param.noOfBoards) {

    int res=SolveBoard(param.bop->deals[j], param.bop->target[j],
    param.bop->solutions[j], param.bop->mode[j], &fut[j], thid);
    if (res==1) {
      param.solvedp->solvedBoard[j]=fut[j];
      /*param.error=0;*/
    }
    else {
      param.error=res;
    }
  }

  if (SetEvent(solveAllEvents[thid])==0) {
    /*int errCode=GetLastError();*/
    return 0;
  }

  return 1;

}


DWORD CALLBACK SolveChunkDDtable (void *) {
  struct futureTricks fut[MAXNOOFBOARDS];
  int thid;
  long j;

  thid=InterlockedIncrement(&threadIndex);

  while ((j=InterlockedExchangeAdd(&current, chunk))<param.noOfBoards) {

    for (int k=0; k<chunk && j+k<param.noOfBoards; k++) {
      int res=SolveBoard(param.bop->deals[j+k], param.bop->target[j+k],
	  param.bop->solutions[j+k], param.bop->mode[j+k], 
	  &fut[j+k], thid);
      if (res==1) {
	param.solvedp->solvedBoard[j+k]=fut[j+k];
	/*param.error=0;*/
      }
      else {
	param.error=res;
      }
    }
  }

  if (SetEvent(solveAllEvents[thid])==0) {
    /*int errCode=GetLastError();*/
    return 0;
  }

  return 1;

}


int SolveAllBoardsN(struct boards *bop, struct solvedBoards *solvedp, int chunkSize) {
  int k/*, errCode*/;
  DWORD res;
  DWORD solveAllWaitResult;

  current = 0;
  threadIndex = -1;
  chunk = chunkSize;
  param.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

    for (k = 0; k<noOfThreads; k++) {
      solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
      if (solveAllEvents[k] == 0) {
	/*errCode=GetLastError();*/
	return RETURN_THREAD_CREATE;
      }
    }

    param.bop = bop; param.solvedp = solvedp; param.noOfBoards = bop->noOfBoards;

    for (k = 0; k<MAXNOOFBOARDS; k++)
      solvedp->solvedBoard[k].cards = 0;

    if (chunkSize != 1) {
      for (k = 0; k<noOfThreads; k++) {
        res = QueueUserWorkItem(SolveChunkDDtable, NULL, WT_EXECUTELONGFUNCTION);
        if (res != 1) {
	  /*errCode=GetLastError();*/
	  return res;
        }
      }
    }
    else {
      for (k=0; k<noOfThreads; k++) {
        res=QueueUserWorkItem(SolveChunk, NULL, WT_EXECUTELONGFUNCTION);
        if (res!=1) {
          /*errCode=GetLastError();*/
          return res;
        }
      }
    }

    solveAllWaitResult = WaitForMultipleObjects(noOfThreads, solveAllEvents, TRUE, INFINITE);
    if (solveAllWaitResult != WAIT_OBJECT_0) {
      /*errCode=GetLastError();*/
      return RETURN_THREAD_WAIT;
    }

    for (k = 0; k<noOfThreads; k++) {
      CloseHandle(solveAllEvents[k]);
    }

    /* Calculate number of solved boards. */

    solvedp->noOfBoards = 0;
    for (k = 0; k<MAXNOOFBOARDS; k++) {
      if (solvedp->solvedBoard[k].cards != 0)
	solvedp->noOfBoards++;
    }

    if (param.error == 0)
      return 1;
    else
      return param.error;
}

#else

int SolveAllBoardsN(struct boards *bop, struct solvedBoards *solvedp, int chunkSize) {
  int k, i, res, chunk, fail;
  struct futureTricks fut[MAXNOOFBOARDS];

  chunk=chunkSize; fail=1;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (i=0; i<MAXNOOFBOARDS; i++)
      solvedp->solvedBoard[i].cards=0;

#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
  if (omp_get_dynamic())
    omp_set_dynamic(0);
  omp_set_num_threads(noOfThreads);	/* Added after suggestion by Dirk Willecke. */
#elif defined (_OPENMP)
  omp_set_num_threads(1);
#endif

  #pragma omp parallel shared(bop, solvedp, chunk, fail) private(k)
  {

    #pragma omp for schedule(dynamic, chunk)

    for (k=0; k<bop->noOfBoards; k++) {
      res=SolveBoard(bop->deals[k], bop->target[k], bop->solutions[k],
        bop->mode[k], &fut[k],
 
#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
		omp_get_thread_num()
#else
		0
#endif
		);
      if (res==1) {
        solvedp->solvedBoard[k]=fut[k];
      }
      else
        fail=res;
    }
  }

  if (fail!=1)
    return fail;

  solvedp->noOfBoards=0;
  for (i=0; i<MAXNOOFBOARDS; i++) {
    if (solvedp->solvedBoard[i].cards!=0)
      solvedp->noOfBoards++;
  }

  return 1;
}

#endif


int STDCALL SolveBoardPBN(struct dealPBN dlpbn, int target,
    int solutions, int mode, struct futureTricks *futp, int threadIndex) {

  int res, k;
  struct deal dl;

  if (ConvertFromPBN(dlpbn.remainCards, dl.remainCards)!=RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  for (k=0; k<=2; k++) {
    dl.currentTrickRank[k]=dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k]=dlpbn.currentTrickSuit[k];
  }
  dl.first=dlpbn.first;
  dl.trump=dlpbn.trump;

  res=SolveBoard(dl, target, solutions, mode, futp, threadIndex);

  return res;
}


int STDCALL SolveAllBoards(struct boardsPBN *bop, struct solvedBoards *solvedp) {
  struct boards bo;
  int k, i, res;

  bo.noOfBoards=bop->noOfBoards;
  for (k=0; k<bop->noOfBoards; k++) {
    bo.mode[k]=bop->mode[k];
    bo.solutions[k]=bop->solutions[k];
    bo.target[k]=bop->target[k];
    bo.deals[k].first=bop->deals[k].first;
    bo.deals[k].trump=bop->deals[k].trump;
    for (i=0; i<=2; i++) {
      bo.deals[k].currentTrickSuit[i]=bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i]=bop->deals[k].currentTrickRank[i];
    }
    if (ConvertFromPBN(bop->deals[k].remainCards, bo.deals[k].remainCards)!=1)
      return RETURN_PBN_FAULT;
  }

  res=SolveAllBoardsN(&bo, solvedp, 1);

  return res;
}

int STDCALL SolveAllChunksPBN(struct boardsPBN *bop, struct solvedBoards *solvedp, int chunkSize) {
  struct boards bo;
  int k, i, res;

  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  bo.noOfBoards = bop->noOfBoards;
  for (k = 0; k<bop->noOfBoards; k++) {
    bo.mode[k] = bop->mode[k];
    bo.solutions[k] = bop->solutions[k];
    bo.target[k] = bop->target[k];
    bo.deals[k].first = bop->deals[k].first;
    bo.deals[k].trump = bop->deals[k].trump;
    for (i = 0; i <= 2; i++) {
      bo.deals[k].currentTrickSuit[i] = bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i] = bop->deals[k].currentTrickRank[i];
    }
    if (ConvertFromPBN(bop->deals[k].remainCards, bo.deals[k].remainCards) != 1)
      return RETURN_PBN_FAULT;
  }

  res = SolveAllBoardsN(&bo, solvedp, chunkSize);
  return res;
}


int STDCALL SolveAllChunks(struct boardsPBN *bop, struct solvedBoards *solvedp, int chunkSize) {

  int res = SolveAllChunksPBN(bop, solvedp, chunkSize);

  return res;
} 


int STDCALL SolveAllChunksBin(struct boards *bop, struct solvedBoards *solvedp, int chunkSize) {
  int res;

  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  res = SolveAllBoardsN(bop, solvedp, chunkSize);
  return res;
}


