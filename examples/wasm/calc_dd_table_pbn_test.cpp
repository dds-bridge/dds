/*
   Regression tests for CalcDDtablePBN (WASM example target).

   Copyright 2020-2026 Adam Wildavsky
   Use of this source code is governed by the MIT license.
*/

#include <cstring>

#include <gtest/gtest.h>

#include <api/dll.h>
#include "hands.hpp"

TEST(CalcDdTablePbnWasmTest, MatchesReferenceTables) {

  for (int handno = 0; handno < 3; ++handno) {
    DdTableDealPBN deal{};
    std::strcpy(deal.cards, pbn_hands_[handno]);

    DdTableResults table{};
    ASSERT_EQ(CalcDDtablePBN(deal, &table), RETURN_NO_FAULT) << "hand " << handno;
    EXPECT_TRUE(compare_table(&table, handno)) << "hand " << handno;
  }
}
