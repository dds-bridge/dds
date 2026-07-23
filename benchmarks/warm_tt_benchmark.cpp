/// @file warm_tt_benchmark.cpp
/// @brief Quantifies the warm-transposition-table win from reusing one
///        DDS_C_SOLVER_CTX across successive similar solves.
///
/// The C-ABI in <api/dds_c_api.h> hands out an opaque solver-context handle
/// that owns the transposition table (TT), so a caller can keep the TT warm
/// across calls instead of paying for a cold table every solve. This benchmark
/// measures what that is worth on the workload it matters most for: opening-lead
/// evaluation in a bridge engine.
///
/// Two ways to value the opening leads of a deal:
///   A       one solve with solutions=3, which scores all 13 leads at once.
///   B       prune to the top-K leads (an engine would use a NN; here we take
///           the K best from A), PLAY each, and solve the resulting position
///           with solutions=1.
/// B is only competitive if the TT survives between the K post-lead positions,
/// which differ by just a couple of cards and so pass the solver's similar-deal
/// gate (no TT reset). So B is measured twice:
///   B_cold  a FRESH context per lead  -- TT reset every time
///   B_warm  ONE context for all K     -- TT reused
///
/// Reports total ms and the B/A ratios for K = 4 and K = 6.
///
/// This is a timing benchmark, so the only *hard* assertions are correctness
/// ones: every solve succeeds and returns at least one card, and warm and cold
/// agree on every lead's score (i.e. TT reuse never changes the answer). The
/// speed story is printed, not asserted, to keep the test non-flaky under CI
/// load.
///
/// The sibling benchmark here, //benchmarks:dds_replay, measures the same reuse
/// on a recorded client workload; this one isolates it on a single synthetic
/// pattern, cheaply enough to run on every build.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include <api/dds.h>
#include <api/dds_c_api.h>

namespace {

using Card  = std::pair<int, int>;                // (suit 0..3, rank 2..14)
using Hands = std::array<std::vector<Card>, 4>;   // indexed N,E,S,W

constexpr int kWest = 3;                          // opening leader vs a South declarer
constexpr int kStrainNT = 4, kStrainSpades = 0;   // DDS trump encoding

// Owns a solver-context handle. The gtest ASSERT_* macros expand to a bare
// `return`, so any explicit destroy call can be jumped over on a failing path;
// this releases the handle however the scope is left.
class ScopedContext {
public:
  ScopedContext() : ctx_(dds_c_create_solvercontext_default()) {}
  ~ScopedContext() { destroy(); }

  ScopedContext(const ScopedContext&) = delete;
  auto operator=(const ScopedContext&) -> ScopedContext& = delete;

  [[nodiscard]] auto get() const -> DDS_C_SOLVER_CTX { return ctx_; }

