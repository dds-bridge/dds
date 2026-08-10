/// @file report_board_timings.cpp
/// @brief Format dtest `-r` / `--report` per-board timing output.

#include "report_board_timings.hpp"

#include <algorithm>

void print_per_board_timings(
  std::ostream& out,
  std::vector<std::pair<int, int>> times)
{
  std::sort(times.begin(), times.end(), [](const auto& a, const auto& b) {
    return a.second > b.second;
  });

  out << "Per-board timings (ms) sorted by longest first:\n";
  out << "ms\tboard\n";
  for (const auto& p : times)
    out << p.second << "\t" << p.first << "\n";
  out << "\n";
}

void append_batch_board_times(
  std::vector<std::pair<int, int>>& accumulated,
  const std::vector<std::pair<int, int>>& batch_times,
  int file_offset)
{
  accumulated.reserve(accumulated.size() + batch_times.size());
  for (const auto& p : batch_times)
    accumulated.emplace_back(p.first + file_offset, p.second);
}
