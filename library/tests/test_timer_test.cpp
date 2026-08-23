/// @file test_timer_test.cpp
/// @brief Unit tests for TestTimer accumulation, printing, and clock helpers.

#include <cstdint>
#include <ctime>
#include <gtest/gtest.h>
#include <iomanip>
#include <ios>
#include <limits>
#include <regex>
#include <sstream>
#include <string>

#include "TestTimer.hpp"

namespace
{

std::string capture_print_hands(const TestTimer& timer)
{
  std::ostringstream out;
  timer.print_hands(out);
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

TEST(TestTimer, RecordAccumulatesHandsAndTimes)
{
  TestTimer timer;
  timer.record(10, 100, 50);
  timer.record(5, 20, 10);

  const std::string out = capture_print_hands(timer);
  // Anchor values to their labels so digits elsewhere in the report cannot
  // satisfy the assertions (e.g. "15" matching inside "150").
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(Number of hands\s+15\s*(?:\n|$))")));  // 10 + 5
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(User time \(ms\)\s+120\s*(?:\n|$))")));  // 100 + 20
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(Avg user time \(ms\)\s+8\.00\s*(?:\n|$))")));  // 120/15
  EXPECT_EQ(out.find("Min user time (ms)"), std::string::npos);
  EXPECT_EQ(out.find("Max user time (ms)"), std::string::npos);
}

TEST(TestTimer, RecordIgnoresNonPositiveHands)
{
  TestTimer timer;
  timer.record(10, 100, 50);
  timer.record(0, 999, 999);
  timer.record(-3, 999, 999);

  const std::string out = capture_print_hands(timer);
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(Number of hands\s+10\s*(?:\n|$))")));
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(User time \(ms\)\s+100\s*(?:\n|$))")));
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(Avg user time \(ms\)\s+10\.00\s*(?:\n|$))")));
  EXPECT_EQ(out.find("999"), std::string::npos);
}

TEST(TestTimer, ResetClearsAccumulatedStats)
{
  TestTimer timer;
  timer.record(1, 10, 2);
  timer.reset();

  const std::string out = capture_print_hands(timer);
  // Require the hands count field itself to be 0 (not a substring match like
  // "10", which also contains '0').
  EXPECT_TRUE(std::regex_search(
    out, std::regex(R"(Number of hands\s+0\s*(?:\n|$))")));
  EXPECT_EQ(out.find("User time (ms)"), std::string::npos);
}

TEST(TestTimer, PrintHandsShowsSysNaWhenClockUnavailable)
{
  // wasm32+pthread: clock() always returns -1 (process CPU clock is epoch-based
  // and does not fit in 32-bit clock_t). That must not be printed as "zero".
  TestTimer timer;
  timer.mark_sys_time_unavailable();
  timer.record(10, 100, 0);

  const std::string out = capture_print_hands(timer);

  EXPECT_NE(out.find("Sys time (ms)"), std::string::npos);
  EXPECT_NE(out.find("n/a"), std::string::npos);
  EXPECT_EQ(out.find("zero"), std::string::npos);
}

TEST(TestTimer, PrintHandsRestoresStreamFormatState)
{
  TestTimer timer;
  timer.record(2, 20, 4);

  std::ostringstream out;
  out << std::scientific << std::setprecision(5);
  const auto flags_before = out.flags();
  const auto precision_before = out.precision();

  timer.print_hands(out);

  EXPECT_EQ(out.flags(), flags_before);
  EXPECT_EQ(out.precision(), precision_before);
}
