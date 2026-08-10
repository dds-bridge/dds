/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <cstring>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

#include "loop.hpp"
#include "TestTimer.hpp"
#include "compare.hpp"
#include "print.hpp"
#include "cst.hpp"
#include "dtest_parallel.hpp"
#include "report_board_timings.hpp"
#include <solve_board.hpp>
#include "system/scheduler.hpp"

using std::cout;
using std::endl;
using std::setw;
using std::left;
using std::right;

#define BATCHTIMES

extern TestTimer timer;
extern OptionsType options;
extern Scheduler scheduler;


void loop_solve(
  BoardsPBN * bop,
  SolvedBoards * solvedbdp,
  DealPBN * deal_list,
  FutureTricks * fut_list,
  const int number,
  const int stepsize,
  std::vector<std::pair<int, int>>* board_times)
{
#ifdef BATCHTIMES
  cout << setw(8) << left << "Hand no." << 
    setw(25) << right << "Time" << "\n";
#endif

  for (int i = 0; i < number; i += stepsize)
  {
    int count = (i + stepsize > number ? number - i : stepsize);

    bop->no_of_boards = count;
    for (int j = 0; j < count; j++)
    {
      bop->deals[j] = deal_list[i + j];
      bop->target[j] = -1;
      bop->solutions[j] = 3;
      bop->mode[j] = 1;
  // (no-op)
    }

    timer.start(count);
    int ret;
    if (dtest_effective_threads(options.num_threads_, count) <= 1)
    {
      ret = SolveAllBoardsSeq(bop, solvedbdp);
    }
    else
    {
      ret = solve_all_boards_pbn_n(*bop, *solvedbdp,
        dtest_effective_threads(options.num_threads_, count));
    }
    if (ret != RETURN_NO_FAULT)
    {
      cout << "loop_solve: i " << i << ", return " << ret << "\n";
      exit(0);
    }
    timer.end();

    if (board_times != nullptr)
    {
      std::vector<std::pair<int, int>> batch_times;
      scheduler.GetBoardTimes(batch_times);
      append_batch_board_times(*board_times, batch_times, i);
    }

#ifdef BATCHTIMES
    timer.print_running(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (compare_FUT(solvedbdp->solved_board[j], fut_list[i + j]))
        continue;

      cout << "loop_solve: i " << i << ", j " << j << ": " <<
        "Difference\n\n";
      print_FUT(solvedbdp->solved_board[j]);
      cout << "\n";
      print_FUT(fut_list[i+j]);
      cout << "\n";
    }
  }

#ifdef BATCHTIMES
  cout << "\n";
#endif

}


bool loop_calc(
  DealPBN * deal_list,
  DdTableResults * table_list,
  const int number)
{
#ifdef BATCHTIMES
  cout << setw(8) << left << "Hand no." <<
    setw(25) << right << "Time" << "\n";
#endif

  // One CalcAllTablesPBNX call for the whole file: expands to number×strains
  // boards and solves them in a single parallel job (ddss large-batch shape).
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
  const int strain_count = DDS_STRAINS;
  if (number <= 0)
    return true;
  std::vector<DdTableDealPBN> deals(static_cast<unsigned>(number));
  std::vector<DdTableResults> results(static_cast<unsigned>(number));
  for (int i = 0; i < number; i++)
  {
    std::strncpy(
      deals[static_cast<unsigned>(i)].cards,
      deal_list[i].remainCards,
      sizeof(deals[0].cards));
    deals[static_cast<unsigned>(i)].cards[sizeof(deals[0].cards) - 1] = '\0';
  }

  timer.start(number);
  const int workload = number * strain_count;
  const int threads = dtest_effective_threads(options.num_threads_, workload);
  const int ret = CalcAllTablesPBNX(
    number, deals.data(), -1, filter, results.data(), nullptr, threads);
  timer.end();
  if (ret != RETURN_NO_FAULT)
  {
    cout << "loop_calc: CalcAllTablesPBNX return " << ret << "\n";
    exit(0);
  }

#ifdef BATCHTIMES
  timer.print_running(number, number);
#endif

  for (int j = 0; j < number; j++)
  {
    if (compare_TABLE(results[static_cast<unsigned>(j)], table_list[j]))
      continue;

    cout << "loop_calc: j " << j << ": Difference\n\n";
    print_TABLE(results[static_cast<unsigned>(j)]);
    cout << "\n";
    print_TABLE(table_list[j]);
    cout << "\n";
  }

#ifdef BATCHTIMES
  cout << "\n";
#endif

  return true;
}



