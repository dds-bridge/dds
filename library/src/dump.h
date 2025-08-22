/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DUMP_H
#define DDS_DUMP_H

#include "dds.h"
#include "Moves.h"
#include "Memory.h"

int DumpInput(
  const int errCode,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode);

void DumpTopLevel(
  ofstream& fout,
  const ThreadData& thrd,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode);

void DumpRetrieved(
  ofstream& fout,
  const pos& tpos,
  const nodeCardsType& node,
  const int target,
  const int depth);

void DumpStored(
  ofstream& fout,
  const pos& tpos,
  const Moves& moves,
  const nodeCardsType& node,
  const int target,
  const int depth);

#endif

