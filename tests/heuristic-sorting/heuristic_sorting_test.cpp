#include "dds.h"
#include "Moves.h"
#include <gmock/gmock.h>

using namespace ::testing;

class HeuristicSortingTest : public Test {
 protected:
  HeuristicSortingTest() = default;

};

TEST_F(HeuristicSortingTest, SortsHighCardFirst) {
  EXPECT_EQ(4, 3); // 3
}

