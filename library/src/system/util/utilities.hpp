#ifndef DDS_SYSTEM_UTIL_UTILITIES_H
#define DDS_SYSTEM_UTIL_UTILITIES_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dds {

// A tiny, instance-scoped utility bundle for logging and stats.
class Utilities {
public:
  Utilities() = default;

  Utilities(const Utilities&) = default;
  Utilities& operator=(const Utilities&) = default;

  Utilities(Utilities&&) noexcept = default;
  Utilities& operator=(Utilities&&) noexcept = default;

  // Compile-time feature detection helpers for tests and guarded code paths.
  static constexpr bool log_enabled() {
#ifdef DDS_UTILITIES_LOG
    return true;
#else
    return false;
#endif
  }

  static constexpr bool stats_enabled() {
#ifdef DDS_UTILITIES_STATS
    return true;
#else
    return false;
#endif
  }

  // Logging: a very simple append-only buffer; callers can flush and clear.
  void log_append(const std::string& s) { log_.push_back(s); }
  const std::vector<std::string>& log_buffer() const { return log_; }
  size_t log_size() const { return log_.size(); }
  bool log_contains(const std::string& prefix) const {
    for (const auto& line : log_) {
      if (line.rfind(prefix, 0) == 0) return true; // prefix match
    }
    return false;
  }
  void log_clear() { log_.clear(); }

  // Minimal stats: opt-in counters for smoke validation in tests.
  struct Stats {
    unsigned tt_creates = 0;
    unsigned tt_disposes = 0;
  };

  const Stats& stats() const { return stats_; }
  Stats& stats() { return stats_; }
  Stats stats_snapshot() const { return stats_; }
  void stats_reset() { stats_ = Stats{}; }

private:
  std::vector<std::string> log_{};    // minimal structured log lines
  Stats stats_{};                     // optional counters
};

} // namespace dds

#endif // DDS_SYSTEM_UTIL_UTILITIES_H
