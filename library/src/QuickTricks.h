/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_QUICKTRICKS_H
#define DDS_QUICKTRICKS_H

#include "dds/dds.h"
#include "system/Memory.h"
#include <solver_context/SolverContext.h>


int QuickTricks(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  bool& result,
  SolverContext& ctx);

bool QuickTricksSecondHand(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  SolverContext& ctx);

#endif
