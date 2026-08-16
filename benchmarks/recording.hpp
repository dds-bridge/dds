/*
   DDS, a bridge double dummy solver.

   Reader for recorded DDS workloads (JSON Lines).

   A recording is one JSON object per line, written by an application that
   logged its own DDS traffic. Replaying it measures the solver on the call mix
   a real client actually produced -- trick depths, batch sizes, `solutions`
   values and all -- rather than on a guess at a representative deal.

   Only the small subset of JSON the recorder emits is supported (objects,
   arrays, strings, numbers, true/false/null). That keeps the benchmark free of
   a third-party JSON dependency.

   See LICENSE and README.
*/

#pragma once

#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace dds_replay {

// ---------------------------------------------------------------------------
// Minimal JSON value
// ---------------------------------------------------------------------------

struct JsonValue
{
  enum class Kind { Null, Bool, Number, String, Array, Object };

  Kind kind = Kind::Null;
  bool boolean = false;
  double number = 0.0;
  std::string text;
  std::vector<JsonValue> items;
  std::vector<std::pair<std::string, JsonValue>> members;

  auto find(const std::string& key) const -> const JsonValue*
  {
    for (const auto& [k, v] : members)
      if (k == key)
        return &v;
    return nullptr;
  }

  auto int_or(const std::string& key, int fallback) const -> int
  {
    const JsonValue* v = find(key);
    return (v != nullptr && v->kind == Kind::Number)
      ? static_cast<int>(v->number) : fallback;
  }

  auto double_or(const std::string& key, double fallback) const -> double
  {
    const JsonValue* v = find(key);
    return (v != nullptr && v->kind == Kind::Number) ? v->number : fallback;
  }

  auto string_or(const std::string& key, const std::string& fallback) const
    -> std::string
  {
    const JsonValue* v = find(key);
    return (v != nullptr && v->kind == Kind::String) ? v->text : fallback;
  }
};

// Parse one JSON document. Returns false (and sets `error`) on malformed input.
auto parse_json(const std::string& text, JsonValue& out, std::string& error)
  -> bool;

// ---------------------------------------------------------------------------
// Recorded calls
// ---------------------------------------------------------------------------

// A result is a set of named integer lists, one entry per board in the call.
// For solutions == 1 the names are "max"/"min"; for solutions == 3 they are
// card codes (suit * 13 + 14 - rank) rendered as decimal strings. Comparing
// this canonical form is what verifies a replay against its recording.
using ResultMap = std::map<std::string, std::vector<int>>;

struct Call
{
  enum class Kind { Solve, Par };

  Kind kind = Kind::Solve;
  int seq = 0;
  int board = 0;
  std::string purpose;
  int trick = 0;
  double recorded_ms = 0.0;

  // Solve
  int strain_i = 0;
  int leader_i = 0;
  int solutions = 1;
  std::vector<int> current_trick;      // card codes, at most 3
  std::vector<std::string> hands_pbn;  // one PBN deal per sampled board
  ResultMap result;

  // Par
  std::string hand;                    // PBN body, without the "N:" prefix
  std::vector<bool> vuln;              // {NS, EW}
  int par_result = 0;
};

struct Recording
{
  // From the "meta" record; all optional.
  std::string created, host, platform, dds_version;
  int dds_mode = 1;
  int threads = 0;

  std::vector<Call> calls;
  int deals = 0;  // distinct board numbers among the calls
};

// Load a recording. Unparseable lines are skipped with a warning on stderr (a
// run killed mid-write leaves a truncated last line; that should not cost the
// whole recording). Returns false only if the file cannot be opened.
auto load_recording(const std::string& path, Recording& out, std::string& error)
  -> bool;

// Resolve a Bazel runfile (e.g. "_main/benchmarks/testdata/x.jsonl") to a real
// path, or "" if it cannot be found. Windows does not materialise the runfiles
// tree -- that would need symlinks -- so the manifest has to be consulted there.
// `bazel test` exports the manifest location in the environment; `bazel run`
// does not, so pass argv[0] and the manifest beside the binary will be used.
auto find_runfile(const std::string& logical, const std::string& argv0 = "")
  -> std::string;

}  // namespace dds_replay
