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
 * @brief Solve a single board. Creates a temporary context per call.
 */
auto calc_single_common(const int bno) -> void;

/**
 * @brief Solve a single board using an explicit solver context for TT reuse.
 */
auto calc_single_common_internal(
  SolverContext& ctx,
  const int bno) -> void;

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
 * @brief Copy calculation results for single Boards based on cross-references.
 *
 * Copies results from previously computed Boards as indicated by the cross-reference vector.
 *
 * @param crossrefs Vector of cross-reference indices mapping Boards to be copied.
 */
auto copy_calc_single(
  const std::vector<int>& crossrefs) -> void;

/**
 * @brief Process all boards queued in the scheduler (sequential).
 */
auto calc_chunk_common() -> void;

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
