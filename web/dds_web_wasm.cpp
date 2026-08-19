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
auto web_solver_config() -> SolverConfig
{
  return SolverConfig{
      .tt_kind_ = TTKind::Small,
      .tt_mem_default_mb_ = THREADMEM_SMALL_DEF_MB,
      .tt_mem_maximum_mb_ = THREADMEM_SMALL_MAX_MB,
  };
}

// Separate contexts so CalcDDtable worker pools cannot disturb SolveBoard
// (and vice versa) when the UI auto-fills the table then analyzes leads.
auto web_table_context() -> SolverContext&
{
  static SolverContext ctx(web_solver_config());
  return ctx;
}

auto web_leads_context() -> SolverContext&
{
  static SolverContext ctx(web_solver_config());
  return ctx;
}

// Caller buffer is out_leads[0] count plus at most 13 (suit,rank,score) triples.
constexpr int kMaxExpandedLeads = 13;

// Write one expanded lead at out_leads index `slot` (0-based among triples).
void write_lead_triple(
    int* out_leads, int slot, int suit, int rank, int score)
{
  out_leads[1 + 3 * slot] = suit;
  out_leads[1 + 3 * slot + 1] = rank;
  out_leads[1 + 3 * slot + 2] = score;
}

// True when Holding-encoded `equals` lists `rank` among lower equivalents.
// Encoding (dll-description / equals_to_string): equals stores sequence << 2,
// so rank r is present iff bit (r - 2) is set after >> 2.
auto equals_lists_rank(int equals_holding, int rank) -> bool
{
  if (rank < 2 || rank > 14) {
    return false;
  }
  const unsigned bits = static_cast<unsigned>(equals_holding) >> 2;
  return (bits & (1u << (rank - 2))) != 0;
}

// FutureTricks packs leads: each of fut.cards is the highest card of an
// equivalence group, with fut.equals[i] naming *lower* ranks that score the
// same (not a second copy of fut.rank[i]). Expand to one triple per physical
// card so the UI can badge every pip. Cap at 13 (one full hand on lead).
void write_expanded_leads(const FutureTricks& fut, int* out_leads)
{
  int n = 0;
  for (int i = 0; i < fut.cards && n < kMaxExpandedLeads; ++i) {
    const int suit = fut.suit[i];
    const int score = fut.score[i];
    const int representative = fut.rank[i];

    write_lead_triple(out_leads, n, suit, representative, score);
    ++n;

    // Only ranks strictly below the representative are valid equals.
    for (int rank = 2; rank < representative && n < kMaxExpandedLeads; ++rank) {
      if (!equals_lists_rank(fut.equals[i], rank)) {
        continue;
      }
      write_lead_triple(out_leads, n, suit, rank, score);
      ++n;
    }
  }
  out_leads[0] = n;
}
}  // namespace

#if !defined(__EMSCRIPTEN__)
// Native unit-test hook for Holding-encoded equals expansion.
extern "C" void dds_web_test_write_expanded_leads(
    const FutureTricks* fut, int* out_leads)
{
  if (fut == nullptr || out_leads == nullptr) {
    return;
  }
  write_expanded_leads(*fut, out_leads);
}

// which: 0 = table context, 1 = leads context.
extern "C" void dds_web_test_context_tt_config(
    int which, int* kind_is_small, int* def_mb, int* max_mb)
{
  SolverContext& ctx =
      (which == 0) ? web_table_context() : web_leads_context();
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
auto dds_web_calc_table(const char* pbn, int* out_table) -> int
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

  SolverContext& ctx = web_table_context();
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
// out_leads[0] = n; then n triples (suit, rank, score) with FutureTricks
// equals bitmasks expanded to one triple per physical card.
// suit 0..3 = S,H,D,C; rank 2..14; score = tricks for the side on lead.
// n is capped at 13. Caller must provide at least 1 + 13*3 ints.
EMSCRIPTEN_KEEPALIVE
auto dds_web_solve_leads(
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

  SolverContext& ctx = web_leads_context();
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

#if !defined(__EMSCRIPTEN__) && !defined(DDS_WEB_WASM_NO_MAIN)
auto main() -> int
{
  return 0;
}
#endif
