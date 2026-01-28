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
