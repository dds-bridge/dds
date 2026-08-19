/*
   DDS, a bridge double dummy solver.

   Replay engine for recorded DDS workloads. See replay.hpp.

   See LICENSE and README.
*/

#include "replay.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <api/dds_c_api.h>
#include <api/dll.h>

namespace dds_replay {

namespace {

using Clock = std::chrono::steady_clock;

auto seconds_since(Clock::time_point t0) -> double
{
  return std::chrono::duration<double>(Clock::now() - t0).count();
}

auto rank_of(char c) -> int
{
  switch (c) {
    case 'A': case 'a': return 14;
    case 'K': case 'k': return 13;
    case 'Q': case 'q': return 12;
    case 'J': case 'j': return 11;
    case 'T': case 't': return 10;
    default: return (c >= '2' && c <= '9') ? (c - '0') : -1;
  }
}

// The recorder encodes a card as suit * 13 + (14 - rank), i.e. 0 = SA .. 12 = S2,
// 13 = HA, and so on. These two helpers are the inverse of each other.
auto card_code(int suit, int rank) -> int { return suit * 13 + 14 - rank; }

}  // namespace

auto pbn_to_remain_cards(const std::string& pbn, unsigned int remain[4][4])
  -> bool
{
  for (int h = 0; h < 4; ++h)
    for (int s = 0; s < 4; ++s)
      remain[h][s] = 0;

  // "<seat>:<hand> <hand> <hand> <hand>", hands running clockwise from <seat>.
  const size_t colon = pbn.find(':');
  if (colon == std::string::npos || colon == 0)
    return false;

  int first_hand;
  switch (pbn[colon - 1]) {
    case 'N': case 'n': first_hand = 0; break;
    case 'E': case 'e': first_hand = 1; break;
    case 'S': case 's': first_hand = 2; break;
    case 'W': case 'w': first_hand = 3; break;
    default: return false;
  }

  // Exactly four dot-separated hands, each with exactly four suits (three dots),
  // running clockwise from <seat>. Reject anything else -- a truncated line, a
  // hand missing a suit, an extra hand -- so the caller reports it as
  // unparseable rather than silently solving a partial, wrong deal.
  int hand = first_hand;
  int suit = 0;
  int hands_seen = 0;
  bool in_hand = false;   // any suit/rank content since the last separator?

  for (size_t i = colon + 1; i <= pbn.size(); ++i) {
    const char c = (i < pbn.size()) ? pbn[i] : ' ';   // trailing sentinel
    if (c == ' ' || c == '\t') {
      if (in_hand) {          // a hand just ended; it must have held four suits
        if (suit != 3)
          return false;
        ++hands_seen;
        hand = (hand + 1) % 4;
        suit = 0;
        in_hand = false;
      }
      continue;               // fold runs of separators, ignore leading ones
    }
    in_hand = true;
    if (c == '.') {
      if (++suit > 3)
        return false;
      continue;
    }
    const int r = rank_of(c);
    if (r < 0)
      return false;
    remain[hand][suit] |= (1u << static_cast<unsigned>(r));
  }
  return hands_seen == 4;
}

auto describe_mismatch(const ResultMap& expected, const ResultMap& actual)
  -> std::string
{
  if (actual.empty() && !expected.empty())
    return "replay returned nothing (DDS error)";

  std::string missing, extra;
  int n_missing = 0, n_extra = 0;
  for (const auto& [k, v] : expected)
    if (actual.find(k) == actual.end() && n_missing++ < 4)
      missing += (missing.empty() ? "" : ",") + k;
  for (const auto& [k, v] : actual)
    if (expected.find(k) == expected.end() && n_extra++ < 4)
      extra += (extra.empty() ? "" : ",") + k;
  if (n_missing != 0 || n_extra != 0)
    return "keys differ (missing [" + missing + "], extra [" + extra + "])";

  for (const auto& [k, want] : expected) {
    const auto& got = actual.at(k);
    if (want == got)
      continue;
    std::string w, g;
    for (size_t i = 0; i < want.size() && i < 6; ++i)
      w += (i ? "," : "") + std::to_string(want[i]);
    for (size_t i = 0; i < got.size() && i < 6; ++i)
      g += (i ? "," : "") + std::to_string(got[i]);
    return "key " + k + ": recorded [" + w + "] vs replayed [" + g + "]";
  }
  return "unknown difference";
}

// ---------------------------------------------------------------------------
// Worker pool: N threads, each owning a SolverContext for the whole replay.
// ---------------------------------------------------------------------------

class ReplayEngine::Impl
{
public:
  // The DDS mode is not held here: it travels per call, as the `mode` argument
  // to solve_batch(), so a single pool can replay calls that used different
  // modes.
  explicit Impl(int threads)
    : n_(threads), results_(nullptr), deals_(nullptr)
  {
    workers_.reserve(static_cast<size_t>(n_));
    for (int t = 0; t < n_; ++t)
      workers_.emplace_back([this] { worker_loop(); });
  }

