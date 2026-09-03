/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "calc_tables.hpp"
#include <algorithm>
#include <limits>
#include <array>
#include <numeric>
#include <vector>

#include <lookup_tables/lookup_tables.hpp>
#include <pbn.hpp>
#include <table_deal_validate.hpp>
#include <solve_board.hpp>
#include <api/solve_board.hpp>
#include <api/dll.h>
#include <solver_if.hpp>
#include <system/deal_fanout.hpp>
#include <system/memory.hpp>
#include <system/parallel_boards.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>


extern Memory memory;
extern Scheduler scheduler;

auto calc_all_boards_n(
  Boards * bop,
  SolvedBoards * solvedp,
  int max_threads = 0,
  bool difficulty_sort = true) -> int;

// Match SolveBoard's remaining-trick count from remainCards alone.
auto remaining_tricks_from_holdings(
  unsigned int const cards[DDS_HANDS][DDS_SUITS]) -> int
{
  int card_count = 0;
  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
      card_count += count_table[cards[h][s] >> 2];
  }

  if (card_count % 4)
    return ((card_count - 4) >> 2) + 2;
  return ((card_count - 4) >> 2) + 1;
}

auto declarer_tricks_from_leader_score(
  int remaining_tricks,
  int leader_side_score) -> int
{
  return remaining_tricks - leader_side_score;
}

namespace
{

// solve_same_board's null-window reuse assumes a full 13-trick deal.
constexpr int kFullDealRemainingTricks = 13;

auto is_full_thirteen_trick_deal(
  unsigned int const cards[DDS_HANDS][DDS_SUITS]) -> bool
{
  return remaining_tricks_from_holdings(cards) == kFullDealRemainingTricks;
}

}  // namespace


auto calc_single_common_internal(
  SolverContext& ctx,
  Boards const& bds,
  SolvedBoards& solved,
  const int bno) -> int
{
  FutureTricks fut{};
  Deal deal = bds.deals[bno];  // Make a local copy
  deal.first = 0;

  int res = solve_board(
                ctx,
                deal,
                bds.target[bno],
                bds.solutions[bno],
                bds.mode[bno],
                &fut);

  // SH: I'm making a terrible use of the fut structure here.

  if (res == 1)
    solved.solved_board[bno].score[0] = fut.score[0];
  else
    return res;

  // Reuse the same SolverContext (including ThreadData and TransTable)
  // for subsequent same-board solves to ensure all declarers on the same
  // board share the same transposition table state, which is important
  // for calculation consistency and fixes a previous consistency bug.
  const bool reuse_same_board =
    is_full_thirteen_trick_deal(deal.remainCards);
  for (int k = 1; k < DDS_HANDS; k++)
  {
    deal.first = k;
    if (reuse_same_board)
    {
      // Fast path for full deals: null-window reuse with a partner/opponent hint.
      const int hint =
        (k == 2 ? fut.score[0] : kFullDealRemainingTricks - fut.score[0]);
      res = solve_same_board(ctx, deal, &fut, hint);
    }
    else
    {
      // Partial deals: solve_same_board hints/reuse are incorrect; full solve.
      res = solve_board(
        ctx,
        deal,
        bds.target[bno],
        bds.solutions[bno],
        bds.mode[bno],
        &fut);
    }

    if (res == 1)
      solved.solved_board[bno].score[k] = fut.score[0];
    else
      return res;
  }
  return 1;
}


