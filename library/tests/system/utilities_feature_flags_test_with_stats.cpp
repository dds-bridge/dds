// Ensure the stats flag is enabled for this translation unit before including the header.
#ifndef DDS_UTILITIES_STATS
#define DDS_UTILITIES_STATS
#endif

/// @file utilities_feature_flags_test_with_stats.cpp
/// @brief Tests for feature flag detection with DDS_UTILITIES_STATS defined.
///
/// Validates that statistics features are properly enabled when
/// DDS_UTILITIES_STATS is defined at compile time.

#include <gtest/gtest.h>

#include "library/src/system/util/utilities.hpp"

namespace dds {

TEST(UtilitiesFeatureFlagsWithStats, StatsEnabledWithDefine) {
  EXPECT_TRUE(Utilities::stats_enabled());
}

} // namespace dds