  ~Impl()
  {
    {
      std::lock_guard<std::mutex> lock(m_);
      stop_ = true;
      ++generation_;
    }
    cv_.notify_all();
    for (auto& w : workers_)
      if (w.joinable())
        w.join();
  }

  // Solve one batch. Output is indexed by board, so results stay in recorded
  // order no matter how the work was distributed.
  auto solve_batch(const std::vector<Deal>& deals,
                   std::vector<FutureTricks>& out,
                   int target, int solutions, int mode) -> bool
  {
    out.assign(deals.size(), FutureTricks{});
    ok_.store(true, std::memory_order_relaxed);

    if (n_ <= 1) {
      // Single-threaded: reuse one context directly, no handoff cost.
      if (solo_ == nullptr)
        solo_ = dds_c_create_solvercontext_default();
      for (size_t i = 0; i < deals.size(); ++i) {
        if (dds_c_solve_board(solo_, &deals[i], target, solutions, mode,
                              &out[i]) != RETURN_NO_FAULT)
          return false;
      }
      return true;
    }

    {
      std::lock_guard<std::mutex> lock(m_);
      deals_ = &deals;
      results_ = &out;
      target_ = target;
      solutions_ = solutions;
      mode_ = mode;
      next_.store(0, std::memory_order_relaxed);
      done_ = 0;
      ++generation_;
    }
    cv_.notify_all();

    std::unique_lock<std::mutex> lock(m_);
    done_cv_.wait(lock, [this] { return done_ == n_; });
    deals_ = nullptr;
    results_ = nullptr;
    return ok_.load(std::memory_order_relaxed);
  }

private:
  auto worker_loop() -> void
  {
    // One context per worker, created once and kept warm for the whole replay.
    DDS_C_SOLVER_CTX ctx = dds_c_create_solvercontext_default();
    unsigned long long seen = 0;

    for (;;) {
      std::unique_lock<std::mutex> lock(m_);
      cv_.wait(lock, [this, &seen] { return stop_ || generation_ != seen; });
      if (stop_)
        break;
      seen = generation_;
      const std::vector<Deal>* deals = deals_;
      std::vector<FutureTricks>* out = results_;
      const int target = target_, solutions = solutions_, mode = mode_;
      lock.unlock();

      if (deals != nullptr && out != nullptr) {
        for (;;) {
          const size_t i = next_.fetch_add(1, std::memory_order_relaxed);
          if (i >= deals->size())
            break;
          if (dds_c_solve_board(ctx, &(*deals)[i], target, solutions, mode,
                                &(*out)[i]) != RETURN_NO_FAULT)
            ok_.store(false, std::memory_order_relaxed);
        }
      }

      {
        std::lock_guard<std::mutex> lock2(m_);
        ++done_;
      }
      done_cv_.notify_one();
    }

    if (ctx != nullptr)
      dds_c_destroy_solvercontext(ctx);
  }

  int n_;
  std::vector<std::thread> workers_;

  std::mutex m_;
  std::condition_variable cv_, done_cv_;
  bool stop_ = false;
  unsigned long long generation_ = 0;
  int done_ = 0;

  std::vector<FutureTricks>* results_;
  const std::vector<Deal>* deals_;
  std::atomic<size_t> next_{0};
  std::atomic<bool> ok_{true};
  int target_ = -1, solutions_ = 1, mode_ = 1;

