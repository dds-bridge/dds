/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <cstdio>
#include <cstdlib>
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
#include <calc_tables.hpp>
#include <solve_board.hpp>
#include "system/scheduler.hpp"

using std::cout;
using std::endl;
using std::setw;
using std::left;
using std::right;

// Print per-batch running timing progress
#define BATCHTIMES

extern TestTimer timer;
extern OptionsType options;
extern Scheduler scheduler;


namespace {

auto report_dds_error(const char* where, const int code) -> void
{
  char line[80];
  ErrorMessage(code, line);
  cout << where << ": " << line << " (" << code << ")\n";
}

}  // namespace


auto loop_solve(
  BoardsPBN * bop,
  SolvedBoards * solvedbdp,
  DealPBN * deal_list,
  FutureTricks * fut_list,
  const int number,
  const int stepsize,
  std::vector<std::pair<int, int>>* board_times) -> bool
{
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
      timer.end();
      timer.finish_running();
      report_dds_error("loop_solve", ret);
      cout << "loop_solve: i " << i << "\n";
      return false;
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

      timer.finish_running();
      cout << "loop_solve: i " << i << ", j " << j << ": " <<
        "Difference\n\n";
      print_FUT(solvedbdp->solved_board[j]);
      cout << "\n";
      print_FUT(fut_list[i+j]);
      cout << "\n";
      return false;
    }
  }

#ifdef BATCHTIMES
  timer.finish_running();
#endif

  return true;
}


auto loop_calc(
  DealPBN * deal_list,
  DdTableResults * table_list,
  const int number,
  const int stepsize,
  std::vector<std::pair<int, int>>* board_times) -> bool
{
  // dtest harness progress only: call CalcAllTablesPBNX repeatedly with
  // `stepsize` deals (typically MAXNOOFBOARDS). Each call still expands to
  // count×strains boards in one parallel job — the X-API single-job contract
  // is unchanged; we intentionally do not pass the whole file in one call so
  // print_running can update between chunks.
  int filter[DDS_STRAINS] = {0, 0, 0, 0, 0};
  const int strain_count = DDS_STRAINS;
  if (number <= 0)
    return true;

  int batch = stepsize;
  if (batch <= 0)
    batch = number;

  std::vector<DdTableDealPBN> deals(static_cast<unsigned>(batch));
  std::vector<DdTableResults> results(static_cast<unsigned>(batch));

  for (int i = 0; i < number; i += batch)
  {
    const int count = (i + batch > number ? number - i : batch);

    for (int j = 0; j < count; j++)
    {
      std::strncpy(
        deals[static_cast<unsigned>(j)].cards,
        deal_list[i + j].remainCards,
        sizeof(deals[0].cards));
      deals[static_cast<unsigned>(j)].cards[sizeof(deals[0].cards) - 1] = '\0';
    }

    timer.start(count);
    const int workload = count * strain_count;
    const int threads = dtest_effective_threads(options.num_threads_, workload);
    std::vector<int> strain_times;
    const int ret = calc_all_tables_pbn_x(
      count,
      deals.data(),
      -1,
      filter,
      results.data(),
      nullptr,
      threads,
      board_times != nullptr ? &strain_times : nullptr);
    if (ret != RETURN_NO_FAULT)
    {
      timer.end();
      timer.finish_running();
      report_dds_error("loop_calc", ret);
      cout << "loop_calc: i " << i << "\n";
      return false;
    }
    timer.end();

    if (board_times != nullptr)
    {
      append_calc_batch_deal_times(
        *board_times, strain_times, strain_count, i);
    }

#ifdef BATCHTIMES
    timer.print_running(i + count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (compare_TABLE(results[static_cast<unsigned>(j)], table_list[i + j]))
        continue;

      timer.finish_running();
      cout << "loop_calc: j " << (i + j) << ": Difference\n\n";
      print_TABLE(results[static_cast<unsigned>(j)]);
      cout << "\n";
      print_TABLE(table_list[i + j]);
      cout << "\n";
      return false;
    }
  }

#ifdef BATCHTIMES
  timer.finish_running();
#endif

  return true;
}



