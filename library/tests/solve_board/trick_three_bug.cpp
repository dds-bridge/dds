#include <gtest/gtest.h>

#include "dds/dds.h"
class TrickThreeBugTests : public ::testing::Test {
     protected:
        TrickThreeBugTests() = default;
};

TEST_F(TrickThreeBugTests, test_declarer_makes_at_least_one_trick) {
    ASSERT_FALSE(true);
}