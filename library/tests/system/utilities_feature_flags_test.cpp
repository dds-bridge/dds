/// @file utilities_feature_flags_test.cpp
/// @brief Tests for feature flag detection in Utilities.
///
/// Validates that logging and statistics features are properly disabled
/// by default without compile-time defines.

#include <gtest/gtest.h>

#include "library/src/system/util/utilities.hpp"

namespace dds {

TEST(UtilitiesFeatureFlags, LogDisabledByDefault) {
  // Default build should not define DDS_UTILITIES_LOG.
  EXPECT_FALSE(Utilities::log_enabled());
}

TEST(UtilitiesFeatureFlags, StatsDisabledByDefault) {
  // Default build should not define DDS_UTILITIES_STATS.
  EXPECT_FALSE(Utilities::stats_enabled());
}

} // namespace dds
