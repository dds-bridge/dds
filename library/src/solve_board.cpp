/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "solve_board.hpp"
#include <solver_if.hpp>
#include <pbn.hpp>
#include <solver_context/worker_context_pool.hpp>
#include <system/scheduler.hpp>
#include <system/system.hpp>
#include <utility/debug.h>


extern Scheduler scheduler;
extern System sysdep;

namespace {

struct SolveRunParam
{
  Boards const* bds = nullptr;
  SolvedBoards* solved = nullptr;
  std::atomic<int> error{RETURN_NO_FAULT};
};

void solve_chunk_common(const int thr_id, SolveRunParam& param)
{
  while (true)
  {
    const schedType st = scheduler.GetNumber(thr_id);
    const int index = st.number;
    if (index == -1)
      break;

    if (st.repeatOf != -1 &&
        param.bds->deals[index].first == param.bds->deals[st.repeatOf].first)
    {
      START_THREAD_TIMER(thr_id);
      param.solved->solved_board[index] =
        param.solved->solved_board[st.repeatOf];
      END_THREAD_TIMER(thr_id);
      continue;
    }

    FutureTricks fut;
    const auto t0 = std::chrono::steady_clock::now();
    START_THREAD_TIMER(thr_id);
    const int res = SolveBoard(
      param.bds->deals[index],
      param.bds->target[index],
      param.bds->solutions[index],
      param.bds->mode[index],
      &fut,
      thr_id);
    END_THREAD_TIMER(thr_id);

    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - t0).count();
    if (dur < 0)
      dur = 0;
    scheduler.SetBoardTime(index, static_cast<int>(dur));

    if (res == RETURN_NO_FAULT)
      param.solved->solved_board[index] = fut;
    else
    {
      int expected = RETURN_NO_FAULT;
      param.error.compare_exchange_strong(
        expected, res, std::memory_order_relaxed);
      break;
    }
  }
}

auto run_solve_threads(SolveRunParam& param) -> int
{
  const int num_threads = sysdep.get_num_threads();
  ensure_worker_contexts(num_threads);

  if (num_threads <= 1)
  {
    solve_chunk_common(0, param);
    return RETURN_NO_FAULT;
  }

  std::vector<std::thread> threads;
  threads.reserve(static_cast<unsigned>(num_threads));
  try
  {
    for (int k = 0; k < num_threads; ++k)
      threads.emplace_back(solve_chunk_common, k, std::ref(param));
  }
  catch (...)
  {
    for (auto& th : threads)
    {
      if (th.joinable())
        th.join();
    }
    throw;
  }

  for (auto& th : threads)
    th.join();

  return RETURN_NO_FAULT;
}

} // namespace

auto same_board(
  const Boards& bds,
  const unsigned index1,
  const unsigned index2) -> bool;


auto solve_all_boards_n(
  Boards const& bds,
  SolvedBoards& solved) -> int
{
  const int n = bds.no_of_boards;
  if (n > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solved.solved_board[k].cards = 0;

  scheduler.RegisterRun(RunMode::DDS_RUN_SOLVE, bds);

  SolveRunParam param;
  param.bds = &bds;
  param.solved = &solved;
  param.error.store(RETURN_NO_FAULT, std::memory_order_relaxed);

  START_BLOCK_TIMER;

  const int ret_run = run_solve_threads(param);

  END_BLOCK_TIMER;

  if (ret_run != RETURN_NO_FAULT)
    return ret_run;

  const int err = param.error.load(std::memory_order_relaxed);
  if (err != RETURN_NO_FAULT)
    return err;

  solved.no_of_boards = n;

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


int STDCALL SolveAllBoardsSeq(
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

  ensure_worker_contexts(sysdep.get_num_threads());

  START_BLOCK_TIMER;
  for (int bno = 0; bno < n && error == 0; bno++) {
    FutureTricks fut;
    const auto t0 = std::chrono::steady_clock::now();
    const int res = SolveBoard(
      bds.deals[bno], bds.target[bno], bds.solutions[bno],
      bds.mode[bno], &fut, 0);
    auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - t0).count();
    if (dur < 0) dur = 0;
    scheduler.SetBoardTime(bno, static_cast<int>(dur));

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

