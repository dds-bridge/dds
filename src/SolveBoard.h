/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_H
#define DDS_SOLVEBOARD_H


#include "Scheduler.h"


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
