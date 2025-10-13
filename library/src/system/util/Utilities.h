#ifndef DDS_SYSTEM_UTIL_UTILITIES_H
#define DDS_SYSTEM_UTIL_UTILITIES_H

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace dds {

// A tiny, instance-scoped utility bundle holding RNG and a simple log buffer.
class Utilities {
public:
  Utilities() = default;

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

  // RNG: mt19937 seeded with a 64-bit seed for determinism across platforms.
  void seed(uint64_t seed_value) {
    rng_.seed(static_cast<std::mt19937::result_type>(seed_value));
  }

  std::mt19937& rng() { return rng_; }
  const std::mt19937& rng() const { return rng_; }

  // Logging: a very simple append-only buffer; callers can flush and clear.
  void log_append(const std::string& s) { log_.push_back(s); }
  const std::vector<std::string>& log_buffer() const { return log_; }
  void log_clear() { log_.clear(); }

  // Minimal stats: opt-in counters for smoke validation in tests.
  struct Stats {
    unsigned tt_creates = 0;
    unsigned tt_disposes = 0;
  };

  const Stats& stats() const { return stats_; }
  Stats& stats() { return stats_; }
  void stats_reset() { stats_ = Stats{}; }

private:
  std::mt19937 rng_{};                // default-constructed; seed via seed()
  std::vector<std::string> log_{};    // minimal structured log lines
  Stats stats_{};                     // optional counters
};

} // namespace dds

#endif // DDS_SYSTEM_UTIL_UTILITIES_H
