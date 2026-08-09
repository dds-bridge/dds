/// @file ab_stats_test.cpp
/// @brief Unit tests for ABstats counters used by AB_COUNT instrumentation.

#include <gtest/gtest.h>

#include <ab_stats.hpp>

TEST(ABstatsTest, GetPosCountStartsAtZeroAfterReset)
{
  ABstats stats;
  stats.Reset();

  EXPECT_EQ(stats.GetPosCount(AB_MAIN_LOOKUP), 0);
}

TEST(ABstatsTest, IncrPosIncrementsGetPosCount)
{
  ABstats stats;
  stats.Reset();

  stats.IncrPos(AB_MAIN_LOOKUP, /*side=*/true, /*depth=*/20);

  EXPECT_EQ(stats.GetPosCount(AB_MAIN_LOOKUP), 1);
  EXPECT_EQ(stats.GetPosCount(AB_MOVE_LOOP), 0);
}

TEST(ABstatsTest, GetPosCountRejectsOutOfRangePlace)
{
  ABstats stats;
  stats.Reset();

  EXPECT_EQ(stats.GetPosCount(-1), 0);
  EXPECT_EQ(stats.GetPosCount(AB_SIZE), 0);
  EXPECT_EQ(stats.GetPosCount(100), 0);
}
