/// @file compare_test.cpp
/// @brief Unit tests for dtest result comparison helpers.

#include <gtest/gtest.h>

#include "compare.hpp"

namespace
{

auto filled_table(const int value) -> DdTableResults
{
  DdTableResults table{};
  for (int strain = 0; strain < DDS_STRAINS; ++strain)
  {
    for (int hand = 0; hand < DDS_HANDS; ++hand)
      table.res_table[strain][hand] = value;
  }
  return table;
}

}  // namespace

TEST(CompareTable, EqualTablesMatchIncludingNt)
{
  const DdTableResults a = filled_table(7);
  const DdTableResults b = filled_table(7);
  EXPECT_TRUE(compare_TABLE(a, b));
}

TEST(CompareTable, DetectsNtOnlyMismatch)
{
  DdTableResults a = filled_table(7);
  DdTableResults b = filled_table(7);
  b.res_table[4][0] = 8;  // NT / North
  EXPECT_FALSE(compare_TABLE(a, b));
}

TEST(CompareTable, DetectsSuitMismatch)
{
  DdTableResults a = filled_table(7);
  DdTableResults b = filled_table(7);
  b.res_table[0][1] = 3;  // Spades / East
  EXPECT_FALSE(compare_TABLE(a, b));
}