auto loop_par(
  int * vul_list,
  DdTableResults * table_list,
  ParResults * par_list,
  const int number,
  const int stepsize) -> bool
{
  // This is so fast that there is no batch or multi-threaded
  // version. We run it many times just to get meaningful times.

  ParResults presp;

  for (int i = 0; i < number; i++)
  {
    timer.start(1);
    for (int j = 0; j < stepsize; j++)
    {
      int ret;
      if ((ret = Par(&table_list[i], &presp, vul_list[i]))
          != RETURN_NO_FAULT)
      {
        timer.end();
        timer.finish_running();
        report_dds_error("loop_par", ret);
        cout << "loop_par: i " << i << ", j " << j << "\n";
        return false;
      }
    }
    timer.end();

    if (compare_PAR(presp, par_list[i]))
    {
#ifdef BATCHTIMES
      timer.print_running(i + 1, number);
#endif
      continue;
    }

#ifdef BATCHTIMES
    timer.finish_running();
#endif
    cout << "loop_par i " << i << ": Difference\n\n";
    print_PAR(presp);
    cout << "\n";
    print_PAR(par_list[i]);
    cout << "\n";
    return false;
  }

#ifdef BATCHTIMES
  timer.finish_running();
#endif

  return true;
}


auto loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  DdTableResults * table_list,
  ParResultsDealer * dealerpar_list,
  const int number,
  const int stepsize) -> bool
{
  // This is so fast that there is no batch or multi-threaded
  // version. We run it many times just to get meaningful times.

  ParResultsDealer presp;

  for (int i = 0; i < number; i++)
  {
    timer.start(1);
    for (int j = 0; j < stepsize; j++)
    {
      int ret;
      if ((ret = DealerPar(&table_list[i], &presp,
          dealer_list[i], vul_list[i])) != RETURN_NO_FAULT)
      {
        timer.end();
        timer.finish_running();
        report_dds_error("loop_dealerpar", ret);
        cout << "loop_dealerpar: i " << i << ", j " << j << "\n";
        return false;
      }
    }
    timer.end();

    if (compare_DEALERPAR(presp, dealerpar_list[i]))
    {
#ifdef BATCHTIMES
      timer.print_running(i + 1, number);
#endif
      continue;
    }

#ifdef BATCHTIMES
    timer.finish_running();
#endif
    cout << "loop_dealerpar i " << i << ": Difference\n\n";
    print_DEALERPAR(presp);
    cout << "\n";
    print_DEALERPAR(dealerpar_list[i]);
    cout << "\n";
    return false;
  }

#ifdef BATCHTIMES
  timer.finish_running();
#endif

  return true;
}


auto loop_play(
  BoardsPBN * bop,
  PlayTracesPBN * playsp,
  SolvedPlays * solvedplp,
  DealPBN * deal_list,
  PlayTracePBN * play_list,
  SolvedPlay * trace_list,
  const int number,
  const int stepsize) -> bool
{
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
      timer.end();
      timer.finish_running();
      report_dds_error("loop_play", ret);
      cout << "loop_play: i " << i << "\n";
      return false;
    }
    timer.end();

#ifdef BATCHTIMES
    timer.print_running(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (compare_TRACE(solvedplp->solved[j], trace_list[i+j]))
        continue;

      timer.finish_running();
      printf("loop_play i %d, j %d: Difference\n", i, j);
      cout << "loop_play: i " << i << ", j " << j << ": " <<
        "Difference\n\n";
      print_double_TRACE(solvedplp->solved[j], trace_list[i+j]);
      cout << "\n";
      return false;
    }
  }

#ifdef BATCHTIMES
  timer.finish_running();
#endif

  return true;
}
