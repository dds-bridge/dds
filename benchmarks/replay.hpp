/*
   DDS, a bridge double dummy solver.

   Replay engine for recorded DDS workloads. See recording.hpp for the reader.

   See LICENSE and README.
*/

#pragma once

#include <map>
#include <string>
#include <vector>

#include "recording.hpp"

namespace dds_replay {

struct Bucket
{
  int calls = 0;
  long long boards = 0;
  double seconds = 0.0;
  double recorded_ms = 0.0;

  auto add(long long n_boards, double elapsed_s, double rec_ms) -> void
  {
    ++calls;
    boards += n_boards;
    seconds += elapsed_s;
    recorded_ms += rec_ms;
  }
};

struct Mismatch
{
  int seq = 0;
  std::string purpose;
  std::string why;
};

struct ReplayStats
{
  std::map<std::string, Bucket> by_purpose;
  std::map<int, Bucket> by_trick;
  Bucket total;
  std::vector<Mismatch> mismatches;
  double total_seconds = 0.0;
};

// One SolverContext per worker thread, kept alive for the whole replay -- the
// arrangement a threaded client uses, and the one that lets DDS reuse a warm
// transposition table across consecutive solves.
class ReplayEngine
{
public:
  explicit ReplayEngine(int threads, int dds_mode);
  ~ReplayEngine();

  ReplayEngine(const ReplayEngine&) = delete;
  auto operator=(const ReplayEngine&) -> ReplayEngine& = delete;

  // Issue every call once, in recorded order. When `verify` is set, each
  // result is compared against the recording.
  auto run(const std::vector<Call>& calls, bool verify) -> ReplayStats;

  auto threads() const -> int { return threads_; }

private:
  class Impl;
  Impl* impl_;
  int threads_;
  int dds_mode_;
};

// Convert a PBN deal ("N:AK3.Q42... ...") into DDS remainCards bitmaps.
// Exposed for testing. Returns false on malformed input.
auto pbn_to_remain_cards(const std::string& pbn,
                         unsigned int remain[4][4]) -> bool;

// Render a result map the way the recorder does, for diffing.
auto describe_mismatch(const ResultMap& expected, const ResultMap& actual)
  -> std::string;

}  // namespace dds_replay
