/*
   Browser entry point for CalcDDtablePBN and SolveBoard lead analysis.

   Copyright 2020-2026 Adam Wildavsky
   Use of this source code is governed by the MIT license.
*/

#include <cstring>

#include <api/calc_dd_table.hpp>
#include <api/dds.h>
#include <api/PBN.h>
#include <api/solve_board.hpp>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {
// Cap browser TT heaps: two session contexts at Large defaults would be ~190MB.
auto mvp_solver_config() -> SolverConfig
{
  return SolverConfig{
      .tt_kind_ = TTKind::Small,
      .tt_mem_default_mb_ = THREADMEM_SMALL_DEF_MB,
      .tt_mem_maximum_mb_ = THREADMEM_SMALL_MAX_MB,
  };
}

// Separate contexts so CalcDDtable worker pools cannot disturb SolveBoard
// (and vice versa) when the UI auto-fills the table then analyzes leads.
auto mvp_table_context() -> SolverContext& {
  static SolverContext ctx(mvp_solver_config());
  return ctx;
}

auto mvp_leads_context() -> SolverContext& {
  static SolverContext ctx(mvp_solver_config());
  return ctx;
}

// Caller buffer is out_leads[0] count plus at most 13 (suit,rank,score) triples.
constexpr int kMaxExpandedLeads = 13;

void append_lead_card(
    int* out_leads, int& n, int capacity, int suit, int rank, int score)
{
  if (n >= capacity) {
    return;
  }
  out_leads[1 + 3 * n] = suit;
  out_leads[1 + 3 * n + 1] = rank;
  out_leads[1 + 3 * n + 2] = score;
  ++n;
}

void write_expanded_leads(const FutureTricks& fut, int* out_leads)
{
  int n = 0;
  for (int i = 0; i < fut.cards; ++i) {
    if (n >= kMaxExpandedLeads) {
      break;
    }
    append_lead_card(
        out_leads, n, kMaxExpandedLeads, fut.suit[i], fut.rank[i], fut.score[i]);
    // Holding encoding (see equals_to_string): equals is sequence << 2, so
    // rank r is present when bit (r - 2) is set in (equals >> 2).
    const int equals_ranks = fut.equals[i] >> 2;
    for (int rank = 2; rank <= 14; ++rank) {
      if (n >= kMaxExpandedLeads) {
        break;
      }
      if ((equals_ranks & (1 << (rank - 2))) != 0) {
        append_lead_card(
            out_leads, n, kMaxExpandedLeads, fut.suit[i], rank, fut.score[i]);
      }
    }
  }
  out_leads[0] = n;
}
}  // namespace

#if !defined(__EMSCRIPTEN__)
// Native unit-test hook for Holding-encoded equals expansion.
extern "C" void dds_mvp_test_write_expanded_leads(
    const FutureTricks* fut, int* out_leads)
{
  if (fut == nullptr || out_leads == nullptr) {
    return;
  }
  write_expanded_leads(*fut, out_leads);
}

// which: 0 = table context, 1 = leads context.
extern "C" void dds_mvp_test_mvp_context_tt_config(
    int which, int* kind_is_small, int* def_mb, int* max_mb)
{
  SolverContext& ctx =
      (which == 0) ? mvp_table_context() : mvp_leads_context();
  const SolverConfig& cfg = ctx.config();
  if (kind_is_small != nullptr) {
    *kind_is_small = (cfg.tt_kind_ == TTKind::Small) ? 1 : 0;
  }
  if (def_mb != nullptr) {
    *def_mb = cfg.tt_mem_default_mb_;
  }
  if (max_mb != nullptr) {
    *max_mb = cfg.tt_mem_maximum_mb_;
  }
}
#endif

extern "C" {

// Fills out_table[20] with res_table[strain][hand] (strain 0..4 = S,H,D,C,N).
// Returns RETURN_NO_FAULT (1) on success, or a DDS error code.
EMSCRIPTEN_KEEPALIVE
auto dds_mvp_calc_table(const char* pbn, int* out_table) -> int
{
  if (pbn == nullptr || out_table == nullptr) {
    return RETURN_UNKNOWN_FAULT;
  }

  DdTableDealPBN deal{};
  const size_t pbn_len = std::strlen(pbn);
  if (pbn_len >= sizeof(deal.cards)) {
    return RETURN_PBN_FAULT;
  }
  std::memcpy(deal.cards, pbn, pbn_len + 1);

  SolverContext& ctx = mvp_table_context();
  ctx.reset_for_solve();   // recycle TT memory pool + search bookkeeping
                            // between deals; keeps the underlying allocation

  DdTableResults table{};
  const int res = calc_dd_table_pbn(ctx, deal, &table);
  if (res != RETURN_NO_FAULT) {
    return res;
  }

  int k = 0;
  for (int strain = 0; strain < DDS_STRAINS; ++strain) {
    for (int hand = 0; hand < DDS_HANDS; ++hand) {
      out_table[k++] = table.res_table[strain][hand];
    }
  }
  return RETURN_NO_FAULT;
}

// Solves all opening leads from `first` in `trump`.
// out_leads[0] = n; then n triples (suit, rank, score) with equals expanded.
// suit 0..3 = S,H,D,C; rank 2..14; score = tricks for the side on lead.
// n is capped at 13. Caller must provide at least 1 + 13*3 ints.
EMSCRIPTEN_KEEPALIVE
auto dds_mvp_solve_leads(
    const char* pbn, int trump, int first, int* out_leads) -> int
{
  if (pbn == nullptr || out_leads == nullptr) {
    return RETURN_UNKNOWN_FAULT;
  }
  if (trump < 0 || trump > 4 || first < 0 || first > 3) {
    return RETURN_UNKNOWN_FAULT;
  }

  Deal dl{};
  dl.trump = trump;
  dl.first = first;
  dl.currentTrickSuit[0] = 0;
  dl.currentTrickSuit[1] = 0;
  dl.currentTrickSuit[2] = 0;
  dl.currentTrickRank[0] = 0;
  dl.currentTrickRank[1] = 0;
  dl.currentTrickRank[2] = 0;

  if (convert_from_pbn(pbn, dl.remainCards) != RETURN_NO_FAULT) {
    return RETURN_PBN_FAULT;
  }

  SolverContext& ctx = mvp_leads_context();
  ctx.reset_for_solve();

  FutureTricks fut{};
  const int res = solve_board(ctx, dl, /*target=*/-1, /*solutions=*/3,
                              /*mode=*/0, &fut);
  if (res != RETURN_NO_FAULT) {
    return res;
  }

  write_expanded_leads(fut, out_leads);
  return RETURN_NO_FAULT;
}

}  // extern "C"

#if !defined(__EMSCRIPTEN__) && !defined(DDS_MVP_WASM_NO_MAIN)
auto main() -> int
{
  return 0;
}
#endif
