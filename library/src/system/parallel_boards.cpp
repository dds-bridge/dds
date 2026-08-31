/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "parallel_boards.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <api/dds_data_types.hpp>


namespace
{

std::atomic<std::uint64_t> g_threads_created{0};
std::atomic<int> g_last_job_board_count{0};


class BoardWorkerPool
{
public:
  BoardWorkerPool() = default;

  ~BoardWorkerPool()
  {
    {
      std::lock_guard<std::mutex> lock(mu_);
      stopping_ = true;
      ++generation_;
      job_ = nullptr;
      workers_for_job_ = 0;
    }
    cv_start_.notify_all();
    for (auto& th : threads_)
    {
      if (th.joinable())
        th.join();
    }
  }

  BoardWorkerPool(const BoardWorkerPool&) = delete;
  auto operator=(const BoardWorkerPool&) -> BoardWorkerPool& = delete;

  auto run(
    const int workers,
    const int count,
    const std::function<int(int worker_id, int bno)>& process_board,
    const bool use_order,
    const std::vector<int>* order) -> int
  {
    // The pool tracks exactly one outstanding job (job_, workers_for_job_,
    // generation_). Serialize whole runs so a second concurrent caller queues
    // up instead of overwriting the first caller's job, which would leave the
    // first run waiting forever for workers that never saw its job.
    std::lock_guard<std::mutex> run_lock(run_mu_);

    ensure_workers(workers);

    std::atomic<int> next{0};
    std::atomic<int> first_error{RETURN_NO_FAULT};
    // finished is protected by mu_: every worker increments it under that lock
    // so the unlock→lock handoff through cv_done_ happens-before the caller's
    // continued use of process_board side effects (e.g. result buffers).
    int finished{0};

    Job job{
      &process_board,
      order,
      &next,
      &first_error,
      &finished,
      count,
      workers,
      use_order};

    {
      std::lock_guard<std::mutex> lock(mu_);
      job_ = &job;
      workers_for_job_ = workers;
      ++generation_;
    }
    cv_start_.notify_all();

    {
      std::unique_lock<std::mutex> lock(mu_);
      cv_done_.wait(lock, [&] { return finished >= workers; });
      job_ = nullptr;
    }

    const int err = first_error.load(std::memory_order_relaxed);
    return err != RETURN_NO_FAULT ? err : RETURN_NO_FAULT;
  }

private:
  struct Job
  {
    const std::function<int(int, int)>* process_board = nullptr;
    const std::vector<int>* order = nullptr;
    std::atomic<int>* next = nullptr;
    std::atomic<int>* first_error = nullptr;
    int* finished = nullptr;
    int count = 0;
    int workers = 0;
    bool use_order = false;
  };

  void ensure_workers(const int workers)
  {
    std::lock_guard<std::mutex> lock(mu_);
    while (static_cast<int>(threads_.size()) < workers)
    {
      const int worker_id = static_cast<int>(threads_.size());
      threads_.emplace_back([this, worker_id] { worker_main(worker_id); });
      g_threads_created.fetch_add(1, std::memory_order_relaxed);
    }
  }

  void worker_main(const int worker_id)
  {
    std::uint64_t seen_generation = 0;
    for (;;)
    {
      Job local{};
      {
        std::unique_lock<std::mutex> lock(mu_);
        cv_start_.wait(lock, [&] {
          return stopping_ ||
            (job_ != nullptr &&
             generation_ != seen_generation &&
             worker_id < workers_for_job_);
        });
        if (stopping_)
          return;
        seen_generation = generation_;
        local = *job_;
      }

      // Workers with id >= local.workers for this job still wake on notify_all;
      // only the selected prefix participates and signals completion.
      if (worker_id < local.workers)
      {
        try
        {
          for (;;)
          {
            const int slot = local.next->fetch_add(1, std::memory_order_relaxed);
            if (slot >= local.count ||
                local.first_error->load(std::memory_order_relaxed) != RETURN_NO_FAULT)
            {
              break;
            }
            const int bno =
              local.use_order
                ? (*local.order)[static_cast<unsigned>(slot)]
                : slot;
            const int rc = (*local.process_board)(worker_id, bno);
            if (rc != RETURN_NO_FAULT)
            {
              int expected = RETURN_NO_FAULT;
              local.first_error->compare_exchange_strong(
                expected, rc, std::memory_order_relaxed);
              break;
            }
          }
        }
        catch (...)
        {
          // process_board must not leave the pool hanging or abort the process
          // via std::terminate. Any throw maps the run to RETURN_UNKNOWN_FAULT
          // (even if another worker already recorded a non-success return code),
          // then fall through to finished accounting so cv_done_ is signaled.
          local.first_error->store(
            RETURN_UNKNOWN_FAULT, std::memory_order_relaxed);
        }
        // Account completion under mu_ so (1) the predicate change cannot race
        // with cv_done_.wait's check-and-sleep (lost wakeup) and (2) unlocking
        // mu_ after process_board writes publishes those writes to the caller
        // when wait reacquires the mutex.
        bool notify = false;
        {
          std::lock_guard<std::mutex> lock(mu_);
          notify = ++(*local.finished) >= local.workers;
        }
        if (notify)
          cv_done_.notify_one();
      }
    }
  }

