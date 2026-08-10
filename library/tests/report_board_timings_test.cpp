/// @file report_board_timings_test.cpp
/// @brief Unit tests for dtest `-r` per-board timing report formatting.

#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "report_board_timings.hpp"

TEST(ReportBoardTimings, PrintsColumnHeadingsThenSortedRows)
{
  // Arrange
  std::vector<std::pair<int, int>> times = {
    {2, 10},
    {5, 42},
    {1, 7},
  };
  std::ostringstream out;

  // Act
  print_per_board_timings(out, times);

  // Assert
  EXPECT_EQ(
    out.str(),
    "Per-board timings (ms) sorted by longest first:\n"
    "ms\tboard\n"
    "42\t5\n"
    "10\t2\n"
    "7\t1\n"
    "\n");
}

TEST(ReportBoardTimings, EmptyInputPrintsTitleAndHeadingsOnly)
{
  std::ostringstream out;
  print_per_board_timings(out, {});

  EXPECT_EQ(
    out.str(),
    "Per-board timings (ms) sorted by longest first:\n"
    "ms\tboard\n"
    "\n");
}
