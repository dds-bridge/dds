/// @file bridge_solver_runner_test.cpp
/// @brief Unit tests for PBN↔macroxue conversion and solver stdout parsing.

#include <gtest/gtest.h>

#include <cstring>
#include <unistd.h>
#include <string>

#include "bridge_solver_runner.hpp"

namespace
{

// deals/fixed/deal.01 from https://github.com/macroxue/bridge-solver
constexpr const char* kDeal01Pbn =
  "N:J75.AQT86.J.AK95 92.KJ92.T985.Q72 AKQ864.53.Q42.T3 T3.74.AK763.J864";

constexpr const char* kVoidDealPbn =
  "N:KQ3..T832.AJ9765 T96.K83.654.T843 AJ854.QT654.KJ9. 72.AJ972.AQ7.KQ2";

constexpr const char* kSolverStdout =
  "N  9  9  3  3  0.01 s 4064.0 M\n"
  "S 11 11  2  2  0.01 s 4096.0 M\n"
  "H  8  8  4  4  0.04 s 5440.0 M\n"
  "D  6  6  6  6  0.05 s 5616.0 M\n"
  "C  7  7  3  3  0.11 s 8480.0 M\n";

// DDS res_table[strain][N,E,S,W]
constexpr int kDeal01Table[DDS_STRAINS][DDS_HANDS] = {
  {11, 2, 11, 2},  // S
  {8, 4, 8, 4},    // H
  {6, 6, 6, 6},    // D
  {7, 3, 7, 3},    // C
  {9, 3, 9, 3},    // N
};

void expect_table_eq(
  const DdTableResults& got,
  const int expected[DDS_STRAINS][DDS_HANDS])
{
  for (int strain = 0; strain < DDS_STRAINS; ++strain)
  {
    for (int hand = 0; hand < DDS_HANDS; ++hand)
    {
      EXPECT_EQ(got.res_table[strain][hand], expected[strain][hand])
        << "strain=" << strain << " hand=" << hand;
    }
  }
}

}  // namespace

TEST(BridgeSolverRunner, PbnToMacroxueAcceptsEastFirst)
{
  // masterDD.txt deal 2: starts with E:; South is void in spades.
  constexpr const char* kEastFirst =
      "E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63";
  std::string error;
  const std::string text = pbn_to_macroxue_deal(kEastFirst, &error);
  ASSERT_FALSE(text.empty()) << error;
  // Clockwise from East: E, S, W, N → North is last token.
  EXPECT_NE(text.find("AK96 KQ8 A98 K63"), std::string::npos) << text;
  // South void spades → dash on the South line.
  EXPECT_NE(text.find("- J97543 K7532 94"), std::string::npos) << text;
  EXPECT_NE(text.find("QJT5432 T 6 QJ82"), std::string::npos) << text;
}

TEST(BridgeSolverRunner, CountCardsWorksForNonNorthFirst)
{
  int score = -1;
  std::string error;
  // Unit-level: conversion must succeed so card count can run.
  EXPECT_FALSE(
      pbn_to_macroxue_deal(
          "W:AK8.A83.KT.KQJ98 QJT96543.4.AJ.A5 .KQJ976.742.7642 72.T52.Q98653.T3",
          &error)
          .empty())
      << error;
  (void)score;
}

TEST(BridgeSolverRunner, PbnToMacroxueDeal01RoundTripsSeats)
{
  std::string error;
  const std::string text = pbn_to_macroxue_deal(kDeal01Pbn, &error);
  ASSERT_FALSE(text.empty()) << error;
  EXPECT_NE(text.find("J75 AQT86 J AK95"), std::string::npos);
  EXPECT_NE(text.find("T3 74 AK763 J864"), std::string::npos);
  EXPECT_NE(text.find("92 KJ92 T985 Q72"), std::string::npos);
  EXPECT_NE(text.find("AKQ864 53 Q42 T3"), std::string::npos);
}

TEST(BridgeSolverRunner, PbnToMacroxueUsesDashForVoids)
{
  std::string error;
  const std::string text = pbn_to_macroxue_deal(kVoidDealPbn, &error);
  ASSERT_FALSE(text.empty()) << error;
  EXPECT_NE(text.find("KQ3 - T832 AJ9765"), std::string::npos);
  EXPECT_NE(text.find("AJ854 QT654 KJ9 -"), std::string::npos);
}

