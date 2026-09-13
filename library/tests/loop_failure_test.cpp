/// @file loop_failure_test.cpp
/// @brief Failure-path coverage for dtest solve/calc/play/par/dealerpar loops.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <ios>
#include <regex>
#include <sstream>
#include <string>

#include <api/dll.h>

#include "cst.hpp"
#include "loop.hpp"
#include "parse.hpp"
#include "TestTimer.hpp"

TestTimer timer;
OptionsType options;

namespace
{

constexpr const char* kDealBody = R"(PBN 1 0 2 0 "N:Q87.T8.AKJT64.J6 964.AJ765.Q73.74 AKJT2.Q943..AK95 53.K2.9852.QT832" 
FUT 10 0 3 3 2 1 2 1 2 2 0 12 6 11 14 8 4 10 6 11 8 0 0 0 8192 0 0 0 0 1024 128 10 10 10 10 10 10 10 10 10 9 
TABLE 11 2 11 1 9 4 9 4 10 3 10 3 8 5 8 4 10 3 10 3 
PAR "NS 450" "EW -450" "NS:NS 45S" "EW:NS 45S" 
PAR2 "450" "4S-NS+1" 
PLAY 52 "SQS4S2S3DAD3H4D2DKD7H3D5D6DQC5D9H7H9HKH8H2HTHAHQC4CAC3C6SAS5S8S6CKC2CJC7C9CQD4HJDJH6SKD8S7S9SJCTSTC8DTH5" 
TRACE 49 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 3 
)";

struct HandLists
{
  int number = 0;
  bool gib_mode = false;
  int* dealer_list = nullptr;
  int* vul_list = nullptr;
  DealPBN* deal_list = nullptr;
  FutureTricks* fut_list = nullptr;
  DdTableResults* table_list = nullptr;
  ParResults* par_list = nullptr;
  ParResultsDealer* dealerpar_list = nullptr;
  PlayTracePBN* play_list = nullptr;
  SolvedPlay* trace_list = nullptr;
  std::string path;

  ~HandLists()
  {
    free(dealer_list);
    free(vul_list);
    free(deal_list);
    free(fut_list);
    free(table_list);
    free(par_list);
    free(dealerpar_list);
    free(play_list);
    free(trace_list);
    if (!path.empty())
      std::remove(path.c_str());
  }
};

auto write_hands(const std::string& name, const std::string& body)
  -> std::string
{
  const std::string path = std::string(::testing::TempDir()) + name;
  std::ofstream out(path, std::ios::out | std::ios::trunc);
  out << body;
  return path;
}

auto load_hands(const std::string& name, const std::string& body) -> HandLists
{
  HandLists hands;
  hands.path = write_hands(name, body);
  EXPECT_TRUE(read_file(
      hands.path,
      hands.number,
      hands.gib_mode,
      &hands.dealer_list,
      &hands.vul_list,
      &hands.deal_list,
      &hands.fut_list,
      &hands.table_list,
      &hands.par_list,
      &hands.dealerpar_list,
      &hands.play_list,
      &hands.trace_list));
  return hands;
}

auto two_deal_body(const std::string& first, const std::string& second)
  -> std::string
{
  return std::string("NUMBER 2 \n") + first + second;
}

auto corrupt_table(DdTableResults* table) -> void
{
  for (int d = 0; d < DDS_STRAINS; d++)
    for (int h = 0; h < DDS_HANDS; h++)
      table->res_table[d][h] = 2000000000;
}

auto capture_print_hands(const TestTimer& test_timer) -> std::string
{
  std::ostringstream out;
  test_timer.print_hands(out);
  return out.str();
}

}  // namespace

class LoopFailureTest : public ::testing::Test
{
 protected:
  static void SetUpTestSuite()
  {
    InitializeStaticMemory();
  }

  void SetUp() override
  {
    options = OptionsType{};
    options.num_threads_ = 1;
    timer.reset();
  }
};

