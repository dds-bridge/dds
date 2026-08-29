/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <chrono>

#include "solve_board.hpp"
#include <api/solve_board.hpp>
#include <api/dll.h>
#include <solver_if.hpp>
#include <pbn.hpp>
#include <system/memory.hpp>
#include <system/parallel_boards.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>
#include <utility/debug.h>
#include <vector>


extern Scheduler scheduler;

auto same_board(
  const Boards& bds,
  const unsigned index1,
  const unsigned index2) -> bool;


static auto boards_from_pbn(
  BoardsPBN const& bop,
  Boards& bo) -> int
{
  bo.no_of_boards = bop.no_of_boards;
  if (bo.no_of_boards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < bop.no_of_boards; k++)
  {
    bo.mode[k] = bop.mode[k];
    bo.solutions[k] = bop.solutions[k];
    bo.target[k] = bop.target[k];
    bo.deals[k].first = bop.deals[k].first;
    bo.deals[k].trump = bop.deals[k].trump;

    for (int i = 0; i <= 2; i++)
    {
      bo.deals[k].currentTrickSuit[i] = bop.deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i] = bop.deals[k].currentTrickRank[i];
    }

    if (convert_from_pbn(bop.deals[k].remainCards, bo.deals[k].remainCards)
        != RETURN_NO_FAULT)
      return RETURN_PBN_FAULT;
  }

  return RETURN_NO_FAULT;
}


auto solve_all_boards_n(
  Boards const& bds,
  SolvedBoards& solved,
  int max_threads) -> int
{
  const int n = bds.no_of_boards;
  if (n > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solved.solved_board[k].cards = 0;

  scheduler.RegisterRun(RunMode::DDS_RUN_SOLVE, bds);

  START_BLOCK_TIMER;

  const int err = parallel_all_boards_n(n, max_threads,
    [&](const int worker_id, const int bno) -> int {
      (void)worker_id;

      FutureTricks fut;
      const auto t0 = std::chrono::steady_clock::now();
      // Persistent per-thread context: reuses the worker's TT across boards
      // and consecutive batch calls instead of allocating one per board.
      const int res = solve_board(
        dds::internal::worker_solver_context(),
        bds.deals[bno], bds.target[bno], bds.solutions[bno],
        bds.mode[bno], &fut);
      auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t0).count();
      scheduler.SetBoardTime(bno, dur);

      if (res == RETURN_NO_FAULT)
        solved.solved_board[bno] = fut;
      return res;
    });

  END_BLOCK_TIMER;

  if (err != RETURN_NO_FAULT)
    return err;

  solved.no_of_boards = n;

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
}


auto solve_all_boards_pbn_n(
  BoardsPBN const& bop,
  SolvedBoards& solved,
  const int max_threads) -> int
{
  Boards bo;
  const int rc = boards_from_pbn(bop, bo);
  if (rc != RETURN_NO_FAULT)
    return rc;
  return solve_all_boards_n(bo, solved, max_threads);
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
int STDCALL SolveAllBoardsN(
  BoardsPBN const * bop,
  SolvedBoards * solvedp,
  int maxThreads)
{
  return solve_all_boards_pbn_n(* bop, * solvedp, maxThreads);
}


int STDCALL SolveAllBoards(
  BoardsPBN const * bop,
  SolvedBoards * solvedp)
{
  return SolveAllBoardsN(bop, solvedp, 0);
}


int STDCALL SolveAllBoardsBinN(
  Boards const * bop,
  SolvedBoards * solvedp,
  int maxThreads)
{
  return solve_all_boards_n(* bop, * solvedp, maxThreads);
}


int STDCALL SolveAllBoardsBin(
  Boards const * bop,
  SolvedBoards * solvedp)
{
  return SolveAllBoardsBinN(bop, solvedp, 0);
}


int STDCALL SolveAllBoardsSeq(
  BoardsPBN const * bop,
  SolvedBoards * solvedp)
{
  Boards bo;
  const int rc = boards_from_pbn(*bop, bo);
  if (rc != RETURN_NO_FAULT)
    return rc;
  return solve_all_boards_n_seq(bo, * solvedp);
}


int STDCALL SolveAllBoardsBinSeq(
  Boards const * bop,
  SolvedBoards * solvedp)
{
  return solve_all_boards_n_seq(* bop, * solvedp);
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


auto solve_all_boards_n_seq(
  Boards const& bds,
  SolvedBoards& solved) -> int
{
  const int n = bds.no_of_boards;
  if (n > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solved.solved_board[k].cards = 0;

  scheduler.RegisterRun(RunMode::DDS_RUN_SOLVE, bds);

  int error = 0;

  START_BLOCK_TIMER;
  for (int bno = 0; bno < n && error == 0; bno++) {
    FutureTricks fut;
    const auto t0 = std::chrono::steady_clock::now();
    const int res = solve_board(
      dds::internal::worker_solver_context(),
      bds.deals[bno], bds.target[bno], bds.solutions[bno],
      bds.mode[bno], &fut);
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - t0).count();
    scheduler.SetBoardTime(bno, dur);

    if (res == 1)
      solved.solved_board[bno] = fut;
    else
      error = res;
  }
  END_BLOCK_TIMER;

  if (error != 0)
    return error;

  solved.no_of_boards = n;

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  return RETURN_NO_FAULT;
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

