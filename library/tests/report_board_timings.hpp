/// @file report_board_timings.hpp
/// @brief Format dtest `-r` / `--report` per-board timing output.

#pragma once

#include <ostream>
#include <utility>
#include <vector>

/// Print per-board timings sorted by longest first.
///
/// Each element of @p times is `(board_index, time_ms)`, where `board_index`
/// is the 0-based deal index in the input file. Output includes a title line,
/// a column-heading line (`ms` then `board`), then one row per board as
/// `ms\\tboard`.
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
