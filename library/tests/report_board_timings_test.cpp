/// @file report_board_timings_test.cpp
/// @brief Unit tests for dtest `-r` per-board timing report formatting.

#include <gtest/gtest.h>

#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "report_board_timings.hpp"

TEST(ReportBoardTimings, PrintsColumnHeadingsThenSortedRows)
{
  // Arrange: stored times are microseconds; report shows ms with one decimal.
  // Both columns are space-padded and right-aligned (no tabs — tab stops
  // shift when the ms field width varies).
  // Summary uses the same ms scale: min 7.0, max 42.5,
  // mean (42.5+10.1+7.0)/3 = 19.9, median 10.1.
  std::vector<std::pair<int, int>> times = {
    {2, 10100},
    {5, 42500},
    {1, 7000},
  };
  std::ostringstream out;

  // Act
  print_per_board_timings(out, times);

  // Assert
  EXPECT_EQ(
    out.str(),
    "\nPer-board timings (ms) sorted by longest first:\n"
    "\n"
    "  ms  board\n"
    "42.5      5\n"
    "10.1      2\n"
    " 7.0      1\n"
    "\n"
    "min 7.0  max 42.5  mean 19.9  median 10.1\n");
}

TEST(ReportBoardTimings, RightAlignsBoardWiderThanHeader)
{
  // Even count: median is the average of the two middle values.
  std::vector<std::pair<int, int>> times = {
    {12, 1000},
    {3456, 2000},
  };
  std::ostringstream out;

  print_per_board_timings(out, times);

  EXPECT_EQ(
    out.str(),
    "\nPer-board timings (ms) sorted by longest first:\n"
    "\n"
    " ms  board\n"
    "2.0   3456\n"
    "1.0     12\n"
    "\n"
    "min 1.0  max 2.0  mean 1.5  median 1.5\n");
}

TEST(ReportBoardTimings, RightAlignsMixedMsWidthsWithSpaces)
{
  // Tab-separated layout breaks once ms strings cross a tab stop; spaces keep
  // the board column fixed.
  // mean (202.0+164.1+158.4+148.3)/4 = 168.2;
  // median avg(158.4, 164.1) = 161.25 → prints as 161.2 (half-to-even).
  std::vector<std::pair<int, int>> times = {
    {1, 202000},
    {13, 164100},
    {7, 158400},
    {92, 148300},
  };
  std::ostringstream out;

  print_per_board_timings(out, times);

  EXPECT_EQ(
    out.str(),
    "\nPer-board timings (ms) sorted by longest first:\n"
    "\n"
    "   ms  board\n"
    "202.0      1\n"
    "164.1     13\n"
    "158.4      7\n"
    "148.3     92\n"
    "\n"
    "min 148.3  max 202.0  mean 168.2  median 161.2\n");
}

TEST(ReportBoardTimings, EmptyInputPrintsTitleAndHeadingsOnly)
{
  std::ostringstream out;
  print_per_board_timings(out, {});

  EXPECT_EQ(
    out.str(),
    "\nPer-board timings (ms) sorted by longest first:\n"
    "\n"
    "ms  board\n");
}

TEST(ReportBoardTimings, RestoresStreamFormattingState)
{
  // Arrange: caller-owned formatting that must survive the report printer.
  std::ostringstream out;
  out << std::scientific << std::setprecision(4) << std::left;
  const auto flags_before = out.flags();
  const auto precision_before = out.precision();

  // Act
  print_per_board_timings(out, {{1, 1000}});

  // Assert
  EXPECT_EQ(out.flags(), flags_before);
  EXPECT_EQ(out.precision(), precision_before);
}

TEST(ReportBoardTimings, SingleBoardSummaryHasEqualStats)
{
  std::ostringstream out;
  print_per_board_timings(out, {{3, 12500}});

  EXPECT_EQ(
    out.str(),
    "\nPer-board timings (ms) sorted by longest first:\n"
    "\n"
    "  ms  board\n"
    "12.5      3\n"
    "\n"
    "min 12.5  max 12.5  mean 12.5  median 12.5\n");
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

TEST(AppendBatchBoardTimes, PreReserveKeepsCapacityAcrossBatches)
{
  // Arrange: dtest -r reserves one slot per input deal before multi-batch appends.
  std::vector<std::pair<int, int>> accumulated;
  accumulated.reserve(4);
  const auto capacity_before = accumulated.capacity();

  // Act
  append_batch_board_times(accumulated, {{0, 11}, {1, 22}}, /*file_offset=*/0);
  append_batch_board_times(accumulated, {{0, 33}, {1, 44}}, /*file_offset=*/2);

  // Assert: no reallocation beyond the file-wide reserve.
  ASSERT_EQ(accumulated.size(), 4u);
  EXPECT_EQ(accumulated.capacity(), capacity_before);
}
