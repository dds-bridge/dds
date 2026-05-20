/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "calc_tables.hpp"
#include <pbn.hpp>
#include <solve_board.hpp>
#include <api/solve_board.hpp>
#include <solver_if.hpp>
#include <system/memory.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>


ParamType cparam;

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;

// Legacy overloads (create temporary context)
auto calc_all_boards_n(
  Boards * bop,
  SolvedBoards * solvedp) -> int;

// Legacy shim for system/threading infrastructure (not actively used)
auto calc_single_common(
  const int thrId,
  const int bno) -> void;


auto calc_single_common_internal(
  SolverContext& ctx,
  [[maybe_unused]] const int thrId,
  const int bno) -> void
{
  // Solves a single Deal and strain for all four declarers.

  FutureTricks fut{};
  Deal deal = cparam.bop->deals[bno];  // Make a local copy
  deal.first = 0;

  // Use caller-provided SolverContext for DD table calculation.
  // This allows transposition table reuse across multiple table calculations.

  START_THREAD_TIMER(thrId);
  int res = solve_board(
                ctx,
                deal,
                cparam.bop->target[bno],
                cparam.bop->solutions[bno],
                cparam.bop->mode[bno],
                &fut);

  // SH: I'm making a terrible use of the fut structure here.

  if (res == 1)
    cparam.solvedp->solved_board[bno].score[0] = fut.score[0];
  else
    cparam.error = res;

  // Reuse the same SolverContext (including ThreadData and TransTable)
  // for subsequent same-board solves to ensure all declarers on the same
  // board share the same transposition table state, which is important
  // for calculation consistency and fixes a previous consistency bug.
  for (int k = 1; k < DDS_HANDS; k++)
  {
    int hint = (k == 2 ? fut.score[0] : 13 - fut.score[0]);

    deal.first = k; // Next declarer

    res = solve_same_board(ctx, deal, &fut, hint);

    if (res == 1)
      cparam.solvedp->solved_board[bno].score[k] = fut.score[0];
    else
      cparam.error = res;
  }
  END_THREAD_TIMER(thrId);
}

// Legacy shim for system/threading infrastructure (not actively used)
auto calc_single_common(
  const int thrId,
  const int bno) -> void
{
  SolverContext ctx;
  calc_single_common_internal(ctx, thrId, bno);
}


auto copy_calc_single(const vector<int>& crossrefs) -> void
{
  for (unsigned i = 0; i < crossrefs.size(); i++)
  {
    if (crossrefs[i] == -1)
      continue;

    START_THREAD_TIMER(thrId);
    for (int k = 0; k < DDS_HANDS; k++)
      cparam.solvedp->solved_board[i].score[k] = 
        cparam.solvedp->solved_board[ crossrefs[i] ].score[k];
    END_THREAD_TIMER(thrId);
  }
}


auto calc_chunk_common(
  const int thrId) -> void
{
  // Solves each Deal and strain for all four declarers.
  // NOTE: This function is legacy multi-threading infrastructure not actively used
  // in current sequential execution mode.
  
  vector<FutureTricks> fut;
  fut.resize(static_cast<unsigned>(cparam.no_of_boards));

  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thrId);
    index = st.number;
    if (index == -1)
      break;

    if (st.repeatOf != -1)
    {
      START_THREAD_TIMER(thrId);
      for (int k = 0; k < DDS_HANDS; k++)
      {
        cparam.solvedp->solved_board[index].score[k] =
          cparam.solvedp->solved_board[ st.repeatOf ].score[k];
      }
      END_THREAD_TIMER(thrId);
      continue;
    }

    calc_single_common(thrId, index);
  }
}


