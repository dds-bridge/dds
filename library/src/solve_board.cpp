/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <chrono>

#include "solve_board.hpp"
#include <solver_if.hpp>
#include <pbn.hpp>
#include <system/memory.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>
#include <utility/debug.h>


ParamType param;

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;

auto solve_all_boards_n(
  Boards const & bds,
  SolvedBoards& solved) -> int;

auto same_board(
  const Boards& bds,
  const unsigned index1,
  const unsigned index2) -> bool;


auto solve_single_common(
  const int thrId,
  const int bno) -> void
{
  FutureTricks fut;

  // Fallback timing: measure per-board elapsed time (ms) even when
  // DDS_SCHEDULER isn't enabled at compile time. This allows the
  // dtest -r/--report option to print per-board timings.
  START_THREAD_TIMER(thrId);
  auto t0 = std::chrono::steady_clock::now();
  int res = SolveBoard(
              param.bop->deals[bno],
              param.bop->target[bno],
              param.bop->solutions[bno],
              param.bop->mode[bno],
              &fut,
              thrId);
  auto t1 = std::chrono::steady_clock::now();
  END_THREAD_TIMER(thrId);

  // Compute elapsed milliseconds and publish to scheduler as a
  // lightweight fallback. Scheduler will ignore or use this value
  // when reporting if full DDS_SCHEDULER timing is not active.
  auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  if (dur < 0) dur = 0;
  scheduler.SetBoardTime(bno, static_cast<int>(dur));

  if (res == 1)
    param.solvedp->solved_board[bno] = fut;
  else
    param.error = res;
}


auto copy_solve_single(const vector<int>& crossrefs) -> void
{
  for (unsigned i = 0; i < crossrefs.size(); i++)
  {
    if (crossrefs[i] == -1)
      continue;

    START_THREAD_TIMER(thrId);
    param.solvedp->solved_board[i] = 
      param.solvedp->solved_board[crossrefs[i]];
    END_THREAD_TIMER(thrId);
  }
}


auto solve_chunk_common(
  const int thrId) -> void
{
  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thrId);
    index = st.number;
    if (index == -1)
      break;

    // This is not a perfect repeat detector, as the hands in
    // a group might have declarers N, S, N, N. Then the second
    // N would not reuse the first N. However, must reuses are
    // reasonably adjacent, and this is just an optimization anyway.

    if (st.repeatOf != -1 &&
        param.bop->deals[index ].first ==
        param.bop->deals[st.repeatOf].first)
    {
      START_THREAD_TIMER(thrId);
      param.solvedp->solved_board[index] = 
        param.solvedp->solved_board[st.repeatOf];
      END_THREAD_TIMER(thrId);
      continue;
    }
    else
    {
      solve_single_common(thrId, index);
    }
  }
}


