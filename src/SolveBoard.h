/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_H
#define DDS_SOLVEBOARD_H

#include "dds.h"


void SolveChunkCommon(
  const int thrId);

void SolveChunkDDtableCommon(
  const int thrId);

int SolveAllBoardsN(
  boards * bop,
  solvedBoards * solvedp,
  const int chunkSize,
  const int source); // 0 source, 1 calc

#endif
