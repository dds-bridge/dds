/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <solver_context/solver_context.hpp>

/** Pre-create SolverContext instances for worker slots 0..num_threads-1. */
auto ensure_worker_contexts(const int num_threads) -> void;

/** Return the persistent SolverContext for a worker slot (call after ensure). */
auto worker_context_for(const int thr_id) -> SolverContext&;

/** Drop all pooled SolverContext instances (e.g. on FreeMemory). */
auto clear_worker_contexts() -> void;
