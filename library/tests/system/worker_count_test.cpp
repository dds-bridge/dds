/// @file worker_count_test.cpp
/// @brief Unit tests for resolve_worker_count (batch worker-count resolution).
///
/// Validates the shared helper that maps an optional max_threads cap and a work
/// item count onto the number of worker threads to use.

#include <gtest/gtest.h>
#include <algorithm>
#include <thread>

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
