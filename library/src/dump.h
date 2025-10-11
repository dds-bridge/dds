/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DUMP_H
#define DDS_DUMP_H

#include "dds/dds.h"
#include "moves/Moves.h"
#include "system/SolverContext.h"
#include "system/Memory.h"
#include "trans_table/TransTable.h" // for nodeCardsType and enums

// Forward declaration left for clarity; full type is included above.
struct nodeCardsType;

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

// Convenience overload to avoid direct Moves exposure at call sites
void DumpStored(
  ofstream& fout,
  const pos& tpos,
  SolverContext& ctx,
  const nodeCardsType& node,
  const int target,
  const int depth);

#endif

