/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DUMP_H
#define DDS_DUMP_H

#include <api/dds.h>
#include "moves/Moves.hpp"
#include <solver_context/SolverContext.hpp>
#include "system/Memory.hpp"
#include "trans_table/TransTable.hpp" // for NodeCards and enums

// NodeCards is provided via TransTable.hpp

int DumpInput(
  const int errCode,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode);

void DumpTopLevel(
  ofstream& fout,
  const std::shared_ptr<ThreadData>& thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode);

void DumpRetrieved(
  ofstream& fout,
  const pos& tpos,
  const NodeCards& node,
  const int target,
  const int depth);

void DumpStored(
  ofstream& fout,
  const pos& tpos,
  const Moves& moves,
  const NodeCards& node,
  const int target,
  const int depth);

// Convenience overload to avoid direct Moves exposure at call sites
void DumpStored(
  ofstream& fout,
  const pos& tpos,
  SolverContext& ctx,
  const NodeCards& node,
  const int target,
  const int depth);

#endif

