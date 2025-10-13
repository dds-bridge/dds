#include "library/src/system/util/Utilities.h"
#include <gtest/gtest.h>

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
