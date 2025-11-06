#include "system/util/Arena.hpp"
#include <gtest/gtest.h>

using dds::Arena;

TEST(ArenaTest, BumpAndReset) {
  Arena a(1024);
  auto* p1 = a.allocate({64});
  auto* p2 = a.allocate({32});
  EXPECT_NE(p1, nullptr);
  EXPECT_NE(p2, nullptr);
  EXPECT_GE(a.offset(), 96u);
  a.reset();
  EXPECT_EQ(a.offset(), 0u);
  EXPECT_EQ(a.fallback_count(), 0u);
}

TEST(ArenaTest, FallbackAllocations) {
  Arena a(64);
  auto* p1 = a.allocate({128}); // fallback
  EXPECT_NE(p1, nullptr);
  EXPECT_EQ(a.offset(), 0u);
  EXPECT_EQ(a.fallback_count(), 1u);
  a.reset();
  EXPECT_EQ(a.fallback_count(), 0u);
}