bool loop_par(
  int * vul_list,
  DdTableResults * table_list,
  ParResults * par_list,
  const int number,
  const int stepsize)
{
  // This is so fast that there is no batch or multi-threaded
  // version. We run it many times just to get meaningful times.

  ParResults presp;

  for (int i = 0; i < number; i++)
  {
    for (int j = 0; j < stepsize; j++)
    {
      int ret;
      if ((ret = Par(&table_list[i], &presp, vul_list[i]))
          != RETURN_NO_FAULT)
      {
        cout << "loop_par: i " << i << ", j " << j << ": " <<
          "return " << ret << "\n";
        exit(0);
      }
    }

    if (compare_PAR(presp, par_list[i]))
      continue;

    cout << "loop_par i " << i << ": Difference\n\n";
    print_PAR(presp);
    cout << "\n";
    print_PAR(par_list[i]);
    cout << "\n";
  }

  return true;
}


bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  DdTableResults * table_list,
  ParResultsDealer * dealerpar_list,
  const int number,
  const int stepsize)
{
  // This is so fast that there is no batch or multi-threaded
  // version. We run it many times just to get meaningful times.

  ParResultsDealer presp;

  timer.start(number);
  for (int i = 0; i < number; i++)
  {
    for (int j = 0; j < stepsize; j++)
    {
      int ret;
      if ((ret = DealerPar(&table_list[i], &presp,
          dealer_list[i], vul_list[i])) != RETURN_NO_FAULT)
      {
        cout << "loop_dealerpar: i " << i << ", j " << j << ": " <<
          "return " << ret << "\n";
        exit(0);
      }
    }

    if (compare_DEALERPAR(presp, dealerpar_list[i]))
      continue;

    cout << "loop_dealerpar i " << i << ": Difference\n\n";
    print_DEALERPAR(presp);
    cout << "\n";
    print_DEALERPAR(dealerpar_list[i]);
    cout << "\n";
  }
  timer.end();

#ifdef BATCHTIMES
  timer.print_running(number, number);
#endif

  return true;
}


bool loop_play(
  BoardsPBN * bop,
  PlayTracesPBN * playsp,
  SolvedPlays * solvedplp,
  DealPBN * deal_list,
  PlayTracePBN * play_list,
  SolvedPlay * trace_list,
  const int number,
  const int stepsize)
{
#ifdef BATCHTIMES
  cout << setw(8) << left << "Hand no." << 
    setw(25) << right << "Time" << "\n";
#endif

  for (int i = 0; i < number; i += stepsize)
  {
    int count = (i + stepsize > number ? number - i : stepsize);

    bop->no_of_boards = count;
    playsp->no_of_boards = count;

    for (int j = 0; j < count; j++)
    {
      bop->deals[j] = deal_list[i + j];
      bop->target[j] = 0;
      bop->solutions[j] = 3;
      bop->mode[j] = 1;

      playsp->plays[j] = play_list[i + j];
    }

    timer.start(count);
    int ret;
    if (dtest_effective_threads(options.num_threads_, count) <= 1)
    {
      ret = AnalyseAllPlaysPBN(bop, playsp, solvedplp, 1);
    }
    else
    {
      solvedplp->no_of_boards = count;
      ret = dtest_run_parallel(count, options.num_threads_,
        [&](const int j) -> int {
          return AnalysePlayPBN(
            bop->deals[j], playsp->plays[j], &solvedplp->solved[j], 0);
        });
    }
    if (ret != RETURN_NO_FAULT)
    {
      printf("loop_play i %i: Return %d\n", i, ret);
      cout << "loop_play: i " << i << ": " << "return " << ret << "\n";
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.print_running(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (compare_TRACE(solvedplp->solved[j], trace_list[i+j]))
        continue;

      printf("loop_play i %d, j %d: Difference\n", i, j);
      cout << "loop_play: i " << i << ", j " << j << ": " <<
        "Difference\n\n";
      print_double_TRACE(solvedplp->solved[j], trace_list[i+j]);
      cout << "\n";
    }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

  return true;
}