  // Held for the duration of run(); see the comment there.
  std::mutex run_mu_;
  std::mutex mu_;
  std::condition_variable cv_start_;
  std::condition_variable cv_done_;
  std::vector<std::thread> threads_;
  bool stopping_ = false;
  std::uint64_t generation_ = 0;
  Job* job_ = nullptr;
  int workers_for_job_ = 0;
};


struct PoolHolder
{
  std::mutex mu;
  std::shared_ptr<BoardWorkerPool> pool;
};


auto pool_holder() -> PoolHolder&
{
  static PoolHolder holder;
  return holder;
}


auto default_pool() -> std::shared_ptr<BoardWorkerPool>
{
  auto& h = pool_holder();
  std::lock_guard<std::mutex> lock(h.mu);
  if (!h.pool)
    h.pool = std::make_shared<BoardWorkerPool>();
  return h.pool;
}

}  // namespace


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


auto resolve_worker_count(
  const int max_threads,
  const int count) -> int
{
  int workers = max_threads;
  if (workers <= 0)
  {
    const unsigned hw = std::thread::hardware_concurrency();
    workers = hw > 0 ? static_cast<int>(hw) : 1;
  }
  workers = std::max(1, std::min(workers, count));

  // Parallel workers each keep a Large TT (via SolverContext). Under
  // Emscripten the wasm32 heap cannot grow past ~2 GiB; uncapped auto
  // (= HW concurrency) OOMs large multi-board batches. Native builds keep
  // the uncapped count — this block must stay __EMSCRIPTEN__-only.
#if defined(__EMSCRIPTEN__)
  constexpr int kHeapBudgetMB = 1400;
  constexpr int kPerWorkerMB = THREADMEM_LARGE_DEF_MB + 24;
  workers = clamp_workers_to_memory_budget(workers, kHeapBudgetMB, kPerWorkerMB);
#endif

  return workers;
}


static auto is_permutation_of_range(
  const std::vector<int>& order,
  const int count) -> bool
{
  std::vector<char> seen(static_cast<unsigned>(count), 0);
  for (const int v : order)
  {
    if (v < 0 || v >= count || seen[static_cast<unsigned>(v)])
      return false;
    seen[static_cast<unsigned>(v)] = 1;
  }
  return true;
}


namespace dds::internal
{

auto parallel_boards_worker_threads_created() -> std::uint64_t
{
  return g_threads_created.load(std::memory_order_relaxed);
}

auto parallel_boards_last_job_board_count() -> int
{
  return g_last_job_board_count.load(std::memory_order_relaxed);
}

void shutdown_parallel_boards_pool()
{
  std::shared_ptr<BoardWorkerPool> dying;
  {
    auto& h = pool_holder();
    std::lock_guard<std::mutex> lock(h.mu);
    dying = std::move(h.pool);
  }
  // Drop the last shared_ptr outside the holder lock so joins cannot deadlock
  // against a concurrent parallel_all_boards_n that needs the mutex to recreate
  // the pool. In-flight runs keep their own shared_ptr alive until run returns.
}

}  // namespace dds::internal


auto parallel_all_boards_n(
  const int count,
  const int worker_cap,
  const std::function<int(int worker_id, int bno)>& process_board,
  const std::vector<int>* order) -> int
{
  // Always record the requested count, including early-return paths, so the
  // test seam cannot report a stale prior job.
  g_last_job_board_count.store(count, std::memory_order_relaxed);

  if (count <= 0)
  {
    return RETURN_NO_FAULT;
  }

  // Map a dispatch slot to the board number to process. With an order, hand out
  // boards in that sequence (e.g. hardest first); otherwise in index order. The
  // order is only honored when it is a valid permutation of [0, count); a
  // malformed order falls back to index order to avoid invalid board indices.
  const bool use_order =
    (order != nullptr &&
     order->size() == static_cast<std::size_t>(count) &&
     is_permutation_of_range(*order, count));

  const int workers = resolve_worker_count(worker_cap, count);

  if (workers == 1)
  {
    for (int slot = 0; slot < count; ++slot)
    {
      const int bno =
        use_order ? (*order)[static_cast<unsigned>(slot)] : slot;
      const int rc = process_board(0, bno);
      if (rc != RETURN_NO_FAULT)
      {
        return rc;
      }
    }
    return RETURN_NO_FAULT;
  }

  return default_pool()->run(workers, count, process_board, use_order, order);
}
