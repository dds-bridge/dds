/// @file report_board_timings.hpp
/// @brief Format dtest `-r` / `--report` per-board timing output.

#pragma once

#include <ostream>
#include <utility>
#include <vector>

/// Print per-board timings sorted by longest first.
///
/// Each element of @p times is `(board_index, time_us)`, where `board_index`
/// is the 0-based deal index in the input file and `time_us` is wall time in
/// microseconds. Printed times are milliseconds with two decimal places.
/// Both the `ms` and `board` columns are right-aligned with spaces (no tabs).
/// Output includes a title line, a heading line, one row per board, then a
/// blank line and a summary of min/max/mean/median/stddev when @p times is
/// non-empty. For an even count, median is the average of the two middle
/// values. `stddev` is the sample standard deviation (n-1); for a single
/// board it is 0.
void print_per_board_timings(
    std::ostream& out,
    std::vector<std::pair<int, int>> times);

/// Append one solve-batch's scheduler timings into a file-wide report.
///
/// @p batch_times entries use batch-local indices `(0 .. batch_size-1)`.
/// Each is remapped to `board_index + file_offset` so multi-batch runs
/// (chunks of `MAXNOOFBOARDS`) report every deal in the input file.
void append_batch_board_times(
    std::vector<std::pair<int, int>>& accumulated,
    const std::vector<std::pair<int, int>>& batch_times,
    int file_offset);
