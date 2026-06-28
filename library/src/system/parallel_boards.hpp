/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

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
 *        the real board number, so result placement is unaffected.
 * @return First non-success code from @p process_board, or RETURN_NO_FAULT.
 */
auto parallel_all_boards_n(
  int count,
  int worker_cap,
  const std::function<int(int worker_id, int bno)>& process_board,
  const std::vector<int>* order = nullptr) -> int;
