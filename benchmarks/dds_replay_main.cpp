/*
   DDS, a bridge double dummy solver.

   Replay a recorded DDS workload as a benchmark.

   Reads a JSON Lines recording of an application's DDS traffic, re-issues every
   call in the order it was made, times them, and checks the answers still match
   what was recorded. Because the calls are a real client's, the result reflects
   the actual mix of trick depths, batch sizes and `solutions` values rather than
   a guess at what a representative deal looks like.

       dds_replay recording.jsonl
       dds_replay recording.jsonl --threads 8 --threads 16 --repeat 3
       dds_replay recording.jsonl --purpose play --tricks
       dds_replay recording.jsonl --list

   See LICENSE and README.
*/

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "recording.hpp"
#include "replay.hpp"

namespace {

using dds_replay::Bucket;
using dds_replay::Call;
using dds_replay::Recording;
using dds_replay::ReplayEngine;
using dds_replay::ReplayStats;

struct Options
{
  std::string path;
  std::vector<int> threads;
  std::vector<std::string> purpose;
  std::vector<int> boards;
  std::vector<int> solutions;
  int dds_mode = -1;
  int repeat = 1;
  int min_trick = 0;
  int max_trick = 0;
  int limit = 0;
  bool warmup = false;
  bool no_par = false;
  bool no_verify = false;
  bool tricks = false;
  bool list = false;
};

auto usage() -> int
{
  std::printf(
    "Replay a recorded DDS workload as a benchmark.\n\n"
    "Usage: dds_replay <recording.jsonl> [options]\n\n"
    "  --threads N     worker threads; repeat to sweep several (default: recorded)\n"
    "  --dds-mode N    solver mode to replay with (default: recorded)\n"
    "  --repeat N      run the workload N times, report the best\n"
    "  --warmup        one untimed pass first\n"
    "  --purpose P     only this purpose (bid/lead/play/claimcheck/par); repeatable\n"
    "  --board N       only this board number; repeatable\n"
    "  --solutions N   only calls with this solutions value; repeatable\n"
    "  --min-trick N / --max-trick N\n"
    "  --limit N       stop after N calls\n"
    "  --no-par        skip par calculations\n"
    "  --no-verify     do not compare results against the recording\n"
    "  --tricks        also break the report down by trick number\n"
    "  --list          summarise the recording and exit\n");
  return 2;
}

auto contains(const std::vector<int>& v, int x) -> bool
{
  return std::find(v.begin(), v.end(), x) != v.end();
}

auto contains(const std::vector<std::string>& v, const std::string& x) -> bool
{
  return std::find(v.begin(), v.end(), x) != v.end();
}

auto select(const Recording& rec, const Options& opt) -> std::vector<Call>
{
  std::vector<Call> out;
  for (const Call& c : rec.calls) {
    if (c.kind == Call::Kind::Par) {
      // --purpose names the buckets the report uses, and par is one of them,
      // so `--purpose bid` must exclude par rather than silently leave it in.
      if (opt.no_par)
        continue;
      if (!opt.purpose.empty() && !contains(opt.purpose, "par"))
        continue;
    } else {
      if (!opt.purpose.empty() && !contains(opt.purpose, c.purpose))
        continue;
      if (opt.min_trick != 0 && c.trick < opt.min_trick)
        continue;
      if (opt.max_trick != 0 && c.trick > opt.max_trick)
        continue;
      if (!opt.solutions.empty() && !contains(opt.solutions, c.solutions))
        continue;
    }
    if (!opt.boards.empty() && !contains(opt.boards, c.board))
      continue;
    out.push_back(c);
    if (opt.limit != 0 && static_cast<int>(out.size()) >= opt.limit)
      break;
  }
  return out;
}

auto header() -> void
{
  std::printf("%-12s%8s%10s%10s%11s%11s%13s\n",
              "", "calls", "boards", "seconds", "ms/call", "ms/board",
              "rec ms/call");
}

auto row(const std::string& name, const Bucket& b) -> void
{
  const double ms_call  = b.calls  != 0 ? b.seconds * 1000 / b.calls : 0.0;
  const double ms_board = b.boards != 0
    ? b.seconds * 1000 / static_cast<double>(b.boards) : 0.0;
  const double rec_call = b.calls  != 0 ? b.recorded_ms / b.calls : 0.0;
  std::printf("%-12s%8d%10lld%10.3f%11.2f%11.3f%13.2f\n",
              name.c_str(), b.calls, b.boards, b.seconds, ms_call, ms_board,
              rec_call);
}

auto print_list(const Recording& rec, const std::vector<Call>& calls,
                const std::string& path) -> void
{
  int n_solve = 0;
  long long n_boards = 0;
  double rec_s = 0.0;
  std::set<int> boards;
  std::map<std::string, Bucket> by_purpose;

  for (const Call& c : calls) {
    const long long b = (c.kind == Call::Kind::Solve)
      ? static_cast<long long>(c.hands_pbn.size()) : 1;
    if (c.kind == Call::Kind::Solve) { ++n_solve; n_boards += b; }
    rec_s += c.recorded_ms / 1000.0;
    boards.insert(c.board);
    by_purpose[c.purpose.empty() ? "(none)" : c.purpose]
      .add(b, 0.0, c.recorded_ms);
  }

  std::printf("%s\n", path.c_str());
  std::printf("  recorded %s on %s with DDS %s mode %d %d threads\n",
              rec.created.c_str(), rec.host.c_str(), rec.dds_version.c_str(),
              rec.dds_mode, rec.threads);
  std::printf("  %d deals, %d solve calls (%lld boards), %d par, "
              "%.1f s of DDS when recorded\n",
              static_cast<int>(boards.size()), n_solve, n_boards,
              static_cast<int>(calls.size()) - n_solve, rec_s);
  std::printf("  %-14s%8s%10s%10s\n", "purpose", "calls", "boards", "rec s");
  for (const auto& [purpose, b] : by_purpose)
    std::printf("  %-14s%8d%10lld%10.3f\n", purpose.c_str(), b.calls, b.boards,
                b.recorded_ms / 1000.0);
}

auto print_report(const Recording& rec, const std::vector<Call>& calls,
                  const ReplayStats& stats, const std::vector<double>& runs,
                  int threads, int dds_mode, bool show_tricks) -> void
{
  int n_solve = 0;
  long long n_boards = 0;
  std::set<int> boards;
  for (const Call& c : calls) {
    if (c.kind == Call::Kind::Solve) {
      ++n_solve;
      n_boards += static_cast<long long>(c.hands_pbn.size());
    }
    boards.insert(c.board);
  }

  std::printf("\n  recorded : %s on %s (%s)  DDS %s  mode %d  %d threads\n",
              rec.created.c_str(), rec.host.c_str(), rec.platform.c_str(),
              rec.dds_version.c_str(), rec.dds_mode, rec.threads);
  std::printf("  replaying: mode %d  %d threads\n", dds_mode, threads);
  std::printf("  workload : %d solve calls (%lld boards) + %d par, over %d deals\n\n",
              n_solve, n_boards, static_cast<int>(calls.size()) - n_solve,
              static_cast<int>(boards.size()));

  header();
  std::printf("%s\n", std::string(75, '-').c_str());
  std::vector<std::pair<std::string, Bucket>> ordered(stats.by_purpose.begin(),
                                                      stats.by_purpose.end());
  std::sort(ordered.begin(), ordered.end(),
            [](const auto& a, const auto& b) { return a.second.seconds > b.second.seconds; });
  for (const auto& [purpose, b] : ordered)
    row(purpose, b);
  std::printf("%s\n", std::string(75, '-').c_str());
  row("TOTAL", stats.total);

  if (show_tricks) {
    std::printf("\nby trick\n");
    header();
    std::printf("%s\n", std::string(75, '-').c_str());
    for (const auto& [trick, b] : stats.by_trick) {
      char label[16];
      if (trick == 0)
        std::snprintf(label, sizeof(label), "par");
      else
        std::snprintf(label, sizeof(label), "t%02d", trick);
      row(label, b);
    }
  }

  std::printf("\n");
  if (runs.size() > 1) {
    std::string all;
    for (double r : runs)
      all += (all.empty() ? "" : "  ") +
             std::string(std::to_string(r).substr(0, 6));
    std::printf("DDS time: %.3f s   (best of %d: %s)\n", stats.total_seconds,
                static_cast<int>(runs.size()), all.c_str());
  } else {
    std::printf("DDS time: %.3f s\n", stats.total_seconds);
  }

  const double rec_s = stats.total.recorded_ms / 1000.0;
  if (rec_s > 0.0)
    std::printf("recorded : %.3f s  (this run is %.2fx the recorded time)\n",
                rec_s, stats.total_seconds / rec_s);

  if (!stats.mismatches.empty()) {
    std::printf("VERIFY: %d of %d calls returned a different result than recorded\n",
                static_cast<int>(stats.mismatches.size()),
                static_cast<int>(calls.size()));
    for (size_t i = 0; i < stats.mismatches.size() && i < 10; ++i)
      std::printf("  seq %d (%s): %s\n", stats.mismatches[i].seq,
                  stats.mismatches[i].purpose.c_str(),
                  stats.mismatches[i].why.c_str());
    if (stats.mismatches.size() > 10)
      std::printf("  ... and %d more\n",
                  static_cast<int>(stats.mismatches.size()) - 10);
  } else {
    std::printf("VERIFY: all %d calls returned the recorded result\n",
                static_cast<int>(calls.size()));
  }
}

}  // namespace

