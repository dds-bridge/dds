/// @file args_test.cpp
/// @brief Unit tests for dtest --max / --min option parsing.

#include <gtest/gtest.h>

#include <fstream>
#include <string>

#include "args.hpp"
#include "cst.hpp"

OptionsType options;

namespace
{

std::string make_temp_input_file()
{
  const std::string path =
    std::string(::testing::TempDir()) + "args_test_input.txt";
  std::ofstream out(path);
  out << "placeholder\n";
  return path;
}

}  // namespace

TEST(Args, MaxAndMinFlagsDefaultOff)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char* argv[] = {arg0, arg_f, const_cast<char*>(path.c_str())};
  read_args(3, argv);
  EXPECT_FALSE(options.show_min_);
  EXPECT_FALSE(options.show_max_);
}

TEST(Args, MaxFlagEnablesShowMax)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_max[] = "--max";
  char* argv[] = {arg0, arg_f, const_cast<char*>(path.c_str()), arg_max};
  read_args(4, argv);
  EXPECT_TRUE(options.show_max_);
  EXPECT_FALSE(options.show_min_);
}

TEST(Args, MinFlagEnablesShowMin)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_min[] = "--min";
  char* argv[] = {arg0, arg_f, const_cast<char*>(path.c_str()), arg_min};
  read_args(4, argv);
  EXPECT_TRUE(options.show_min_);
  EXPECT_FALSE(options.show_max_);
}

TEST(Args, MaxAndMinFlagsCanCombine)
{
  const std::string path = make_temp_input_file();
  char arg0[] = "dtest";
  char arg_f[] = "-f";
  char arg_max[] = "--max";
  char arg_min[] = "--min";
  char* argv[] = {
    arg0, arg_f, const_cast<char*>(path.c_str()), arg_max, arg_min};
  read_args(5, argv);
  EXPECT_TRUE(options.show_min_);
  EXPECT_TRUE(options.show_max_);
}
