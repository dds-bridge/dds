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
extern "C" int dds_mvp_solve_leads(
    const char* pbn, int trump, int first, int* out_leads);

namespace {

constexpr int kExpectedHand0[20] = {
    5, 8, 5, 8, 6, 6, 6, 6, 5, 7, 5, 7, 7, 5, 7, 5, 6, 6, 6, 6,
};

constexpr char kPbnHand0[] =
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

constexpr int kExpectedHand1[20] = {
    3, 10, 3, 10, 9, 4, 9, 4, 8, 4, 8, 4, 3, 9, 3, 9, 4, 8, 4, 8,
};

constexpr char kPbnHand1[] =
    "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93";

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

TEST(DdsMvpWasmTest, FillsFlatStrainHandTableAcrossReuse) {
  int out0[20]{};
  ASSERT_EQ(dds_mvp_calc_table(kPbnHand0, out0), RETURN_NO_FAULT);
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(out0[i], kExpectedHand0[i]) << "hand0 index " << i;
  }

  int out1[20]{};
  ASSERT_EQ(dds_mvp_calc_table(kPbnHand1, out1), RETURN_NO_FAULT);
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(out1[i], kExpectedHand1[i]) << "hand1 index " << i;
  }

  // Solve hand0 again through the same reused context to confirm the
  // intervening different-deal solve didn't leave stale state behind.
  int out0_again[20]{};
  ASSERT_EQ(dds_mvp_calc_table(kPbnHand0, out0_again), RETURN_NO_FAULT);
  for (int i = 0; i < 20; ++i) {
    EXPECT_EQ(out0_again[i], kExpectedHand0[i]) << "hand0 repeat index " << i;
  }
}

// Part-score deal from the MVP page (fillFormWithPartScoreTestData).
constexpr char kPbnPartScore[] =
    "N:AQ85.AK976.5.J87 JT.QJ5432.Q9.KQ9 972..JT863.A6432 K643.T8.AK742.T5";

TEST(DdsMvpWasmTest, SolveLeadsRejectsNullPointers) {
  int out[40]{};
  EXPECT_EQ(dds_mvp_solve_leads(nullptr, 4, 3, out), RETURN_UNKNOWN_FAULT);
  EXPECT_EQ(
      dds_mvp_solve_leads(kPbnPartScore, 4, 3, nullptr), RETURN_UNKNOWN_FAULT);
}

TEST(DdsMvpWasmTest, SolveLeadsReturnsOpeningLeadTricksForEachCard) {
  // South declares NT → West leads. CalcDDtable says South takes 6 in NT, so
  // the best EW opening lead scores 7 tricks for the side on lead.
  int out[40]{};
  ASSERT_EQ(dds_mvp_solve_leads(kPbnPartScore, /*trump=*/4, /*first=*/3, out),
            RETURN_NO_FAULT);

  const int n = out[0];
  ASSERT_EQ(n, 13);

  int max_score = -1;
  bool saw_sk = false;
  for (int i = 0; i < n; ++i) {
    const int suit = out[1 + 3 * i];
    const int rank = out[1 + 3 * i + 1];
    const int score = out[1 + 3 * i + 2];
    EXPECT_GE(suit, 0);
    EXPECT_LE(suit, 3);
    EXPECT_GE(rank, 2);
    EXPECT_LE(rank, 14);
    EXPECT_GE(score, 0);
    EXPECT_LE(score, 13);
    if (score > max_score) {
      max_score = score;
    }
    // West holds ♠K.
    if (suit == 0 && rank == 13) {
      saw_sk = true;
    }
  }
  EXPECT_TRUE(saw_sk);
  EXPECT_EQ(max_score, 7);
}