auto solve_all_boards_n(
  Boards const & bds,
  SolvedBoards& solved) -> int
{
  param.error = 0;

  if (bds.no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  param.bop = &bds;
  param.solvedp = &solved;
  param.no_of_boards = bds.no_of_boards;

  scheduler.RegisterRun(RunMode::DDS_RUN_SOLVE, bds);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solved.solved_board[k].cards = 0;

  START_BLOCK_TIMER;
  
  // Sequential execution: solve each board in order
  // Thread ID 0 is used for all boards (single-threaded)
  for (int bno = 0; bno < bds.no_of_boards; bno++) {
    solve_single_common(0, bno);
    if (param.error != 0)
      return param.error;
  }
  
  END_BLOCK_TIMER;

  solved.no_of_boards = param.no_of_boards;

#ifdef DDS_SCHEDULER 
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
}


/*
 * Solve a single bridge Deal in PBN format.
 *
 * Public API documentation is maintained in the API headers.
 */
int STDCALL SolveBoardPBN(
  DealPBN dlpbn, 
  int target,
  int solutions, 
  int mode, 
  FutureTricks * futp, 
  int thrId)
{
  Deal dl;
  if (convert_from_pbn(dlpbn.remainCards, dl.remainCards) != RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  for (int k = 0; k <= 2; k++)
  {
    dl.currentTrickRank[k] = dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k] = dlpbn.currentTrickSuit[k];
  }
  dl.first = dlpbn.first;
  dl.trump = dlpbn.trump;

  int res = SolveBoard(dl, target, solutions, mode, futp, thrId);
  return res;
}


/**
 * @brief Solve multiple bridge deals in PBN format.
 *
 * Converts each PBN Deal to internal format and solves all Boards.
 *
 * @param bop Pointer to multiple PBN deals
 * @param solvedp Pointer to results for solved Boards
 * @return 1 on success, error code otherwise
 */
int STDCALL SolveAllBoards(
  BoardsPBN const * bop,
  SolvedBoards * solvedp)
{
  Boards bo;
  bo.no_of_boards = bop->no_of_boards;
  if (bo.no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < bop->no_of_boards; k++)
  {
    bo.mode[k] = bop->mode[k];
    bo.solutions[k] = bop->solutions[k];
    bo.target[k] = bop->target[k];
    bo.deals[k].first = bop->deals[k].first;
    bo.deals[k].trump = bop->deals[k].trump;

    for (int i = 0; i <= 2; i++)
    {
      bo.deals[k].currentTrickSuit[i] = bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i] = bop->deals[k].currentTrickRank[i];
    }

    if (convert_from_pbn(bop->deals[k].remainCards, bo.deals[k].remainCards) 
        != 1)
      return RETURN_PBN_FAULT;
  }

  int res = solve_all_boards_n(bo, * solvedp);
  return res;
}


int STDCALL SolveAllBoardsBin(
  Boards const * bop,
  SolvedBoards * solvedp)
{
  return solve_all_boards_n(* bop, * solvedp);
}


int STDCALL SolveAllChunksPBN(
  BoardsPBN const * bop,
  SolvedBoards * solvedp,
  int chunkSize)
{
  // Historical aliases.  Don't use -- they may go away.
  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  return SolveAllBoards(bop, solvedp);
}


int STDCALL SolveAllChunks(
  BoardsPBN const * bop,
  SolvedBoards * solvedp,
  int chunkSize)
{
  // Historical aliases.  Don't use -- they may go away.
  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  return SolveAllBoards(bop, solvedp);
}


int STDCALL SolveAllChunksBin(
  Boards const * bop,
  SolvedBoards * solvedp,
  int chunkSize)
{
  // Historical aliases.  Don't use -- they may go away.
  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  return solve_all_boards_n(* bop, * solvedp);
}


auto detect_solve_duplicates(
  const Boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs) -> void
{
  const unsigned nu = static_cast<unsigned>(bds.no_of_boards);

  uniques.clear();
  crossrefs.resize(nu);

  for (unsigned i = 0; i < nu; i++)
    crossrefs[i] = -1;

  for (unsigned i = 0; i < nu; i++)
  {
    if (crossrefs[i] != -1)
      continue;

    uniques.push_back(static_cast<int>(i));

    for (unsigned index = i+1; index < nu; index++)
    {
      if (same_board(bds, i, index))
        crossrefs[index] = static_cast<int>(i);
    }
  }
}


auto same_board(
  const Boards& bds,
  const unsigned index1,
  const unsigned index2) -> bool
{
  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      if (bds.deals[index1].remainCards[h][s] !=
          bds.deals[index2].remainCards[h][s])
        return false;
    }
  }

  if (bds.mode[index1] != bds.mode[index2])
    return false;
  if (bds.solutions[index1] != bds.solutions[index2])
    return false;
  if (bds.target[index1] != bds.target[index2])
    return false;
  if (bds.deals[index1].first != bds.deals[index2].first)
    return false;
  if (bds.deals[index1].trump != bds.deals[index2].trump)
    return false;

  for (int k = 0; k < 3; k++)
  {
    if (bds.deals[index1].currentTrickSuit[k] != 
        bds.deals[index2].currentTrickSuit[k])
      return false;
    if (bds.deals[index1].currentTrickRank[k] != 
        bds.deals[index2].currentTrickRank[k])
      return false;
  }
  return true;
}

