/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_OPENMP_H
#define DDS_SOLVEBOARD_OPENMP_H

int SolveInitThreadsOpenMP();

int SolveRunThreadsOpenMP(
  const int chunkSize);

#endif
