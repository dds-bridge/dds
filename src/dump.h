/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DUMP_H
#define DDS_DUMP_H

#include "dds.h"


int DumpInput(
  const int errCode,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode);

void DumpRetrieved(
  FILE * fp,
  pos * posPoint,
  nodeCardsType * np,
  int target,
  int depth);

void DumpStored(
  FILE * fp,
  pos * posPoint,
  Moves * moves,
  nodeCardsType * np,
  int target,
  int depth);

#endif

