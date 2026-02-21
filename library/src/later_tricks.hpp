/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>


bool LaterTricksMIN(
  Pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  SolverContext& ctx);

bool LaterTricksMAX(
  Pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  SolverContext& ctx);
