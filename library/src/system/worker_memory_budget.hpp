/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/**
 * @brief Clamp a worker count to a memory budget (MB).
 *
 * Each parallel worker owns a SolverContext with a Large transposition table
 * plus stack; under Emscripten the wasm32 heap tops out near 2 GiB. The
 * implementation is in worker_memory_budget.cpp so this helper stays out of the
 * native parallel_boards translation unit.
 *
 * @param workers Requested workers (values < 1 become 1)
 * @param budget_mb Total MB available for worker TT/stack footprints
 * @param per_worker_mb Assumed MB cost of one worker
 * @return workers clamped to max(1, budget_mb / per_worker_mb)
 */
auto clamp_workers_to_memory_budget(
  int workers,
  int budget_mb,
  int per_worker_mb) -> int;
