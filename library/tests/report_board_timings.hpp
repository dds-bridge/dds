/// @file report_board_timings.hpp
/// @brief Format dtest `-r` / `--report` per-board timing output.

#pragma once

#include <ostream>
#include <utility>
#include <vector>

/// Print per-board timings sorted by longest first.
///
/// Each element of @p times is `(board_index, time_ms)`. Output includes a
/// title line, a column-heading line (`ms` then `board`), then one row per
/// board as `ms\\tboard`.
void print_per_board_timings(
  std::ostream& out,
  std::vector<std::pair<int, int>> times);