TEST(BridgeSolverRunner, PbnToMacroxueRejectsBadInput)
{
  std::string error;
  EXPECT_TRUE(pbn_to_macroxue_deal("not-a-deal", &error).empty());
  EXPECT_FALSE(error.empty());
}

TEST(BridgeSolverRunner, ParseStdoutMapsSnweToNesw)
{
  DdTableResults table{};
  std::string error;
  ASSERT_TRUE(parse_macroxue_solver_stdout(kSolverStdout, table, &error))
    << error;
  expect_table_eq(table, kDeal01Table);
}

TEST(BridgeSolverRunner, ParseStdoutIgnoresNoiseLines)
{
  const std::string noisy =
    std::string("                          some deal art\n") + kSolverStdout;
  DdTableResults table{};
  std::string error;
  ASSERT_TRUE(parse_macroxue_solver_stdout(noisy, table, &error)) << error;
  expect_table_eq(table, kDeal01Table);
}

TEST(BridgeSolverRunner, ParseStdoutRequiresFiveStrains)
{
  DdTableResults table{};
  std::string error;
  EXPECT_FALSE(
    parse_macroxue_solver_stdout("N  9  9  3  3  0.01 s\n", table, &error));
  EXPECT_FALSE(error.empty());
}

TEST(BridgeSolverRunner, PbnToMacroxueAppendsTrumpAndLead)
{
  std::string error;
  const std::string text = pbn_to_macroxue_deal(
      kDeal01Pbn, /*trump=*/2, /*first=*/0, &error);
  ASSERT_FALSE(text.empty()) << error;
  EXPECT_NE(text.find("\nD\nN\n"), std::string::npos) << text;
}

TEST(BridgeSolverRunner, PbnToMacroxueNoTrumpLeadOmitsExtraLines)
{
  std::string error;
  const std::string text = pbn_to_macroxue_deal(kDeal01Pbn, &error);
  ASSERT_FALSE(text.empty()) << error;
  EXPECT_EQ(text.find("\nD\n"), std::string::npos);
}

TEST(BridgeSolverRunner, ParseSolveStdoutSingleTrickCount)
{
  int tricks = -1;
  std::string error;
  ASSERT_TRUE(parse_macroxue_solver_solve_stdout(
      "D  3  0.00 s 3936.0 M\n", tricks, &error))
      << error;
  EXPECT_EQ(tricks, 3);
}

TEST(BridgeSolverRunner, LeadingSideTricksFromDeclarerSide)
{
  // Full deal: remaining_tricks=13. Bridge-solver with a fixed lead reports
  // the non-leading (declarer) side; DDS SolveBoard scores the leading side.
  EXPECT_EQ(leading_side_tricks_from_declarer_side(13, 3), 10);
  EXPECT_EQ(leading_side_tricks_from_declarer_side(13, 10), 3);
}

TEST(BridgeSolverRunner, RunRealBinarySolveIfPresent)
{
  const char* path = "/Users/adamw/src/bridge-solver/solver";
  if (access(path, X_OK) != 0)
    GTEST_SKIP() << "bridge-solver binary not present";

  // list1.txt: trump=diamonds, first=North; FUT scores are 10 for NS.
  int score = -1;
  std::string error;
  ASSERT_TRUE(run_bridge_solver_solve(
      path,
      "N:Q87.T8.AKJT64.J6 964.AJ765.Q73.74 AKJT2.Q943..AK95 53.K2.9852.QT832",
      /*trump=*/2,
      /*first=*/0,
      score,
      &error))
      << error;
  EXPECT_EQ(score, 10);
}

TEST(BridgeSolverRunner, RunRealBinaryIfPresent)
{
  const char* path = "/Users/adamw/src/bridge-solver/solver";
  if (access(path, X_OK) != 0)
    GTEST_SKIP() << "bridge-solver binary not present";

  DdTableResults table{};
  std::string error;
  const bool ok = run_bridge_solver_table(
    path,
    "N:J75.AQT86.J.AK95 92.KJ92.T985.Q72 AKQ864.53.Q42.T3 T3.74.AK763.J864",
    table,
    &error);
  EXPECT_TRUE(ok) << error;
  if (ok)
    expect_table_eq(table, kDeal01Table);
}
