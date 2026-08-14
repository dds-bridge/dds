/// @file loop_par_test.cpp
/// @brief Unit tests for dtest PAR loop timing integration.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
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

auto write_temp_hand_list(const std::string& body) -> std::string
{
  const std::string path = "loop_par_test_hand.txt";
  std::ofstream out(path);
  out << "NUMBER 1 \n" << body;
  out.close();
  return path;
}

auto cleanup(const std::string& path) -> void
{
  std::remove(path.c_str());
}

auto capture_print_hands(const TestTimer& test_timer) -> std::string
{
  std::ostringstream out;
  test_timer.print_hands(out);
  return out.str();
}

}  // namespace

TEST(LoopPar, RecordsHandCountInTimer)
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

  timer.reset();
  ASSERT_TRUE(loop_par(vul_list, table_list, par_list, number, 1));

  const std::string out = capture_print_hands(timer);
  EXPECT_TRUE(std::regex_search(
      out, std::regex(R"(Number of hands\s+1\s*(?:\n|$))")));

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
