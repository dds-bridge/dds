/// @file utilities_log_contains_test.cpp
/// @brief Tests for log buffer content validation.
///
/// Validates that log entries are properly recorded and can be searched
/// within the utilities log buffer.

#include <gtest/gtest.h>

#include "library/src/system/util/utilities.hpp"

namespace dds {

TEST(UtilitiesLogContains, PrefixMatchesWork) {
  Utilities u;
  u.log_clear();
  u.log_append("tt:create|K|1024|4096");
  u.log_append("ctx:reset_for_solve");
  EXPECT_EQ(u.log_size(), 2u);
  EXPECT_TRUE(u.log_contains("tt:create"));
  EXPECT_TRUE(u.log_contains("ctx:reset"));
  EXPECT_FALSE(u.log_contains("not:there"));
}

} // namespace dds
