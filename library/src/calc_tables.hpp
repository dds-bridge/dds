/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_CALCTABLES_H
#define DDS_CALCTABLES_H

#include <vector>

#include <api/dll.h>

using namespace std;


/**
 * @brief Perform common single-board calculations for double dummy analysis.
 *
 * Computes results for a single board using the specified thread ID.
 *
 * @param thrID Thread identifier for parallel execution.
 * @param bno Board number to analyze.
 */
void CalcSingleCommon(
  const int thrID,
  const int bno);

/**
 * @brief Copy calculation results for single boards based on cross-references.
 *
 * Copies results from previously computed boards as indicated by the cross-reference vector.
 *
 * @param crossrefs Vector of cross-reference indices mapping boards to be copied.
 */
void CopyCalcSingle(
  const vector<int>& crossrefs);

/**
 * @brief Perform common chunk calculations for a set of boards.
 *
 * Computes results for a chunk (batch) of boards using the specified thread ID.
 *
 * @param thrId Thread identifier for parallel execution.
 */
void CalcChunkCommon(
  const int thrId);

/**
 * @brief Detect duplicate board calculations and build cross-reference maps.
 *
 * Identifies unique and duplicate boards in a batch, populating vectors for unique indices and cross-references.
 *
 * @param bds Boards to analyze for duplicates.
 * @param uniques Output vector of indices for unique boards.
 * @param crossrefs Output vector mapping each board to its unique representative.
 */
void DetectCalcDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs);

#endif
