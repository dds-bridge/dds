/// @file report_board_timings.cpp
/// @brief Format dtest `-r` / `--report` per-board timing output.

#include "report_board_timings.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace
{

int decimal_digits(int value)
{
  const int n = std::abs(value);
  int digits = 1;
  int x = n;
  while (x >= 10)
  {
    x /= 10;
    ++digits;
  }
  if (value < 0)
    ++digits;
  return digits;
}

int board_column_width(const std::vector<std::pair<int, int>>& times)
{
  constexpr int kHeaderWidth = 5;  // "board"
  int width = kHeaderWidth;
  for (const auto& p : times)
    width = std::max(width, decimal_digits(p.first));
  return width;
}

int ms_column_width(const std::vector<std::pair<int, int>>& times)
{
  constexpr int kHeaderWidth = 2;  // "ms"
  int width = kHeaderWidth;
  for (const auto& p : times)
  {
    std::ostringstream formatted;
    formatted << std::fixed << std::setprecision(1)
              << (static_cast<double>(p.second) / 1000.0);
    width = std::max(width, static_cast<int>(formatted.str().size()));
  }
  return width;
}

}  // namespace

void print_per_board_timings(
  std::ostream& out,
  std::vector<std::pair<int, int>> times)
{
  std::sort(times.begin(), times.end(), [](const auto& a, const auto& b) {
    return a.second > b.second;
  });

  const int ms_width = ms_column_width(times);
  const int board_width = board_column_width(times);

  // Preserve caller formatting; setw is one-shot but fixed/precision/align persist.
  const auto saved_flags = out.flags();
  const auto saved_precision = out.precision();

  out << "\nPer-board timings (ms) sorted by longest first:\n\n";
  out << std::right << std::setw(ms_width) << "ms" << "  "
      << std::setw(board_width) << "board" << "\n";
  out << std::fixed << std::setprecision(1);
  for (const auto& p : times)
  {
    out << std::right << std::setw(ms_width)
        << (static_cast<double>(p.second) / 1000.0) << "  "
        << std::setw(board_width) << p.first << "\n";
  }

  out.flags(saved_flags);
  out.precision(saved_precision);
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
