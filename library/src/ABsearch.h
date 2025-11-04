/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_ABSEARCH_H
#define DDS_ABSEARCH_H

#include <dds/dds.h>
#include <system/SolverContext.h>


bool ABsearch(
  pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch0(
  pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch1(
  pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch2(
  pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

bool ABsearch3(
  pos * posPoint,
  const int target,
  const int depth,
  SolverContext& ctx);

void Make0(
  pos * posPoint,
  const int depth,
  moveType const * mply);

void Make1(
  pos * posPoint,
  const int depth,
  moveType const * mply);

void Make2(
  pos * posPoint,
  const int depth,
  moveType const * mply);

void Make3(
  pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  moveType const * mply,
  const std::shared_ptr<ThreadData>& thrp);

evalType Evaluate(
  pos const * posPoint,
  const int trump,
  const std::shared_ptr<ThreadData>& thrp);

#endif
