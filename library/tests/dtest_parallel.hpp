/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <functional>

/// Resolve the worker thread count for a dtest batch.
///
/// @param requested Thread count from -n (0 = auto from hardware).
/// @param workload Number of independent items in the batch.
/// @return Thread count in [1, workload].
int dtest_effective_threads(int requested, int workload);

/// Run @p body for each index in [0, count) using up to @p requested_threads workers.
///
/// @p body must return RETURN_NO_FAULT (1) on success.
/// @return First non-success code from @p body, or RETURN_NO_FAULT.
int dtest_run_parallel(
  int count,
  int requested_threads,
  const std::function<int(int)> & body);