TEST_F(LoopFailureTest, SolveStopsOnFirstExpectedMismatch)
{
  auto wrong = std::string(kDealBody);
  const auto pos = wrong.find("10 10 10 10 10 10 10 10 10 9");
  ASSERT_NE(pos, std::string::npos);
  wrong.replace(pos, std::strlen("10 10 10 10 10 10 10 10 10 9"),
      "10 10 10 10 10 10 10 10 10 0");

  auto hands = load_hands(
      "loop_fail_solve.txt", two_deal_body(wrong, wrong));
  ASSERT_EQ(hands.number, 2);

  BoardsPBN bop{};
  SolvedBoards solved{};
  testing::internal::CaptureStdout();
  const bool ok =
      loop_solve(&bop, &solved, hands.deal_list, hands.fut_list, 2, 1);
  const std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_NE(out.find("loop_solve: i 0, j 0: Difference"), std::string::npos);
  EXPECT_EQ(out.find("loop_solve: i 1"), std::string::npos);
  // Progress was printed for the failed batch then cleared before the report.
  EXPECT_NE(out.find("\033[2K"), std::string::npos);
  const auto diff_at = out.find("Difference");
  ASSERT_NE(diff_at, std::string::npos);
  EXPECT_NE(out.rfind("\033[2K\r", diff_at), std::string::npos);
}

TEST_F(LoopFailureTest, CalcStopsOnFirstExpectedMismatchAndClearsProgress)
{
  auto wrong = std::string(kDealBody);
  const auto pos = wrong.find("TABLE 11 ");
  ASSERT_NE(pos, std::string::npos);
  wrong.replace(pos, std::strlen("TABLE 11 "), "TABLE 0 ");

  auto hands = load_hands(
      "loop_fail_calc.txt", two_deal_body(wrong, wrong));
  ASSERT_EQ(hands.number, 2);

  testing::internal::CaptureStdout();
  const bool ok = loop_calc(hands.deal_list, hands.table_list, 2);
  const std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_NE(out.find("loop_calc: j 0: Difference"), std::string::npos);
  EXPECT_EQ(out.find("loop_calc: j 1:"), std::string::npos);
  const auto diff_at = out.find("Difference");
  ASSERT_NE(diff_at, std::string::npos);
  EXPECT_NE(out.rfind("\033[2K\r", diff_at), std::string::npos);
}

TEST_F(LoopFailureTest, PlayStopsOnFirstExpectedMismatch)
{
  auto wrong = std::string(kDealBody);
  const auto pos = wrong.find("TRACE 49 3 ");
  ASSERT_NE(pos, std::string::npos);
  wrong.replace(pos, std::strlen("TRACE 49 3 "), "TRACE 49 0 ");

  auto hands = load_hands(
      "loop_fail_play.txt", two_deal_body(wrong, wrong));
  ASSERT_EQ(hands.number, 2);

  BoardsPBN bop{};
  PlayTracesPBN plays{};
  SolvedPlays solved{};
  testing::internal::CaptureStdout();
  const bool ok = loop_play(
      &bop,
      &plays,
      &solved,
      hands.deal_list,
      hands.play_list,
      hands.trace_list,
      2,
      1);
  const std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_NE(out.find("loop_play: i 0, j 0: Difference"), std::string::npos);
  EXPECT_EQ(out.find("loop_play: i 1"), std::string::npos);
  const auto diff_at = out.find("Difference");
  ASSERT_NE(diff_at, std::string::npos);
  EXPECT_NE(out.rfind("\033[2K\r", diff_at), std::string::npos);
}

TEST_F(LoopFailureTest, DealerParStopsOnFirstExpectedMismatch)
{
  auto wrong = std::string(kDealBody);
  const auto pos = wrong.find("PAR2 \"450\"");
  ASSERT_NE(pos, std::string::npos);
  wrong.replace(pos, std::strlen("PAR2 \"450\""), "PAR2 \"999\"");

  auto hands = load_hands(
      "loop_fail_dealerpar.txt", two_deal_body(wrong, wrong));
  ASSERT_EQ(hands.number, 2);

  testing::internal::CaptureStdout();
  const bool ok = loop_dealerpar(
      hands.dealer_list, hands.vul_list, hands.table_list,
      hands.dealerpar_list, 2, 1);
  const std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_NE(out.find("loop_dealerpar i 0: Difference"), std::string::npos);
  EXPECT_EQ(out.find("loop_dealerpar i 1:"), std::string::npos);
}

