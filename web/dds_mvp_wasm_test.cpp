/*
   Unit tests for the browser MVP WASM bridge (dds_mvp_calc_table).

   Copyright 2020-2026 Adam Wildavsky
   Use of this source code is governed by the MIT license.
*/

#include <cstring>
#include <string>

#include <gtest/gtest.h>

#include <api/dll.h>

extern "C" int dds_mvp_calc_table(const char* pbn, int* out_table);

namespace {

constexpr int kExpectedHand0[20] = {
    5, 8, 5, 8, 6, 6, 6, 6, 5, 7, 5, 7, 7, 5, 7, 5, 6, 6, 6, 6,
};

constexpr char kPbnHand0[] =
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

}  // namespace

TEST(DdsMvpWasmTest, RejectsNullPointers) {
  int out[20]{};
  EXPECT_EQ(dds_mvp_calc_table(nullptr, out), RETURN_UNKNOWN_FAULT);
  EXPECT_EQ(dds_mvp_calc_table(kPbnHand0, nullptr), RETURN_UNKNOWN_FAULT);
}

TEST(DdsMvpWasmTest, RejectsPbnTooLong) {
  int out[20]{};
  const std::string too_long(80, 'A');
  EXPECT_EQ(dds_mvp_calc_table(too_long.c_str(), out), RETURN_PBN_FAULT);
}

TEST(DdsMvpWasmTest, RejectsInvalidPbn) {
  int out[20]{};
  const int res = dds_mvp_calc_table("not-a-valid-pbn", out);
  EXPECT_NE(res, RETURN_NO_FAULT);
  EXPECT_LT(res, RETURN_NO_FAULT);
}

TEST(DdsMvpWasmTest, FillsFlatStrainHandTable) {
  int out[20]{};
  ASSERT_EQ(dds_mvp_calc_table(kPbnHand0, out), RETURN_NO_FAULT);
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(out[i], kExpectedHand0[i]) << "index " << i;
  }
}
