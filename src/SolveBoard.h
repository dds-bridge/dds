/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2015 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_H
#define DDS_SOLVEBOARD_H


int SolveAllBoardsN(
  struct boards * bop,
  struct solvedBoards * solvedp,
  int chunkSize,
  int source); // 0 source, 1 calc

#endif
