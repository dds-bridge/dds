/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_H
#define DDS_SOLVEBOARD_H

#include <vector>

#include "dds.h"

using namespace std;


void SolveSingleCommon(
  const int thrId,
  const int bno);

void CopySolveSingle(
  const vector<int>& crossrefs);

void SolveChunkCommon(
  const int thrId);

void DetectSolveDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs);

#endif