  // Release early, so the cost of destruction lands inside the timed region
  // rather than at the end of the enclosing scope. Idempotent.
  auto destroy() -> void
  {
    if (ctx_ != nullptr) {
      dds_c_destroy_solvercontext(ctx_);
      ctx_ = nullptr;
    }
  }

private:
  DDS_C_SOLVER_CTX ctx_;
};

// Deal 52 cards into four hands from the caller's generator. This is only as
// reproducible as `rng` is: the determinism comes from the caller seeding it,
// not from anything here. See the seeding comment in the test case below.
auto make_deal(std::mt19937& rng) -> Hands
{
  std::vector<Card> deck;
  for (int s = 0; s < 4; ++s)
    for (int r = 2; r <= 14; ++r)
      deck.emplace_back(s, r);
  std::shuffle(deck.begin(), deck.end(), rng);
  Hands hands;
  for (int h = 0; h < 4; ++h)
    for (int i = 0; i < 13; ++i)
      hands[static_cast<size_t>(h)].push_back(deck[static_cast<size_t>(h * 13 + i)]);
  return hands;
}

// Build a binary Deal. With `lead` supplied, the position is the one AFTER the
// leader has played that card: it is removed from the leader's holding and
// becomes the first card of the current trick.
auto make_position(const Hands& hands, int trump, int leader,
                   const Card* lead = nullptr) -> Deal
{
  Deal dl{};
  dl.trump = trump;
  dl.first = leader;
  for (int h = 0; h < DDS_HANDS; ++h)
    for (int s = 0; s < DDS_SUITS; ++s)
      dl.remainCards[h][s] = 0;

  for (int h = 0; h < DDS_HANDS; ++h) {
    for (const auto& [s, r] : hands[static_cast<size_t>(h)]) {
      if (lead != nullptr && h == leader && s == lead->first && r == lead->second)
        continue;                                  // the card just led
      dl.remainCards[h][s] |= (1u << static_cast<unsigned>(r));
    }
  }

  if (lead != nullptr) {
    dl.currentTrickSuit[0] = lead->first;
    dl.currentTrickRank[0] = lead->second;
  }
  return dl;
}

// Top-K candidate leads, ranked by the solutions=3 result `fut` (a stand-in for
// an engine's NN shortlist: here simply the double-dummy-best leads).
auto top_k_leads(const FutureTricks& fut, int k) -> std::vector<Card>
{
  std::vector<int> idx;
  for (int i = 0; i < fut.cards; ++i) idx.push_back(i);
  std::stable_sort(idx.begin(), idx.end(),
    [&](int a, int b) { return fut.score[a] > fut.score[b]; });
  std::vector<Card> leads;
  for (int i = 0; i < static_cast<int>(idx.size()) && i < k; ++i)
    leads.emplace_back(fut.suit[idx[static_cast<size_t>(i)]],
                       fut.rank[idx[static_cast<size_t>(i)]]);
  return leads;
}

using Clock = std::chrono::steady_clock;
auto ms_since(Clock::time_point t0) -> double
{
  return std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
}

struct Totals { double a = 0, b_cold = 0, b_warm = 0; int deals = 0; };

// One contract across all deals for a given K, accumulating ms into `t`.
// Returns via out-param because the gtest ASSERT_* macros below expand to a
// `return;` on failure and so require a void-returning function.
auto run(const std::vector<Hands>& deals, int trump, int leader, int k,
         const char* label, Totals& t) -> void
{
  for (size_t d = 0; d < deals.size(); ++d) {
    const Hands& hands = deals[d];

    // --- A: one fresh-context solutions=3 solve valuing all leads. ---
    const Deal full = make_position(hands, trump, leader);
    FutureTricks futA{};
    int rcA = RETURN_UNKNOWN_FAULT;
    {
      const auto tA = Clock::now();
      ScopedContext ctx_a;
      ASSERT_NE(nullptr, ctx_a.get()) << label << " deal " << d << ": context alloc failed";
      rcA = dds_c_solve_board(ctx_a.get(), &full, -1, 3, 1, &futA);
      ctx_a.destroy();
      t.a += ms_since(tA);
    }
    ASSERT_EQ(RETURN_NO_FAULT, rcA) << label << " deal " << d << ": A solve failed";
    // A full-deal solve always has playable cards; none would mean the solver
    // answered something we cannot compare, not a deal worth skipping.
    ASSERT_GT(futA.cards, 0) << label << " deal " << d << ": A returned no cards";

    // Precompute the post-lead positions (excluded from the timings).
    std::vector<Deal> positions;
    for (const Card lead : top_k_leads(futA, k))
      positions.push_back(make_position(hands, trump, leader, &lead));

    // --- B_cold: a fresh context (cold TT) per lead. ---
    std::vector<int> cold_scores;
    const auto tCold = Clock::now();
    for (const auto& pos : positions) {
      ScopedContext ctx;
      ASSERT_NE(nullptr, ctx.get()) << label << " deal " << d << ": context alloc failed";
      FutureTricks fut{};
      const int rc = dds_c_solve_board(ctx.get(), &pos, -1, 1, 0, &fut);
      ctx.destroy();
      ASSERT_EQ(RETURN_NO_FAULT, rc) << label << " deal " << d << ": B_cold solve failed";
      ASSERT_GT(fut.cards, 0) << label << " deal " << d << ": B_cold returned no cards";
      cold_scores.push_back(fut.score[0]);
    }
    t.b_cold += ms_since(tCold);

    // --- B_warm: ONE context reused across the K leads (warm TT). ---
    std::vector<int> warm_scores;
    {
      const auto tWarm = Clock::now();
      ScopedContext ctx_w;
      ASSERT_NE(nullptr, ctx_w.get()) << label << " deal " << d << ": context alloc failed";
      for (const auto& pos : positions) {
        FutureTricks fut{};
        const int rc = dds_c_solve_board(ctx_w.get(), &pos, -1, 1, 0, &fut);
        ASSERT_EQ(RETURN_NO_FAULT, rc) << label << " deal " << d << ": B_warm solve failed";
        ASSERT_GT(fut.cards, 0) << label << " deal " << d << ": B_warm returned no cards";
        warm_scores.push_back(fut.score[0]);
      }
      ctx_w.destroy();
      t.b_warm += ms_since(tWarm);
    }

    // --- Correctness: warm TT reuse must not change any answer. ---
    ASSERT_EQ(cold_scores, warm_scores)
      << label << " deal " << d << ": warm-TT reuse changed a lead's score";
    ++t.deals;
  }
}

auto report(const char* tag, int k, const Totals& t) -> void
{
  const double a = t.a, bc = t.b_cold, bw = t.b_warm;
  std::printf(
    "  %-10s K=%d over %3d deals:  A(sol=3)=%8.1f ms   "
    "B_cold=%8.1f ms (%.2fx A)   B_warm=%8.1f ms (%.2fx A)   warm/cold=%.2f\n",
    tag, k, t.deals, a, bc, (a > 0 ? bc / a : 0.0),
    bw, (a > 0 ? bw / a : 0.0), (bc > 0 ? bw / bc : 0.0));
}

TEST(WarmTtBenchmark, ContextReuseKeepsTranspositionTableWarm)
{
  // Fixed seed, on purpose: it is what makes this benchmark reproducible.
  // std::mt19937 is specified by the standard, so a given seed yields the same
  // sequence on every platform and library version -- so every run, on every
  // machine, measures the same 100 deals. That matters here because the numbers
  // are meant to be compared across runs (and across changes to the solver): a
  // fresh random sample each time would move the timings for reasons unrelated
  // to the code under test, and would also make any failure of the warm/cold
  // agreement check below impossible to reproduce from the failure output alone.
  std::mt19937 rng(20260717u);
  constexpr int kDeals = 100;
  std::vector<Hands> deals;
  deals.reserve(kDeals);
  for (int d = 0; d < kDeals; ++d) deals.push_back(make_deal(rng));

  // Warm up process-wide one-time init so it doesn't bias the first timer.
  {
    const Deal w = make_position(deals[0], kStrainNT, kWest);
    FutureTricks fut{};
    ScopedContext ctx;
    ASSERT_NE(nullptr, ctx.get());
    (void) dds_c_solve_board(ctx.get(), &w, -1, 3, 1, &fut);
  }

  std::printf("\n[warm-TT benchmark] declarer South; opening leader West; "
              "%d deals x {3N, 4S}\n", kDeals);

  double warm_sum = 0, cold_sum = 0;
  for (const int k : {4, 6}) {
    Totals nt, sp;
    run(deals, kStrainNT,     kWest, k, "3N", nt);
    run(deals, kStrainSpades, kWest, k, "4S", sp);
    if (::testing::Test::HasFatalFailure()) return;
    report("3N", k, nt);
    report("4S", k, sp);
    Totals both;
    both.a = nt.a + sp.a; both.b_cold = nt.b_cold + sp.b_cold;
    both.b_warm = nt.b_warm + sp.b_warm; both.deals = nt.deals + sp.deals;
    report("3N+4S", k, both);
    warm_sum += both.b_warm; cold_sum += both.b_cold;
  }

  // Sanity (not a timing gate): reusing a warm TT must not be materially
  // slower than rebuilding a cold one. The real speed story is in the table.
  EXPECT_LE(warm_sum, cold_sum * 1.25)
    << "context reuse was unexpectedly slower than fresh contexts";
}

}  // namespace