TEST_F(LoopFailureTest, ParClosesTimerBeforeReturningOnMismatch)
{
  auto wrong = std::string(kDealBody);
  const auto pos = wrong.find("PAR \"NS 450\"");
  ASSERT_NE(pos, std::string::npos);
  wrong.replace(pos, std::strlen("PAR \"NS 450\""), "PAR \"NS -999\"");

  auto hands = load_hands(
      "loop_fail_par_timer.txt",
      std::string("NUMBER 1 \n") + wrong);
  ASSERT_EQ(hands.number, 1);

  testing::internal::CaptureStdout();
  EXPECT_FALSE(loop_par(
      hands.vul_list, hands.table_list, hands.par_list, 1, 1));
  testing::internal::GetCapturedStdout();

  const std::string summary = capture_print_hands(timer);
  EXPECT_TRUE(std::regex_search(
      summary, std::regex(R"(Number of hands\s+1\s*(?:\n|$))")));
}

TEST_F(LoopFailureTest, DealerParClosesTimerBeforeReturningOnMismatch)
{
  auto wrong = std::string(kDealBody);
  const auto pos = wrong.find("PAR2 \"450\"");
  ASSERT_NE(pos, std::string::npos);
  wrong.replace(pos, std::strlen("PAR2 \"450\""), "PAR2 \"999\"");

  auto hands = load_hands(
      "loop_fail_dealerpar_timer.txt",
      std::string("NUMBER 1 \n") + wrong);
  ASSERT_EQ(hands.number, 1);

  testing::internal::CaptureStdout();
  EXPECT_FALSE(loop_dealerpar(
      hands.dealer_list, hands.vul_list, hands.table_list,
      hands.dealerpar_list, 1, 1));
  testing::internal::GetCapturedStdout();

  const std::string summary = capture_print_hands(timer);
  EXPECT_TRUE(std::regex_search(
      summary, std::regex(R"(Number of hands\s+1\s*(?:\n|$))")));
}

TEST_F(LoopFailureTest, ParStopsOnApiFaultWithoutProcessingLaterDeal)
{
  auto hands = load_hands(
      "loop_fail_par_api.txt", two_deal_body(kDealBody, kDealBody));
  ASSERT_EQ(hands.number, 2);
  corrupt_table(&hands.table_list[0]);

  testing::internal::CaptureStdout();
  const bool ok = loop_par(
      hands.vul_list, hands.table_list, hands.par_list, 2, 1);
  const std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_NE(out.find("loop_par:"), std::string::npos);
  EXPECT_NE(out.find("loop_par: i 0"), std::string::npos);
  EXPECT_EQ(out.find("loop_par: i 1"), std::string::npos);
  EXPECT_EQ(out.find("Difference"), std::string::npos);
}

TEST_F(LoopFailureTest, DealerParStopsOnApiFaultWithoutProcessingLaterDeal)
{
  auto hands = load_hands(
      "loop_fail_dealerpar_api.txt", two_deal_body(kDealBody, kDealBody));
  ASSERT_EQ(hands.number, 2);
  corrupt_table(&hands.table_list[0]);

  testing::internal::CaptureStdout();
  const bool ok = loop_dealerpar(
      hands.dealer_list, hands.vul_list, hands.table_list,
      hands.dealerpar_list, 2, 1);
  const std::string out = testing::internal::GetCapturedStdout();

  EXPECT_FALSE(ok);
  EXPECT_NE(out.find("loop_dealerpar:"), std::string::npos);
  EXPECT_NE(out.find("loop_dealerpar: i 0"), std::string::npos);
  EXPECT_EQ(out.find("loop_dealerpar: i 1"), std::string::npos);
  EXPECT_EQ(out.find("Difference"), std::string::npos);
}
