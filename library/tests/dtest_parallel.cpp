/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "dtest_parallel.hpp"

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

#include <api/dll.h>


int dtest_effective_threads(const int requested, const int workload)
{
  if (workload <= 1)
    return 1;

  const unsigned hw = std::thread::hardware_concurrency();
  const int auto_count = hw > 0 ? static_cast<int>(hw) : 1;

  int n = requested > 0 ? requested : auto_count;
  n = std::max(1, std::min(n, workload));
  return n;
}


int dtest_run_parallel(
  const int count,
  const int requested_threads,
  const std::function<int(int)> & body)
{
  if (count <= 0)
    return RETURN_NO_FAULT;

  const int nthreads = dtest_effective_threads(requested_threads, count);
  if (nthreads <= 1)
  {
    for (int i = 0; i < count; ++i)
    {
      const int rc = body(i);
      if (rc != RETURN_NO_FAULT)
        return rc;
    }
    return RETURN_NO_FAULT;
  }

  std::atomic<int> next{0};
  std::atomic<int> first_error{0};

  auto worker = [&] {
    for (;;)
    {
      const int i = next.fetch_add(1, std::memory_order_relaxed);
      if (i >= count || first_error.load(std::memory_order_relaxed) != 0)
        break;

      const int rc = body(i);
      if (rc != RETURN_NO_FAULT)
      {
        int expected = 0;
        first_error.compare_exchange_strong(
          expected, rc, std::memory_order_relaxed);
        break;
      }
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(static_cast<unsigned>(nthreads));
  try
  {
    for (int t = 0; t < nthreads; ++t)
      threads.emplace_back(worker);
  }
  catch (...)
  {
    for (auto & th : threads)
      if (th.joinable())
        th.join();
    throw;
  }

  for (auto & th : threads)
    th.join();

  const int err = first_error.load(std::memory_order_relaxed);
  return err != 0 ? err : RETURN_NO_FAULT;
}