auto calc_all_boards_n(
  SolverContext& ctx,
  Boards * bop,
  SolvedBoards * solvedp) -> int
{
  cparam.error = 0;

  if (bop->no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  cparam.bop = bop;
  cparam.solvedp = solvedp;
  cparam.no_of_boards = bop->no_of_boards;

  scheduler.RegisterRun(RunMode::DDS_RUN_CALC, * bop);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solved_board[k].cards = 0;

  START_BLOCK_TIMER;
  
  // Sequential execution: calculate each board in order
  // Thread ID 0 is used for all boards (single-threaded)
  for (int bno = 0; bno < bop->no_of_boards; bno++) {
    calc_single_common_internal(ctx, 0, bno);
    if (cparam.error != 0)
      return cparam.error;
  }
  
  END_BLOCK_TIMER;

  solvedp->no_of_boards = cparam.no_of_boards;

#ifdef DDS_SCHEDULER 
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
}

// Legacy overload: creates temporary context
auto calc_all_boards_n(
  Boards * bop,
  SolvedBoards * solvedp) -> int
{
  SolverContext ctx;
  return calc_all_boards_n(ctx, bop, solvedp);
}



int STDCALL CalcDDtable(
  DdTableDeal tableDeal,
  DdTableResults * tablep)
{
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

  int res = calc_all_boards_n(&bo, &solved);
  if (res != 1)
    return res;

  for (int index = 0; index < DDS_STRAINS; index++)
  {
    int strain = bo.deals[index].trump;

    // SH: I'm making a terrible use of the fut structure here.

    for (int first = 0; first < DDS_HANDS; first++)
    {
      tablep->res_table[strain][ rho[first] ] =
        13 - solved.solved_board[index].score[first];
    }
  }
  return RETURN_NO_FAULT;
}


int STDCALL CalcAllTables(
  DdTableDeals const * dealsp,
  int mode,
  int const trumpFilter[5],
  DdTablesRes * resp,
  AllParResults * presp)
{
  /* mode = 0: par calculation, vulnerability None
     mode = 1: par calculation, vulnerability All
     mode = 2: par calculation, vulnerability NS
     mode = 3: par calculation, vulnerability EW
         mode = -1: no par calculation */

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

  int ind = 0;
  int lastIndex = 0;
  resp->no_of_boards = 0;

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
      lastIndex = ind;
      ind++;
    }
  }

  bo.no_of_boards = lastIndex + 1;

  int res = calc_all_boards_n(&bo, &solved);
  if (res != 1)
    return res;

  resp->no_of_boards += 4 * solved.no_of_boards;

  for (int m = 0; m < dealsp->no_of_tables; m++)
  {
    for (int strainIndex = 0; strainIndex < count; strainIndex++)
    {
      int index = m * count + strainIndex;
      int strain = bo.deals[index].trump;

      // SH: I'm making a terrible use of the fut structure here.

      for (int first = 0; first < DDS_HANDS; first++)
      {
        resp->results[m].res_table[strain][ rho[first] ] =
          13 - solved.solved_board[index].score[first];
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


int STDCALL CalcAllTablesPBN(
  DdTableDealsPBN const * dealsp,
  int mode,
  int const trumpFilter[5],
  DdTablesRes * resp,
  AllParResults * presp)
{
  DdTableDeals dls;
  for (int k = 0; k < dealsp->no_of_tables; k++)
    if (convert_from_pbn(dealsp->deals[k].cards, dls.deals[k].cards) != 1)
      return RETURN_PBN_FAULT;

  dls.no_of_tables = dealsp->no_of_tables;

  int res = CalcAllTables(&dls, mode, trumpFilter, resp, presp);
  return res;
}


int STDCALL CalcDDtablePBN(
  DdTableDealPBN tableDealPBN,
  DdTableResults * tablep)
{
  DdTableDeal tableDeal;
  if (convert_from_pbn(tableDealPBN.cards, tableDeal.cards) != 1)
    return RETURN_PBN_FAULT;

  int res = CalcDDtable(tableDeal, tablep);
  return res;
}


void detect_calc_duplicates(
  const Boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs)
{
  // Could save a little bit of time with a dedicated checker that
  // only looks at the cards.
  return detect_solve_duplicates(bds, uniques, crossrefs);
}

