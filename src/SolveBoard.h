/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_H
#define DDS_SOLVEBOARD_H


#include "Scheduler.h"

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


void SolveChunkCommon(
  const int thid);

void SolveChunkDDtableCommon(
  const int thid);

int SolveAllBoardsN(
  struct boards * bop,
  struct solvedBoards * solvedp,
  int chunkSize,
  int source); // 0 source, 1 calc

#endif
