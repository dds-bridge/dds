/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <vector>

#include <api/dll.h>
#include <solver_context/solver_context.hpp>


/**
 * @brief Perform common single-board calculations with explicit solver context.
 *
 * Context-aware version that allows TT reuse across calculations. All
 * input/output buffers are passed by pointer so concurrent calls on
 * distinct SolverContexts don't share any state.
 *
 * @param ctx Solver context for resource management
 * @param bop Input boards (read-only)
 * @param solvedp Output solved boards (the bno-th slot is written)
 * @param bno Board number to analyze.
 * @return RETURN_NO_FAULT on success, error code otherwise.
 */
auto calc_single_common_internal(
  SolverContext& ctx,
  const Boards * bop,
  SolvedBoards * solvedp,
  const int bno) -> int;

/**
 * @brief Calculate all boards with explicit solver context.
 *
 * Context-aware version enabling transposition table reuse.
 *
 * @param ctx Solver context for resource management
 * @param bop Input boards to solve
 * @param solvedp Output solved boards
 * @return Error code
 */
auto calc_all_boards_n(
  SolverContext& ctx,
  Boards * bop,
  SolvedBoards * solvedp) -> int;

/**
 * @brief Detect duplicate board calculations and build cross-reference maps.
 *
 * Identifies unique and duplicate Boards in a batch, populating vectors for unique indices and cross-references.
 *
 * @param bds Boards to analyze for duplicates.
 * @param uniques Output vector of indices for unique Boards.
 * @param crossrefs Output vector mapping each board to its unique representative.
 */
auto detect_calc_duplicates(
  const Boards& bds,
  std::vector<int>& uniques,
  std::vector<int>& crossrefs) -> void;
