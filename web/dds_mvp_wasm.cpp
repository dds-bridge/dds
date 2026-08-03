/*
   Browser entry point for CalcDDtablePBN and SolveBoard lead analysis.

   Copyright 2020-2026 Adam Wildavsky
   Use of this source code is governed by the MIT license.
*/

#include <cstring>

#include <api/PBN.h>
#include <api/calc_dd_table.hpp>
#include <api/solve_board.hpp>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

namespace {
// Separate contexts so CalcDDtable worker pools cannot disturb SolveBoard
// (and vice versa) when the UI auto-fills the table then analyzes leads.
auto mvp_table_context() -> SolverContext& {
  static SolverContext ctx;
  return ctx;
}

auto mvp_leads_context() -> SolverContext& {
  static SolverContext ctx;
  return ctx;
}

void append_lead_card(int* out_leads, int& n, int suit, int rank, int score)
{
  out_leads[1 + 3 * n] = suit;
  out_leads[1 + 3 * n + 1] = rank;
  out_leads[1 + 3 * n + 2] = score;
  ++n;
}

void write_expanded_leads(const FutureTricks& fut, int* out_leads)
{
  int n = 0;
  for (int i = 0; i < fut.cards; ++i) {
    append_lead_card(out_leads, n, fut.suit[i], fut.rank[i], fut.score[i]);
    // equals uses Holding encoding: bit r set means rank r is equivalent.
    for (int rank = 2; rank <= 14; ++rank) {
      if ((fut.equals[i] & (1 << rank)) != 0) {
        append_lead_card(out_leads, n, fut.suit[i], rank, fut.score[i]);
      }
    }
  }
  out_leads[0] = n;
}
}  // namespace

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
// Caller must provide at least 1 + 13*3 ints.
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
