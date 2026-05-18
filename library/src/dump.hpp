/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds.h>
#include <moves/moves.hpp>
#include <solver_context/solver_context.hpp>

int DumpInput(
  const int errCode,
  const Deal& dl,
  const int target,
  const int solutions,
  const int mode);

void DumpTopLevel(
  std::ofstream& fout,
  const std::shared_ptr<ThreadData>& thrp,
  const int tricks,
  const int lower,
  const int upper,
  const int printMode);

void DumpRetrieved(
  std::ofstream& fout,
  const Pos& tpos,
  const NodeCards& node,
  const int target,
  const int depth);

void DumpStored(
  std::ofstream& fout,
  const Pos& tpos,
  const Moves& moves,
  const NodeCards& node,
  const int target,
  const int depth);

// Convenience overload to avoid direct Moves exposure at call sites
void DumpStored(
  std::ofstream& fout,
  const Pos& tpos,
  SolverContext& ctx,
  const NodeCards& node,
  const int target,
  const int depth);

