/// @file utilities_feature_flags_test_with_log.cpp
/// @brief Tests for feature flag detection with DDS_UTILITIES_LOG defined.
///
/// Validates that logging features are properly enabled when
/// DDS_UTILITIES_LOG is defined at compile time.

// Ensure the logging flag is enabled for this translation unit before including the header.
#ifndef DDS_UTILITIES_LOG
#define DDS_UTILITIES_LOG
#endif

#include <gtest/gtest.h>

#include "library/src/system/util/utilities.hpp"

namespace dds {

TEST(UtilitiesFeatureFlagsWithLog, LogEnabledWithDefine) {
  EXPECT_TRUE(Utilities::log_enabled());
}

} // namespace dds
