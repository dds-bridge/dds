/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "worker_context_pool.hpp"

#include <memory>
#include <mutex>
#include <vector>

#include <system/memory.hpp>


extern Memory memory;

namespace {

struct WorkerContextPool
{
  std::mutex mu;
  std::vector<std::unique_ptr<SolverContext>> slots;
};

WorkerContextPool& pool()
{
  static WorkerContextPool instance;
  return instance;
}

auto solver_config_for_thread(const unsigned thr_id) -> SolverConfig
{
  SolverConfig cfg;
  if (thr_id < memory.NumThreads() && memory.ThreadSize(thr_id) == "S")
    cfg.tt_kind_ = TTKind::Small;
  return cfg;
}

} // namespace

auto ensure_worker_contexts(const int num_threads) -> void
{
  if (num_threads <= 0)
    return;

  auto& p = pool();
  std::lock_guard<std::mutex> lock(p.mu);
  const unsigned n = static_cast<unsigned>(num_threads);
  if (p.slots.size() < n)
    p.slots.resize(n);
  for (unsigned k = 0; k < n; ++k)
  {
    if (!p.slots[k])
      p.slots[k] = std::make_unique<SolverContext>(solver_config_for_thread(k));
  }
}

auto worker_context_for(const int thr_id) -> SolverContext&
{
  return *pool().slots[static_cast<unsigned>(thr_id)];
}

auto clear_worker_contexts() -> void
{
  std::lock_guard<std::mutex> lock(pool().mu);
  pool().slots.clear();
}
