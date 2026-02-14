/// @file utilities_log_snapshot_test.cpp
/// @brief Tests for log buffer snapshot functionality.
///
/// Validates that snapshots of log buffers are independent copies
/// and remain unchanged when the original buffer is modified.

#include <gtest/gtest.h>

#include "library/src/system/util/utilities.hpp"

namespace dds {

TEST(UtilitiesLogSnapshot, CopiesAreIndependent) {
  Utilities u;
  u.log_clear();
  u.log_append("tt:create|K|1024|4096");
  u.log_append("ctx:reset_for_solve");
  auto snap = u.log_snapshot();
  ASSERT_EQ(snap.size(), 2u);
  EXPECT_EQ(snap[0], "tt:create|K|1024|4096");
  EXPECT_EQ(snap[1], "ctx:reset_for_solve");
  // Mutate original; snapshot should remain unchanged
  u.log_clear();
  EXPECT_EQ(u.log_size(), 0u);
  ASSERT_EQ(snap.size(), 2u);
}

} // namespace dds
