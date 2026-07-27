/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <cstdint>
#include <functional>
#include <vector>


/**
 * @brief Resolve the number of worker threads to use.
 *
 * @param max_threads Requested cap; <= 0 means "auto" (use hardware concurrency).
 * @param count Number of work items; the result is clamped to [1, count] when count > 0 and to 1 when count <= 0.
 * @return The worker count to use.
 */
auto resolve_worker_count(int max_threads, int count) -> int;

/**
 * @brief Process boards [0, count) with work-stealing parallelism.
 *
 * @param count Number of board indices to process.
 * @param worker_cap Maximum worker threads; <= 0 uses hardware concurrency.
 * @param process_board Called for each board; must return RETURN_NO_FAULT (1)
 *        on success. Receives the worker's thread index and board number.
 * @param order Optional dispatch order: a permutation of [0, count) giving the
 *        sequence in which board numbers are handed out (e.g. hardest first to
 *        shorten the tail). When null/empty, boards are dispatched in index
 *        order. Only the dispatch order changes; @p process_board still receives
 *        the real board number, so result placement is unaffected. When
 *        non-null, the vector must remain valid and must not be mutated until
 *        this function returns because worker threads read it concurrently.
 * @return First non-success code from @p process_board, or RETURN_NO_FAULT.
 *         If @p process_board throws during a multi-worker run, the exception is
 *         caught on the worker, the run returns RETURN_UNKNOWN_FAULT, and the
 *         pool stays usable. Single-worker runs leave exceptions to the caller.
 *
 * Multi-worker runs use a process-local persistent thread pool so consecutive
 * calls reuse OS threads instead of create/join each time. Single-worker runs
 * stay on the calling thread. The pool handles one job at a time; concurrent
 * multi-worker calls from different threads are safe but serialize against
 * each other. Not re-entrant: calling this again with worker_cap > 1 from
 * inside @p process_board of another multi-worker run deadlocks (the inner
 * call blocks on the pool mutex while the outer run waits for that worker).
 * Call dds::internal::shutdown_parallel_boards_pool() before process exit
 * when hosts (notably Emscripten) tear down pthread Workers eagerly.
 */
auto parallel_all_boards_n(
  int count,
  int worker_cap,
  const std::function<int(int worker_id, int bno)>& process_board,
  const std::vector<int>* order = nullptr) -> int;

namespace dds::internal
{

// Cumulative number of OS worker threads created by the board pool (test seam).
auto parallel_boards_worker_threads_created() -> std::uint64_t;

// Join and destroy the process-local board worker pool. Safe to call with no
// pool, and again after a later parallel_all_boards_n recreates it.
void shutdown_parallel_boards_pool();

}  // namespace dds::internal
