/// @file worker_count_test.cpp
/// @brief Unit tests for resolve_worker_count (batch worker-count resolution).
///
/// Validates the shared helper that maps an optional max_threads cap and a work
/// item count onto the number of worker threads to use.

#include <gtest/gtest.h>
#include <algorithm>
#include <thread>

#include <api/dds.h>
#include <system/parallel_boards.hpp>

namespace
{

// Mirror the helper's "auto" computation so the test is host-independent.
int auto_workers(const int count)
{
  const unsigned hw = std::thread::hardware_concurrency();
  const int hw_or_1 = hw > 0 ? static_cast<int>(hw) : 1;
  return std::max(1, std::min(hw_or_1, count));
}

}  // namespace

TEST(ResolveWorkerCount, NonPositiveCapUsesAuto)
{
  const int count = 8;
  EXPECT_EQ(resolve_worker_count(0, count), auto_workers(count));
  EXPECT_EQ(resolve_worker_count(-4, count), auto_workers(count));
}

TEST(ResolveWorkerCount, CapLargerThanCountClampsToCount)
{
  EXPECT_EQ(resolve_worker_count(1000, 5), 5);
}

TEST(ResolveWorkerCount, CapSmallerThanCountAndHardwareIsHonoured)
{
  // A cap of 1 is always <= count and <= hardware_concurrency.
  EXPECT_EQ(resolve_worker_count(1, 8), 1);
}

TEST(ResolveWorkerCount, SingleItemAlwaysOneWorker)
{
  EXPECT_EQ(resolve_worker_count(0, 1), 1);
  EXPECT_EQ(resolve_worker_count(16, 1), 1);
  EXPECT_EQ(resolve_worker_count(1, 1), 1);
}

TEST(ClampWorkersToMemoryBudget, CapsByBudgetPerWorker)
{
  EXPECT_EQ(clamp_workers_to_memory_budget(18, 1400, 95 + 24), 11);
  EXPECT_EQ(clamp_workers_to_memory_budget(4, 1400, 95 + 24), 4);
  EXPECT_EQ(clamp_workers_to_memory_budget(0, 1400, 119), 1);
  EXPECT_EQ(clamp_workers_to_memory_budget(8, 100, 200), 1);
}

TEST(ClampWorkersToMemoryBudget, PlatformCapMatchesWasmBudgetConstants)
{
  // Document the Emscripten budget used by resolve_worker_count so a quiet
  // change to THREADMEM_* cannot silently re-OOM large WASM batches.
  constexpr int kHeapBudgetMB = 1400;
  constexpr int kPerWorkerMB = THREADMEM_LARGE_DEF_MB + 24;
  constexpr int kExpectedCap = kHeapBudgetMB / kPerWorkerMB;
  static_assert(kExpectedCap >= 1);
  EXPECT_EQ(kExpectedCap, 11);
  EXPECT_EQ(
    clamp_workers_to_memory_budget(64, kHeapBudgetMB, kPerWorkerMB),
    kExpectedCap);
}

#if defined(__EMSCRIPTEN__)
TEST(ResolveWorkerCount, WasmMemoryCapLimitsAutoWorkers)
{
  constexpr int kHeapBudgetMB = 1400;
  constexpr int kPerWorkerMB = THREADMEM_LARGE_DEF_MB + 24;
  const int mem_cap = kHeapBudgetMB / kPerWorkerMB;
  EXPECT_EQ(resolve_worker_count(0, 5000), std::min(auto_workers(5000), mem_cap));
  EXPECT_EQ(resolve_worker_count(64, 5000), mem_cap);
}
#else
TEST(ResolveWorkerCount, NativeBuildsDoNotApplyWasmMemoryCap)
{
  // Explicit high caps must remain uncapped on native hosts; the WASM heap
  // budget is Emscripten-only.
  EXPECT_EQ(resolve_worker_count(64, 5000), 64);
  EXPECT_EQ(resolve_worker_count(0, 5000), auto_workers(5000));
}
#endif
