/// @file scheduler_board_time_test.cpp
/// @brief Unit tests for saturating per-board microsecond timings into int storage.

#include <gtest/gtest.h>

#include <limits>

#include <system/scheduler.hpp>

TEST(SaturateBoardTimeUs, ClampsAboveIntMax)
{
  // Arrange
  const long long too_large =
    static_cast<long long>(std::numeric_limits<int>::max()) + 1;

  // Act / Assert
  EXPECT_EQ(saturate_board_time_us(too_large), std::numeric_limits<int>::max());
}

TEST(SaturateBoardTimeUs, ClampsNegativeToZero)
{
  EXPECT_EQ(saturate_board_time_us(-1), 0);
  EXPECT_EQ(saturate_board_time_us(-5), 0);
}

TEST(SaturateBoardTimeUs, PreservesInRangeValues)
{
  EXPECT_EQ(saturate_board_time_us(0), 0);
  EXPECT_EQ(saturate_board_time_us(12345), 12345);
  EXPECT_EQ(
    saturate_board_time_us(std::numeric_limits<int>::max()),
    std::numeric_limits<int>::max());
}
