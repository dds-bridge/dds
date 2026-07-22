/// @file test_timer_test.cpp
/// @brief Unit tests for TestTimer batch min/max tracking and optional reporting.

#include <cstdint>
#include <ctime>
#include <gtest/gtest.h>
#include <iomanip>
#include <sstream>
#include <string>

#include "TestTimer.hpp"

namespace
{

std::string capture_print_hands(
  const TestTimer& timer,
  const bool show_min,
  const bool show_max)
{
  std::ostringstream out;
  timer.print_hands(out, show_min, show_max);
  return out.str();
}

/// What the old `1000 * delta` path produces when `long` is 32-bit (wasm32).
long wrapped_i32_clock_delta_to_ms(const clock_t delta)
{
  const auto prod =
    static_cast<std::int32_t>(1000 * static_cast<std::int64_t>(delta));
  return static_cast<long>(prod / static_cast<double>(CLOCKS_PER_SEC));
}

}  // namespace

TEST(TestTimer, ClockDeltaToMsAvoids32BitOverflowForMultiSecondBatches)
{
  // ~3.478s of CLOCKS_PER_SEC ticks: integer `1000 * ticks` overflows int32.
  const clock_t ticks = static_cast<clock_t>(3.478 * CLOCKS_PER_SEC);
  const long expected_ms = static_cast<long>(
    (1000.0 * static_cast<double>(ticks)) /
    static_cast<double>(CLOCKS_PER_SEC));
  const long wrapped_ms = wrapped_i32_clock_delta_to_ms(ticks);

  ASSERT_NE(wrapped_ms, expected_ms)
    << "fixture requires a delta that wraps under 32-bit multiply";
  EXPECT_EQ(clock_delta_to_ms(ticks), expected_ms);
  EXPECT_NE(clock_delta_to_ms(ticks), wrapped_ms);
}

TEST(TestTimer, RecordTracksMinAndMaxPerHandAcrossBatches)
{
  TestTimer timer;

  // Batch totals: (100ms / 10 hands), (300 / 10), (200 / 10)
  timer.record(10, 100, 50);
  timer.record(10, 300, 80);
  timer.record(10, 200, 40);

  EXPECT_TRUE(timer.has_batch_times());
  EXPECT_DOUBLE_EQ(timer.user_min_ms(), 10.0);
  EXPECT_DOUBLE_EQ(timer.user_max_ms(), 30.0);
  EXPECT_DOUBLE_EQ(timer.sys_min_ms(), 4.0);
  EXPECT_DOUBLE_EQ(timer.sys_max_ms(), 8.0);
}

TEST(TestTimer, SingleBatchMinEqualsMaxEqualsPerHand)
{
  TestTimer timer;
  timer.record(5, 42, 7);

  EXPECT_DOUBLE_EQ(timer.user_min_ms(), 8.4);
  EXPECT_DOUBLE_EQ(timer.user_max_ms(), 8.4);
  EXPECT_DOUBLE_EQ(timer.sys_min_ms(), 1.4);
  EXPECT_DOUBLE_EQ(timer.sys_max_ms(), 1.4);
}

TEST(TestTimer, UnevenBatchSizesUsePerHandNotBatchTotal)
{
  TestTimer timer;
  // Slower per hand but smaller total: 50ms / 5 = 10.0
  timer.record(5, 50, 10);
  // Faster per hand but larger total: 90ms / 30 = 3.0
  timer.record(30, 90, 30);

  EXPECT_DOUBLE_EQ(timer.user_min_ms(), 3.0);
  EXPECT_DOUBLE_EQ(timer.user_max_ms(), 10.0);
  EXPECT_DOUBLE_EQ(timer.sys_min_ms(), 1.0);
  EXPECT_DOUBLE_EQ(timer.sys_max_ms(), 2.0);
}

TEST(TestTimer, RecordIgnoresNonPositiveHands)
{
  TestTimer timer;
  timer.record(10, 100, 50);
  timer.record(0, 999, 999);
  timer.record(-3, 999, 999);

  EXPECT_TRUE(timer.has_batch_times());
  EXPECT_DOUBLE_EQ(timer.user_min_ms(), 10.0);
  EXPECT_DOUBLE_EQ(timer.user_max_ms(), 10.0);
  EXPECT_DOUBLE_EQ(timer.sys_min_ms(), 5.0);
  EXPECT_DOUBLE_EQ(timer.sys_max_ms(), 5.0);

  const std::string out = capture_print_hands(timer, false, false);
  EXPECT_NE(out.find("Number of hands"), std::string::npos);
  EXPECT_NE(out.find("100"), std::string::npos);
  EXPECT_NE(out.find("10.00"), std::string::npos);
  EXPECT_EQ(out.find("999"), std::string::npos);
}

TEST(TestTimer, ResetClearsBatchExtremes)
{
  TestTimer timer;
  timer.record(1, 10, 2);
  timer.reset();

  EXPECT_FALSE(timer.has_batch_times());
}

TEST(TestTimer, PrintHandsOmitsMinMaxByDefault)
{
  TestTimer timer;
  timer.set_name("Hand stats");
  timer.record(2, 20, 4);

  const std::string out = capture_print_hands(timer, false, false);

  EXPECT_NE(out.find("Avg user time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Min user time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Max user time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Min sys time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Max sys time (ms)"), std::string::npos);
}

TEST(TestTimer, PrintHandsShowsMinWhenRequested)
{
  TestTimer timer;
  timer.record(2, 20, 4);  // 10.0 user / hand, 2.0 sys / hand
  timer.record(2, 10, 8);  // 5.0 user / hand, 4.0 sys / hand

  const std::string out = capture_print_hands(timer, true, false);

  EXPECT_NE(out.find("Min user time (ms)"), std::string::npos);
  EXPECT_NE(out.find("Min sys time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Max user time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Max sys time (ms)"), std::string::npos);
  EXPECT_NE(out.find("5.00"), std::string::npos);
  EXPECT_NE(out.find("2.00"), std::string::npos);
}

TEST(TestTimer, PrintHandsShowsMaxWhenRequested)
{
  TestTimer timer;
  timer.record(2, 20, 4);  // 10.0 / 2.0
  timer.record(2, 10, 8);  // 5.0 / 4.0

  const std::string out = capture_print_hands(timer, false, true);

  EXPECT_NE(out.find("Max user time (ms)"), std::string::npos);
  EXPECT_NE(out.find("Max sys time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Min user time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Min sys time (ms)"), std::string::npos);
  EXPECT_NE(out.find("10.00"), std::string::npos);
  EXPECT_NE(out.find("4.00"), std::string::npos);
}

TEST(TestTimer, PrintHandsRestoresStreamFormatState)
{
  TestTimer timer;
  timer.record(2, 20, 4);

  std::ostringstream out;
  out << std::scientific << std::setprecision(5);
  const auto flags_before = out.flags();
  const auto precision_before = out.precision();

  timer.print_hands(out, true, true);

  EXPECT_EQ(out.flags(), flags_before);
  EXPECT_EQ(out.precision(), precision_before);
}
