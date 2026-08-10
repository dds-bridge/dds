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

TEST(AppendBatchBoardTimes, RemapsBatchLocalIndicesByFileOffset)
{
  // Arrange: two MAXNOOFBOARDS-sized chunks would look like this after
  // RegisterRun resets scheduler times between batches.
  std::vector<std::pair<int, int>> accumulated;
  const std::vector<std::pair<int, int>> first_batch = {
    {0, 11},
    {1, 22},
  };
  const std::vector<std::pair<int, int>> second_batch = {
    {0, 33},
    {1, 44},
  };

  // Act
  append_batch_board_times(accumulated, first_batch, /*file_offset=*/0);
  append_batch_board_times(accumulated, second_batch, /*file_offset=*/200);

  // Assert: every input deal is present with its file index, not batch index.
  ASSERT_EQ(accumulated.size(), 4u);
  EXPECT_EQ(accumulated[0], (std::pair<int, int>{0, 11}));
  EXPECT_EQ(accumulated[1], (std::pair<int, int>{1, 22}));
  EXPECT_EQ(accumulated[2], (std::pair<int, int>{200, 33}));
  EXPECT_EQ(accumulated[3], (std::pair<int, int>{201, 44}));
}

TEST(AppendBatchBoardTimes, EmptyBatchIsNoOp)
{
  std::vector<std::pair<int, int>> accumulated = {{7, 1}};
  append_batch_board_times(accumulated, {}, /*file_offset=*/200);
  ASSERT_EQ(accumulated.size(), 1u);
  EXPECT_EQ(accumulated[0], (std::pair<int, int>{7, 1}));
}