  DDS_C_SOLVER_CTX solo_ = nullptr;

public:
  auto destroy_solo() -> void
  {
    if (solo_ != nullptr) {
      dds_c_destroy_solvercontext(solo_);
      solo_ = nullptr;
    }
  }
};

ReplayEngine::ReplayEngine(int threads, int dds_mode)
  : impl_(new Impl(threads)), threads_(threads), dds_mode_(dds_mode)
{
}

ReplayEngine::~ReplayEngine()
{
  impl_->destroy_solo();
  delete impl_;
}

auto ReplayEngine::run(const std::vector<Call>& calls, bool verify)
  -> ReplayStats
{
  ReplayStats stats;
  std::vector<Deal> deals;
  std::vector<FutureTricks> solved;

  for (const Call& call : calls) {
    double elapsed = 0.0;
    long long boards = 1;
    ResultMap actual;

    if (call.kind == Call::Kind::Solve) {
      // Build the batch. Every board in a call shares the strain, the leader
      // and the cards already played to the current trick.
      const int trump = ((call.strain_i - 1) % 5 + 5) % 5;

      Deal proto{};
      proto.trump = trump;
      proto.first = call.leader_i;
      for (int k = 0; k < 3; ++k) {
        proto.currentTrickSuit[k] = 0;
        proto.currentTrickRank[k] = 0;
      }
      for (size_t k = 0; k < call.current_trick.size() && k < 3; ++k) {
        proto.currentTrickSuit[k] = call.current_trick[k] / 13;
        proto.currentTrickRank[k] = 14 - call.current_trick[k] % 13;
      }

      deals.clear();
      deals.reserve(call.hands_pbn.size());
      bool parsed = true;
      for (const std::string& pbn : call.hands_pbn) {
        Deal dl = proto;
        if (!pbn_to_remain_cards(pbn, dl.remainCards)) {
          parsed = false;
          break;
        }
        deals.push_back(dl);
      }
      boards = static_cast<long long>(deals.size());

      if (!parsed) {
        if (verify)
          stats.mismatches.push_back(
            {call.seq, call.purpose, "unparseable PBN in recording"});
        continue;
      }

      const auto t0 = Clock::now();
      const bool ok = impl_->solve_batch(deals, solved, -1, call.solutions,
                                         dds_mode_);
      elapsed = seconds_since(t0);

      // `solved_ok` tracks whether we have a well-formed result to compare. It
      // stays false when the solve failed, or when a board came back with no
      // cards -- both of which must be reported, not silently skipped.
      bool solved_ok = ok;

      if (!ok) {
        if (verify)
          stats.mismatches.push_back({call.seq, call.purpose, "DDS error"});
      } else if (call.solutions == 1) {
        // Only the best and worst trick counts for the side to play. A board
        // with no cards is a solver fault, not a 0 to index blindly.
        bool no_cards = false;
        for (const FutureTricks& fut : solved)
          if (fut.cards <= 0) { no_cards = true; break; }
        if (no_cards) {
          solved_ok = false;
          if (verify)
            stats.mismatches.push_back(
              {call.seq, call.purpose, "DDS returned a board with no cards"});
        } else {
          std::vector<int>& mx = actual["max"];
          std::vector<int>& mn = actual["min"];
          mx.reserve(solved.size());
          mn.reserve(solved.size());
          for (const FutureTricks& fut : solved) {
            mx.push_back(fut.score[0]);
            mn.push_back(fut.score[fut.cards - 1]);
          }
        }
      } else {
        // One list per playable card, including cards that are equivalent to
        // the one DDS reported (the `equals` bitmap), in board order.
        for (const FutureTricks& fut : solved) {
          for (int i = 0; i < fut.cards; ++i) {
            const int suit = fut.suit[i];
            actual[std::to_string(card_code(suit, fut.rank[i]))]
              .push_back(fut.score[i]);
            const int eq = fut.equals[i];
            for (int rank = 2; rank <= 14; ++rank) {
              if ((eq & (1 << rank)) != 0)
                actual[std::to_string(card_code(suit, rank))]
                  .push_back(fut.score[i]);
            }
          }
        }
      }

      // Compare whenever the solve produced a result, even an empty one: an
      // empty map where the recording has entries is itself a regression and
      // must surface, not be skipped by an `!actual.empty()` guard.
      if (verify && solved_ok && actual != call.result)
        stats.mismatches.push_back(
          {call.seq, call.purpose, describe_mismatch(call.result, actual)});
    } else {
      // Par: the double dummy table plus the par score for the vulnerability.
      int v = 0;
      if (call.vuln.size() >= 2) {
        if (call.vuln[0]) v = 2;
        if (call.vuln[1]) v = 3;
        if (call.vuln[0] && call.vuln[1]) v = 1;
      }

      DdTableDealPBN table_deal{};
      const std::string pbn = "N:" + call.hand;
      const size_t n = pbn.size() < sizeof(table_deal.cards) - 1
        ? pbn.size() : sizeof(table_deal.cards) - 1;
      for (size_t k = 0; k < n; ++k)
        table_deal.cards[k] = pbn[k];
      table_deal.cards[n] = '\0';

      // SidesParBin is what the string-returning Par() computes internally
      // before formatting; sides[0].score is the NS-view par, the signed
      // integer the recorder stored (Par() prints it as "NS <score>").
      DdTableResults table{};
      ParResultsMaster sides[2]{};
      const auto t0 = Clock::now();
      const int rc1 = CalcDDtablePBN(table_deal, &table);
      const int rc2 = (rc1 == RETURN_NO_FAULT) ? SidesParBin(&table, sides, v) : rc1;
      elapsed = seconds_since(t0);

      if (verify) {
        if (rc2 != RETURN_NO_FAULT)
          stats.mismatches.push_back(
            {call.seq, "par", "DDS error " + std::to_string(rc2)});
        else if (sides[0].score != call.par_result)
          // Same shape as describe_mismatch's line, so par regressions read
          // like solve regressions in the report.
          stats.mismatches.push_back(
            {call.seq, "par",
             "recorded [" + std::to_string(call.par_result) +
             "] vs replayed [" + std::to_string(sides[0].score) + "]"});
      }
    }

    const std::string purpose = call.purpose.empty() ? "(none)" : call.purpose;
    stats.by_purpose[purpose].add(boards, elapsed, call.recorded_ms);
    stats.by_trick[call.trick].add(boards, elapsed, call.recorded_ms);
    stats.total.add(boards, elapsed, call.recorded_ms);
    stats.total_seconds += elapsed;
  }

  return stats;
}

}  // namespace dds_replay
