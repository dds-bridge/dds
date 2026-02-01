/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_ABSEARCH_H
#define DDS_ABSEARCH_H

#include <api/dds.h>
#include <solver_context/solver_context.hpp>


bool ABsearch(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch0(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch1(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch2(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch3(
  Pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

void Make0(
  Pos * posPoint,
  const int depth,
  MoveType const * mply);

void Make1(
  Pos * posPoint,
  const int depth,
  MoveType const * mply);

void Make2(
  Pos * posPoint,
  const int depth,
  MoveType const * mply);

void Make3(
  Pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  MoveType const * mply,
  SolverContext& ctx);

// Evaluate terminal position using the provided context
EvalType EvaluateWithContext(
  Pos const * posPoint,
  const int trump,
  SolverContext& ctx);

#endif
