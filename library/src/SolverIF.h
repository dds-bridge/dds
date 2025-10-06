/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVERIF_H
#define DDS_SOLVERIF_H

#include "dds/dds.h"
#include "system/Memory.h"
#include "system/SolverContext.h"

int SolveBoardWithContext(
  SolverContext& ctx,
  const deal& dl,
  int target,
  int solutions,
  int mode,
  futureTricks* futp);

int SolveBoardInternal(
  ThreadData * thrp,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  futureTricks * futp);

int SolveSameBoard(
  ThreadData * thrp,
  const deal& dl,
  futureTricks * futp,
  const int hint);

int AnalyseLaterBoard(
  ThreadData * thrp,
  const int leadHand,
  moveType const * move,
  const int hint,
  const int hintDir,
  futureTricks * futp);

#endif
