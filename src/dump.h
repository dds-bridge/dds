/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DUMP_H
#define DDS_DUMP_H

#include "dds.h"

#define DDS_HAND_LINES 12
#define DDS_FULL_LINE 80

int DumpInput(
  const int errCode,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode);

void DumpTopLevel(
  ThreadData const * thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode);

void DumpRetrieved(
  const string& fname,
  pos const * posPoint,
  nodeCardsType const * np,
  const int target,
  const int depth);

void DumpStored(
  const string& fname,
  pos const * posPoint,
  Moves const * moves,
  nodeCardsType const * np,
  const int target,
  const int depth);

#endif

