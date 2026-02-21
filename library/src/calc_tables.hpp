/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <vector>

#include <api/dll.h>


/**
 * @brief Perform common single-board calculations for double dummy analysis.
 *
 * Computes results for a single board using the specified thread ID.
 *
 * @param thrID Thread identifier for parallel execution.
 * @param bno Board number to analyze.
 */
auto calc_single_common(
  const int thrID,
  const int bno) -> void;

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
 * @brief Perform common chunk calculations for a set of Boards.
 *
 * Computes results for a chunk (batch) of Boards using the specified thread ID.
 *
 * @param thrId Thread identifier for parallel execution.
 */
auto calc_chunk_common(
  const int thrId) -> void;

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
