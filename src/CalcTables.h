/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_CALCTABLES_H
#define DDS_CALCTABLES_H

#include <vector>

#include "dds.h"

using namespace std;


void CalcSingleCommon(
  const int thrID,
  const int bno);

void CopyCalcSingle(
  const vector<int>& crossrefs);

void CalcChunkCommon(
  const int thrId);

void DetectCalcDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs);

#endif
