/// @file test_timer_test.cpp
/// @brief Unit tests for TestTimer batch min/max tracking and optional reporting.

#include <cstdint>
#include <ctime>
#include <gtest/gtest.h>
#include <iomanip>
#include <limits>
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

/// Map a 64-bit value into signed int32 via explicit two's-complement wrap.
/// Avoids implementation-defined narrowing of out-of-range values to int32.
std::int32_t wrap_i64_to_i32(const std::int64_t value)
{
  constexpr auto kMod = std::uint64_t{1} << 32;
  const auto bits =
    static_cast<std::uint64_t>(value) % kMod;  // low 32 bits
  if (bits > static_cast<std::uint64_t>(
        std::numeric_limits<std::int32_t>::max())) {
    return static_cast<std::int32_t>(
      static_cast<std::int64_t>(bits) - static_cast<std::int64_t>(kMod));
  }
  return static_cast<std::int32_t>(bits);
}

/// What the old `1000 * delta` path produces when `long` is 32-bit (wasm32).
long wrapped_i32_clock_delta_to_ms(const clock_t delta)
{
  const auto prod =
    wrap_i64_to_i32(1000 * static_cast<std::int64_t>(delta));
  return static_cast<long>(prod / static_cast<double>(CLOCKS_PER_SEC));
}

/// Smallest tick count where `1000 * ticks` no longer fits in int32.
/// Independent of CLOCKS_PER_SEC (1000 on Windows, 1e6 on POSIX/wasm).
clock_t ticks_that_overflow_i32_multiply()
{
  constexpr auto kMaxI32 = std::numeric_limits<std::int32_t>::max();
  return static_cast<clock_t>(
    static_cast<std::int64_t>(kMaxI32) / 1000 + 1);
}

}  // namespace

TEST(TestTimer, WrapI64ToI32UsesDefinedTwosComplement)
{
  constexpr auto kMaxI32 = std::numeric_limits<std::int32_t>::max();
  constexpr auto kMinI32 = std::numeric_limits<std::int32_t>::min();

  EXPECT_EQ(wrap_i64_to_i32(0), 0);
  EXPECT_EQ(wrap_i64_to_i32(kMaxI32), kMaxI32);
  EXPECT_EQ(wrap_i64_to_i32(kMinI32), kMinI32);
  EXPECT_EQ(wrap_i64_to_i32(static_cast<std::int64_t>(kMaxI32) + 1), kMinI32);
  EXPECT_EQ(
    wrap_i64_to_i32(static_cast<std::int64_t>(kMinI32) - 1),
    kMaxI32);
  // 1000 * ticks_that_overflow_i32_multiply()
  EXPECT_EQ(
    wrap_i64_to_i32(
      1000 * static_cast<std::int64_t>(ticks_that_overflow_i32_multiply())),
    -2147483296);
}

TEST(TestTimer, ClockDeltaToMsAvoids32BitOverflowForMultiSecondBatches)
{
  const clock_t ticks = ticks_that_overflow_i32_multiply();
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
