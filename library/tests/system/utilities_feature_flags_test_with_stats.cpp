// Ensure the stats flag is enabled for this translation unit before including the header.
#ifndef DDS_UTILITIES_STATS
#define DDS_UTILITIES_STATS
#endif

#include "library/src/system/util/Utilities.hpp"
#include <gtest/gtest.h>

namespace dds {

TEST(UtilitiesFeatureFlagsWithStats, StatsEnabledWithDefine) {
  EXPECT_TRUE(Utilities::stats_enabled());
}

} // namespace dds
