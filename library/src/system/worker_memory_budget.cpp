/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "worker_memory_budget.hpp"

#include <algorithm>


auto clamp_workers_to_memory_budget(
  const int workers,
  const int budget_mb,
  const int per_worker_mb) -> int
{
  const int safe_workers = std::max(1, workers);
  if (budget_mb <= 0 || per_worker_mb <= 0)
    return safe_workers;
  return std::max(1, std::min(safe_workers, budget_mb / per_worker_mb));
}
