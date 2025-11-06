#ifndef DDS_SYSTEM_UTIL_UTILITIES_H
#define DDS_SYSTEM_UTIL_UTILITIES_H

#include <cstdint>
#include <random>
#include <memory>
#include <string>
#include <vector>

namespace dds {

// A tiny, instance-scoped utility bundle holding RNG and a simple log buffer.
class Utilities {
public:
  Utilities() = default;

  // Explicit deep-copy semantics to keep class copyable while
  // preserving small object size via pointer-backed RNG storage.
  Utilities(const Utilities& other)
    : log_(other.log_), stats_(other.stats_)
  {
    if (other.rng_) rng_ = std::make_unique<std::mt19937>(*other.rng_);
  }

  Utilities& operator=(const Utilities& other)
  {
    if (this == &other) return *this;
    if (other.rng_) rng_ = std::make_unique<std::mt19937>(*other.rng_);
    else rng_.reset();
    log_ = other.log_;
    stats_ = other.stats_;
    return *this;
  }

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

  // RNG: mt19937 seeded with a 64-bit seed for determinism across platforms.
  // Lazily instantiate the engine to avoid paying initialization unless used.
  void seed(uint64_t seed_value) {
    if (!rng_) rng_ = std::make_unique<std::mt19937>();
    rng_->seed(static_cast<std::mt19937::result_type>(seed_value));
  }

  std::mt19937& rng() {
    if (!rng_) rng_ = std::make_unique<std::mt19937>();
    return *rng_;
  }
  const std::mt19937& rng() const {
    if (!rng_) rng_ = std::make_unique<std::mt19937>();
    return *rng_;
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
  mutable std::unique_ptr<std::mt19937> rng_;  // created on demand (deep-copyable)
  std::vector<std::string> log_{};    // minimal structured log lines
  Stats stats_{};                     // optional counters
};

} // namespace dds

#endif // DDS_SYSTEM_UTIL_UTILITIES_H
