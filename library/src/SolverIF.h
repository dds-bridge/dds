/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVERIF_H
#define DDS_SOLVERIF_H

#include <api/dds.h>
#include <solver_context/SolverContext.h>
#include <memory>

int SolveBoardInternal(
  SolverContext& ctx,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  futureTricks * futp);

int SolveSameBoard(
  const std::shared_ptr<ThreadData>& thrp,
  const deal& dl,
  futureTricks * futp,
  const int hint);

int AnalyseLaterBoard(
  const std::shared_ptr<ThreadData>& thrp,
  const int leadHand,
  moveType const * move,
  const int hint,
  const int hintDir,
  futureTricks * futp);

#endif
