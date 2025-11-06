/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_HPP
#define DDS_SOLVEBOARD_HPP

#include <api/dds.h>
#include <solver_context/SolverContext.hpp>

// C++-only overload exposed via <api/...> for C++ clients managing state.
int SolveBoard(
  SolverContext& ctx,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  futureTricks* futp);

#endif // DDS_SOLVEBOARD_HPP