auto calc_all_boards_n(
  SolverContext& ctx,
  Boards * bop,
  SolvedBoards * solvedp) -> int
{
  if (bop->no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solved_board[k].cards = 0;

  START_BLOCK_TIMER;

  for (int bno = 0; bno < bop->no_of_boards; bno++) {
    const int err = calc_single_common_internal(ctx, *bop, *solvedp, bno);
    if (err != 1)
      return err;
  }

  END_BLOCK_TIMER;

  solvedp->no_of_boards = bop->no_of_boards;

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
}

// Legacy overload: parallel across boards, one SolverContext per worker.
auto calc_all_boards_n(
  Boards * bop,
  SolvedBoards * solvedp,
  int max_threads,
  bool difficulty_sort) -> int
{
  const int n = bop->no_of_boards;
  if (n > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solved_board[k].cards = 0;

  START_BLOCK_TIMER;

  const int nthreads = resolve_worker_count(max_threads, n);

  int err = RETURN_NO_FAULT;
  if (nthreads <= 1)
  {
    SolverContext& ctx = dds::internal::worker_solver_context();
    for (int bno = 0; bno < n; ++bno)
    {
      err = calc_single_common_internal(ctx, *bop, *solvedp, bno);
      if (err != RETURN_NO_FAULT)
        break;
    }
  }
  else
  {
    // Dispatch hardest boards first to shorten the parallel tail. This only
    // helps across distinct deals (batch calc); for a single deal every board
    // shares one fanout, so the sort is skipped (it would be a no-op anyway).
    std::vector<int> order;
    if (difficulty_sort)
    {
      std::vector<int> fanout(static_cast<unsigned>(n));
      for (int i = 0; i < n; i++)
        fanout[static_cast<unsigned>(i)] =
          dds::internal::deal_fanout(bop->deals[i]);
      order.resize(static_cast<unsigned>(n));
      std::iota(order.begin(), order.end(), 0);
      std::stable_sort(order.begin(), order.end(),
        [&](const int a, const int b) {
          return fanout[static_cast<unsigned>(a)] > fanout[static_cast<unsigned>(b)];
        });
    }

    err = parallel_all_boards_n(n, nthreads,
      [&](const int worker_id, const int bno) -> int {
        (void)worker_id;
        // Persistent per-thread context: pool worker threads survive across
        // batches, so the TT allocated here is reused for the whole run.
        return calc_single_common_internal(
          dds::internal::worker_solver_context(), *bop, *solvedp, bno);
      },
      order.empty() ? nullptr : &order);
  }

  END_BLOCK_TIMER;

  if (err != RETURN_NO_FAULT)
    return err;

  solvedp->no_of_boards = n;

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
}



int STDCALL CalcDDtableN(
  DdTableDeal tableDeal,
  DdTableResults * tablep,
  int maxThreads)
{
  if (int const check = table_deal_checks(tableDeal); check != RETURN_NO_FAULT)
    return check;

  Deal dl;
  Boards bo;
  SolvedBoards solved;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      dl.remainCards[h][s] = tableDeal.cards[h][s];

  for (int k = 0; k <= 2; k++)
  {
    dl.currentTrickRank[k] = 0;
    dl.currentTrickSuit[k] = 0;
  }

  int ind = 0;
  bo.no_of_boards = DDS_STRAINS;

  for (int tr = DDS_STRAINS-1; tr >= 0; tr--)
  {
    dl.trump = tr;
    bo.deals[ind] = dl;
    bo.target[ind] = -1;
    bo.solutions[ind] = 1;
    bo.mode[ind] = 1;
    ind++;
  }

  // Single deal: all boards share one deal, so hardest-first sorting is a no-op.
  int res = calc_all_boards_n(&bo, &solved, maxThreads, /*difficulty_sort=*/false);
  if (res != 1)
    return res;

  const int tricks = remaining_tricks_from_holdings(tableDeal.cards);
  for (int index = 0; index < DDS_STRAINS; index++)
  {
    int strain = bo.deals[index].trump;

    // SH: I'm making a terrible use of the fut structure here.

    for (int first = 0; first < DDS_HANDS; first++)
    {
      tablep->res_table[strain][ rho[first] ] =
        declarer_tricks_from_leader_score(
          tricks, solved.solved_board[index].score[first]);
    }
  }
  return RETURN_NO_FAULT;
}


int STDCALL CalcDDtable(
  DdTableDeal tableDeal,
  DdTableResults * tablep)
{
  return CalcDDtableN(tableDeal, tablep, 0);
}


int STDCALL CalcAllTablesN(
  DdTableDeals const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  DdTablesRes * resp,
  AllParResults * presp,
  int maxThreads)
{
  /* mode = 0: par calculation, vulnerability None
     mode = 1: par calculation, vulnerability All
     mode = 2: par calculation, vulnerability NS
     mode = 3: par calculation, vulnerability EW
         mode = -1: no par calculation */

  // dealsp->deals is a fixed MAXNOOFTABLES * DDS_STRAINS array, and the
  // capacity check below multiplies by count. Bound no_of_tables first, so
  // that multiply cannot overflow signed int and wrap past the check, and so
  // the per-deal loops cannot read past the array.
  if (dealsp == nullptr)
    return RETURN_UNKNOWN_FAULT;

  if (dealsp->no_of_tables < 0 ||
      dealsp->no_of_tables > MAXNOOFTABLES * DDS_STRAINS)
    return RETURN_TOO_MANY_TABLES;

  Boards bo;
  SolvedBoards solved;
  int count = 0;
  bool okey = false;

  for (int k = 0; k < DDS_STRAINS; k++)
  {
    if (!trumpFilter[k])
    {
      okey = true;
      count++;
    }
  }

  if (!okey)
    return RETURN_NO_SUIT;

  if (count * dealsp->no_of_tables > MAXNOOFTABLES * DDS_STRAINS)
    return RETURN_TOO_MANY_TABLES;

  for (int m = 0; m < dealsp->no_of_tables; m++)
  {
    int const check = table_deal_checks(dealsp->deals[m]);
    if (check != RETURN_NO_FAULT)
      return check;
  }

  int ind = 0;
  resp->no_of_boards = 0;

  // With no deals the loop below writes no boards, and bo is an uninitialised
  // local -- solving a board from it reads indeterminate values. Return early,
  // matching CalcAllTablesX().
  if (dealsp->no_of_tables == 0)
    return RETURN_NO_FAULT;

  for (int m = 0; m < dealsp->no_of_tables; m++)
  {
    for (int tr = DDS_STRAINS-1; tr >= 0; tr--)
    {
      if (trumpFilter[tr])
        continue;

      for (int h = 0; h < DDS_HANDS; h++)
        for (int s = 0; s < DDS_SUITS; s++)
          bo.deals[ind].remainCards[h][s] =
            dealsp->deals[m].cards[h][s];

      bo.deals[ind].trump = tr;

      for (int k = 0; k <= 2; k++)
      {
        bo.deals[ind].currentTrickRank[k] = 0;
        bo.deals[ind].currentTrickSuit[k] = 0;
      }

      bo.target[ind] = -1;
      bo.solutions[ind] = 1;
      bo.mode[ind] = 1;
      ind++;
    }
  }

  // ind counts the boards actually written; deriving the count from a
  // last-index variable initialised to 0 claimed one board even when none
  // had been filled in.
  bo.no_of_boards = ind;

  int res = calc_all_boards_n(&bo, &solved, maxThreads);
  if (res != 1)
    return res;

  resp->no_of_boards += 4 * solved.no_of_boards;

  for (int m = 0; m < dealsp->no_of_tables; m++)
  {
    const int tricks = remaining_tricks_from_holdings(dealsp->deals[m].cards);
    for (int strainIndex = 0; strainIndex < count; strainIndex++)
    {
      int index = m * count + strainIndex;
      int strain = bo.deals[index].trump;

      // SH: I'm making a terrible use of the fut structure here.

      for (int first = 0; first < DDS_HANDS; first++)
      {
        resp->results[m].res_table[strain][ rho[first] ] =
          declarer_tricks_from_leader_score(
            tricks, solved.solved_board[index].score[first]);
      }
    }
  }

  if ((mode > -1) && (mode < 4) && (count == 5))
  {
    /* Calculate par */
    for (int k = 0; k < dealsp->no_of_tables; k++)
    {
      res = Par(&(resp->results[k]), &(presp->par_results[k]), mode);
      /* vulnerable 0: None 1: Both 2: NS 3: EW */
      if (res != 1)
        return res;
    }
  }
  return RETURN_NO_FAULT;
}


int STDCALL CalcAllTables(
  DdTableDeals const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  DdTablesRes * resp,
  AllParResults * presp)
{
  return CalcAllTablesN(dealsp, mode, trumpFilter, resp, presp, 0);
}


int STDCALL CalcAllTablesPBNN(
  DdTableDealsPBN const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  DdTablesRes * resp,
  AllParResults * presp,
  int maxThreads)
{
  // dls.deals and dealsp->deals both hold MAXNOOFTABLES * DDS_STRAINS
  // entries. Bound the count before the conversion loop: unchecked, this
  // wrote past the fixed-size local.
  if (dealsp == nullptr)
    return RETURN_UNKNOWN_FAULT;

  if (dealsp->no_of_tables < 0 ||
      dealsp->no_of_tables > MAXNOOFTABLES * DDS_STRAINS)
    return RETURN_TOO_MANY_TABLES;

  DdTableDeals dls;
  for (int k = 0; k < dealsp->no_of_tables; k++)
    if (convert_from_pbn(dealsp->deals[k].cards, dls.deals[k].cards) != 1)
      return RETURN_PBN_FAULT;

  dls.no_of_tables = dealsp->no_of_tables;

  int res = CalcAllTablesN(&dls, mode, trumpFilter, resp, presp, maxThreads);
  return res;
}


int STDCALL CalcAllTablesPBN(
  DdTableDealsPBN const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  DdTablesRes * resp,
  AllParResults * presp)
{
  return CalcAllTablesPBNN(dealsp, mode, trumpFilter, resp, presp, 0);
}


namespace
{

// Solve one strain board (all four declarers) into scores[DDS_HANDS].
auto calc_single_deal_scores(
  SolverContext& ctx,
  Deal deal,
  const int target,
  const int solutions,
  const int mode,
  int scores[DDS_HANDS]) -> int
{
  FutureTricks fut{};
  deal.first = 0;
  int res = solve_board(ctx, deal, target, solutions, mode, &fut);
  if (res != RETURN_NO_FAULT)
    return res;
  scores[0] = fut.score[0];

  const bool reuse_same_board =
    is_full_thirteen_trick_deal(deal.remainCards);
  for (int k = 1; k < DDS_HANDS; k++)
  {
    deal.first = k;
    if (reuse_same_board)
    {
      const int hint =
        (k == 2 ? fut.score[0] : kFullDealRemainingTricks - fut.score[0]);
      res = solve_same_board(ctx, deal, &fut, hint);
    }
    else
    {
      // Partial deals: solve_same_board is wrong; use a full solve per leader.
      res = solve_board(ctx, deal, target, solutions, mode, &fut);
    }
    if (res != RETURN_NO_FAULT)
      return res;
    scores[k] = fut.score[0];
  }
  return RETURN_NO_FAULT;
}

}  // namespace


namespace
{

/* The *X entry points accept an arbitrary deal count by design, so the only
   cheap guard available is the board-count product. Run it before any
   O(numDeals) work -- allocating, converting or validating a count that is
   already guaranteed to be rejected is wasted effort and, for the PBN
   variant, a memory-exhaustion path. Also reports how many strains survive
   the filter, which the caller needs anyway. */
auto batch_count_preflight(
  const int numDeals,
  int const trumpFilter[DDS_STRAINS],
  int& included) -> int
{
  included = 0;
  for (int k = 0; k < DDS_STRAINS; k++)
    if (!trumpFilter[k])
      included++;

  if (included == 0)
    return RETURN_NO_SUIT;

  if (numDeals > std::numeric_limits<int>::max() / included)
    return RETURN_TOO_MANY_TABLES;

  return RETURN_NO_FAULT;
}

}  // namespace


int STDCALL CalcAllTablesX(
  int numDeals,
  DdTableDeal const * deals,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  DdTableResults * results,
  ParResults * par,
  int maxThreads)
{
  // C ABI: exceptions must not unwind into a foreign caller (UB). Heap
  // allocations below (and parallel_all_boards_n) may throw; map any throw
  // to RETURN_UNKNOWN_FAULT.
  try
  {
    if (numDeals < 0)
      return RETURN_TOO_MANY_TABLES;
    if (numDeals == 0)
      return RETURN_NO_FAULT;
    if (deals == nullptr || results == nullptr || trumpFilter == nullptr)
      return RETURN_UNKNOWN_FAULT;

    int included = 0;
    if (int const check = batch_count_preflight(numDeals, trumpFilter, included);
        check != RETURN_NO_FAULT)
      return check;

    const bool want_par = (mode > -1) && (mode < 4) && (included == DDS_STRAINS);
    if (want_par && par == nullptr)
      return RETURN_UNKNOWN_FAULT;

    // This path builds its board list directly rather than going through
    // CalcDDtableN, so it needs the same deal validation.
    for (int m = 0; m < numDeals; m++)
    {
      int const check = table_deal_checks(deals[m]);
      if (check != RETURN_NO_FAULT)
        return check;
    }

    // Expand every deal×included-strain into one board list and solve in a
    // single parallel_all_boards_n job (heap-backed). This is the ddss-style
    // large-batch shape; legacy CalcAllTablesN remains capped at MAXNOOFTABLES.
    // Overflow already ruled out by batch_count_preflight() above.
    const int nboards = numDeals * included;
    std::vector<Deal> boards(static_cast<unsigned>(nboards));
    std::vector<std::array<int, DDS_HANDS>> scores(static_cast<unsigned>(nboards));

    int ind = 0;
    for (int m = 0; m < numDeals; m++)
    {
      for (int tr = DDS_STRAINS - 1; tr >= 0; tr--)
      {
        if (trumpFilter[tr])
          continue;

        Deal& dl = boards[static_cast<unsigned>(ind)];
        for (int h = 0; h < DDS_HANDS; h++)
          for (int s = 0; s < DDS_SUITS; s++)
            dl.remainCards[h][s] = deals[m].cards[h][s];
        dl.trump = tr;
        dl.first = 0;
        for (int k = 0; k <= 2; k++)
        {
          dl.currentTrickRank[k] = 0;
          dl.currentTrickSuit[k] = 0;
        }
        ind++;
      }
    }

    const int nthreads = resolve_worker_count(maxThreads, nboards);

    // Hardest-first dispatch only helps with multiple workers (same idea as
    // calc_all_boards_n). Skip fanout+sort on the single-thread path so large
    // single-worker batches avoid O(n log n) overhead.
    std::vector<int> order;
    if (nthreads > 1)
    {
      order.resize(static_cast<unsigned>(nboards));
      std::iota(order.begin(), order.end(), 0);
      std::vector<int> fanout(static_cast<unsigned>(nboards));
      for (int i = 0; i < nboards; i++)
        fanout[static_cast<unsigned>(i)] =
          dds::internal::deal_fanout(boards[static_cast<unsigned>(i)]);
      std::stable_sort(order.begin(), order.end(),
        [&](const int a, const int b) {
          return fanout[static_cast<unsigned>(a)] > fanout[static_cast<unsigned>(b)];
        });
    }

    const int err = parallel_all_boards_n(nboards, nthreads,
      [&](const int worker_id, const int bno) -> int {
        (void)worker_id;
        return calc_single_deal_scores(
          dds::internal::worker_solver_context(),
          boards[static_cast<unsigned>(bno)],
          -1, 1, 1,
          scores[static_cast<unsigned>(bno)].data());
      },
      order.empty() ? nullptr : &order);
    if (err != RETURN_NO_FAULT)
      return err;

    for (int m = 0; m < numDeals; m++)
    {
      const int tricks = remaining_tricks_from_holdings(deals[m].cards);
      for (int strainIndex = 0; strainIndex < included; strainIndex++)
      {
        const int index = m * included + strainIndex;
        const int strain = boards[static_cast<unsigned>(index)].trump;
        for (int first = 0; first < DDS_HANDS; first++)
        {
          results[m].res_table[strain][rho[first]] =
            declarer_tricks_from_leader_score(
              tricks,
              scores[static_cast<unsigned>(index)][static_cast<unsigned>(first)]);
        }
      }
    }

    if (want_par)
    {
      for (int k = 0; k < numDeals; k++)
      {
        const int res = Par(&results[k], &par[k], mode);
        if (res != RETURN_NO_FAULT)
          return res;
      }
    }

    return RETURN_NO_FAULT;
  }
  catch (...)
  {
    return RETURN_UNKNOWN_FAULT;
  }
}


int STDCALL CalcAllTablesPBNX(
  int numDeals,
  DdTableDealPBN const * deals,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  DdTableResults * results,
  ParResults * par,
  int maxThreads)
{
  // C ABI: same catch-all contract as CalcAllTablesX / dds_c_api.cpp.
  try
  {
    if (numDeals < 0)
      return RETURN_TOO_MANY_TABLES;
    if (numDeals == 0)
      return RETURN_NO_FAULT;
    if (deals == nullptr || results == nullptr || trumpFilter == nullptr)
      return RETURN_UNKNOWN_FAULT;

    // Share CalcAllTablesX's preflight so a count that cannot succeed is
    // rejected before this allocates and converts numDeals records.
    int included = 0;
    if (int const check = batch_count_preflight(numDeals, trumpFilter, included);
        check != RETURN_NO_FAULT)
      return check;

    std::vector<DdTableDeal> binary(static_cast<unsigned>(numDeals));
    for (int i = 0; i < numDeals; ++i)
    {
      if (convert_from_pbn(deals[i].cards, binary[static_cast<unsigned>(i)].cards) != 1)
        return RETURN_PBN_FAULT;
    }

    return CalcAllTablesX(
      numDeals, binary.data(), mode, trumpFilter, results, par, maxThreads);
  }
  catch (...)
  {
    return RETURN_UNKNOWN_FAULT;
  }
}


int STDCALL CalcDDtablePBNN(
  DdTableDealPBN tableDealPBN,
  DdTableResults * tablep,
  int maxThreads)
{
  DdTableDeal tableDeal;
  if (convert_from_pbn(tableDealPBN.cards, tableDeal.cards) != 1)
    return RETURN_PBN_FAULT;

  int res = CalcDDtableN(tableDeal, tablep, maxThreads);
  return res;
}


int STDCALL CalcDDtablePBN(
  DdTableDealPBN tableDealPBN,
  DdTableResults * tablep)
{
  return CalcDDtablePBNN(tableDealPBN, tablep, 0);
}


void detect_calc_duplicates(
  const Boards& bds,
  std::vector<int>& uniques,
  std::vector<int>& crossrefs)
{
  // Could save a little bit of time with a dedicated checker that
  // only looks at the cards.
  return detect_solve_duplicates(bds, uniques, crossrefs);
}