auto main(int argc, char* argv[]) -> int
{
  Options opt;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next_int = [&](int& dst) {
      if (i + 1 < argc) dst = std::atoi(argv[++i]);
    };
    if (a == "--threads" && i + 1 < argc) opt.threads.push_back(std::atoi(argv[++i]));
    else if (a == "--purpose" && i + 1 < argc) opt.purpose.emplace_back(argv[++i]);
    else if (a == "--board" && i + 1 < argc) opt.boards.push_back(std::atoi(argv[++i]));
    else if (a == "--solutions" && i + 1 < argc) opt.solutions.push_back(std::atoi(argv[++i]));
    else if (a == "--dds-mode") next_int(opt.dds_mode);
    else if (a == "--repeat") next_int(opt.repeat);
    else if (a == "--min-trick") next_int(opt.min_trick);
    else if (a == "--max-trick") next_int(opt.max_trick);
    else if (a == "--limit") next_int(opt.limit);
    else if (a == "--warmup") opt.warmup = true;
    else if (a == "--no-par") opt.no_par = true;
    else if (a == "--no-verify") opt.no_verify = true;
    else if (a == "--tricks") opt.tricks = true;
    else if (a == "--list") opt.list = true;
    else if (a == "-h" || a == "--help") return usage();
    else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return usage(); }
    else if (opt.path.empty()) opt.path = a;
    else { std::fprintf(stderr, "unexpected argument %s\n", a.c_str()); return usage(); }
  }

  if (opt.path.empty()) {
    // Default to the workload committed alongside this benchmark, so
    // `bazel run //benchmarks:dds_replay` works with no arguments.
    opt.path = dds_replay::find_runfile(
      "_main/benchmarks/testdata/dds-camrose-1-32.jsonl",
      argc > 0 ? argv[0] : "");
    if (opt.path.empty()) {
      std::fprintf(stderr, "no recording given, and the bundled one was not "
                           "found in the runfiles\n");
      return usage();
    }
  }

  Recording rec;
  std::string error;
  if (!dds_replay::load_recording(opt.path, rec, error)) {
    std::fprintf(stderr, "%s\n", error.c_str());
    return 1;
  }

  const std::vector<Call> calls = select(rec, opt);
  if (calls.empty()) {
    std::fprintf(stderr, "no calls left after filtering\n");
    return 1;
  }

  if (opt.list) {
    print_list(rec, calls, opt.path);
    return 0;
  }

  const int dds_mode = (opt.dds_mode >= 0) ? opt.dds_mode : rec.dds_mode;
  std::vector<int> thread_counts = opt.threads;
  if (thread_counts.empty()) {
    int t = rec.threads;
    if (t <= 0)
      t = static_cast<int>(std::thread::hardware_concurrency());
    thread_counts.push_back(std::max(1, t));
  }

  int exit_code = 0;
  for (const int threads : thread_counts) {
    ReplayEngine engine(std::max(1, threads), dds_mode);

    if (opt.warmup)
      (void) engine.run(calls, /*verify=*/false);

    std::vector<double> runs;
    ReplayStats best;
    for (int r = 0; r < std::max(1, opt.repeat); ++r) {
      ReplayStats stats = engine.run(calls, !opt.no_verify);
      runs.push_back(stats.total_seconds);
      if (r == 0 || stats.total_seconds < best.total_seconds)
        best = std::move(stats);
    }

    std::printf("\n%s\n", std::string(75, '=').c_str());
    std::printf("DDS replay - %s\n", opt.path.c_str());
    std::printf("%s\n", std::string(75, '=').c_str());
    print_report(rec, calls, best, runs, std::max(1, threads), dds_mode,
                 opt.tricks);

    if (!best.mismatches.empty())
      exit_code = 1;
  }

  return exit_code;
}
