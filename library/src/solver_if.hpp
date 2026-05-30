/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>
#include <memory>

auto solve_board_internal(
  SolverContext& ctx,
  const Deal& dl,
  const int target,
  const int solutions,
  const int mode,
  FutureTricks * futp) -> int;

auto solve_same_board(
  SolverContext& ctx,
  const Deal& dl,
  FutureTricks * futp,
  const int hint) -> int;

auto analyse_later_board(
  SolverContext& ctx,
  const int leadHand,
  MoveType const * move,
  const int hint,
  const int hintDir,
  FutureTricks * futp) -> int;
