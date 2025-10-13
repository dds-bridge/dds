#include "library/src/system/util/Utilities.h"
#include <gtest/gtest.h>

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
