/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "parallel_boards.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

#include <api/dll.h>


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
  return std::max(1, std::min(workers, count));
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


auto parallel_all_boards_n(
  const int count,
  const int worker_cap,
  const std::function<int(int worker_id, int bno)>& process_board,
  const std::vector<int>* order) -> int
{
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
     static_cast<int>(order->size()) == count &&
     is_permutation_of_range(*order, count));
  auto board_of = [&](const int slot) -> int {
    return use_order ? (*order)[static_cast<unsigned>(slot)] : slot;
  };

  const int workers = resolve_worker_count(worker_cap, count);

  if (workers == 1)
  {
    for (int slot = 0; slot < count; ++slot)
    {
      const int rc = process_board(0, board_of(slot));
      if (rc != RETURN_NO_FAULT)
      {
        return rc;
      }
    }
    return RETURN_NO_FAULT;
  }

  std::atomic<int> next{0};
  std::atomic<int> first_error{RETURN_NO_FAULT};

  auto worker = [&](const int worker_id) {
    for (;;)
    {
      const int slot = next.fetch_add(1, std::memory_order_relaxed);
      if (slot >= count || first_error.load(std::memory_order_relaxed) != RETURN_NO_FAULT)
      {
        break;
      }
      const int bno = board_of(slot);

      const int rc = process_board(worker_id, bno);
      if (rc != RETURN_NO_FAULT)
      {
        int expected = RETURN_NO_FAULT;
        first_error.compare_exchange_strong(
          expected, rc, std::memory_order_relaxed);
        break;
      }
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(static_cast<unsigned>(workers));
  try
  {
    for (int t = 0; t < workers; ++t)
    {
      threads.emplace_back(worker, t);
    }
  }
  catch (...)
  {
    for (auto & th : threads)
    {
      if (th.joinable())
      {
        th.join();
      }
    }
    throw;
  }

  for (auto & th : threads)
  {
    th.join();
  }

  const int err = first_error.load(std::memory_order_relaxed);
  return err != RETURN_NO_FAULT ? err : RETURN_NO_FAULT;
}
