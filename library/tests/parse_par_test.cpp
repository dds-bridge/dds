/// @file parse_par_test.cpp
/// @brief Unit tests for dtest PAR-line parsing via read_file.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include <api/dll.h>

#include "parse.hpp"

namespace
{

auto write_temp_hand_list(const std::string& body) -> std::string
{
  const std::string path = "parse_par_test_hand.txt";
  std::ofstream out(path);
  out << "NUMBER 1 \n" << body;
  out.close();
  return path;
}

auto cleanup(const std::string& path) -> void
{
  std::remove(path.c_str());
}

}  // namespace

TEST(ParsePar, AcceptsPassOutContractsWithoutInternalSpaces)
{
  // DDS pass-out contracts are "NS:" / "EW:" (no space). Whitespace-split that
  // yields only 7 tokens; dtest must still accept the line.
  const std::string path = write_temp_hand_list(
      "PBN 0 2 4 0 \"N:AJ93.952.Q943.96 T74.84.AKJ6.QT72 K62.KJ6.872.AKJ3 "
      "Q85.AQT73.T5.854\" \n"
      "FUT 0 \n"
      "TABLE 6 5 6 5 5 6 5 6 6 6 6 6 5 6 6 6 5 6 5 6 \n"
      "PAR \"NS 0\" \"EW 0\" \"NS:\" \"EW:\" \n"
      "PAR2 \"0\" \"pass\" \n"
      "PLAY 0 \"\" \n"
      "TRACE 1 0 \n");

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

  ASSERT_TRUE(read_file(
      path,
      number,
      gib_mode,
      &dealer_list,
      &vul_list,
      &deal_list,
      &fut_list,
      &table_list,
      &par_list,
      &dealerpar_list,
      &play_list,
      &trace_list));
  ASSERT_EQ(number, 1);
  ASSERT_NE(par_list, nullptr);
  EXPECT_STREQ(par_list[0].par_score[0], "NS 0");
  EXPECT_STREQ(par_list[0].par_score[1], "EW 0");
  EXPECT_STREQ(par_list[0].par_contracts_string[0], "NS:");
  EXPECT_STREQ(par_list[0].par_contracts_string[1], "EW:");

  free(dealer_list);
  free(vul_list);
  free(deal_list);
  free(fut_list);
  free(table_list);
  free(par_list);
  free(dealerpar_list);
  free(play_list);
  free(trace_list);
  cleanup(path);
}

TEST(ParsePar, StillAcceptsNormalContractsWithSpaces)
{
  const std::string path = write_temp_hand_list(
      "PBN 0 0 0 0 \"N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 "
      "AT942.AQ4.32.KJ3\" \n"
      "FUT 0 \n"
      "TABLE 5 8 5 8 6 6 6 6 5 7 5 7 7 5 7 5 6 6 6 6 \n"
      "PAR \"NS -110\" \"EW 110\" \"NS:EW 2S\" \"EW:EW 2S\" \n"
      "PAR2 \"-110\" \"2S-EW\" \n"
      "PLAY 0 \"\" \n"
      "TRACE 1 0 \n");

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

  ASSERT_TRUE(read_file(
      path,
      number,
      gib_mode,
      &dealer_list,
      &vul_list,
      &deal_list,
      &fut_list,
      &table_list,
      &par_list,
      &dealerpar_list,
      &play_list,
      &trace_list));
  ASSERT_EQ(number, 1);
  EXPECT_STREQ(par_list[0].par_contracts_string[0], "NS:EW 2S");
  EXPECT_STREQ(par_list[0].par_contracts_string[1], "EW:EW 2S");

  free(dealer_list);
  free(vul_list);
  free(deal_list);
  free(fut_list);
  free(table_list);
  free(par_list);
  free(dealerpar_list);
  free(play_list);
  free(trace_list);
  cleanup(path);
}
