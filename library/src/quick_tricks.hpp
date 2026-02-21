/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds.h>
#include <system/memory.hpp>
#include <solver_context/solver_context.hpp>


int QuickTricks(
  Pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  bool& result,
  SolverContext& ctx);

bool QuickTricksSecondHand(
  Pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  SolverContext& ctx);
