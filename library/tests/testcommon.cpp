/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>

#include <api/dll.h>

#include "testcommon.hpp"
#include "TestTimer.hpp"
#include "parse.hpp"
#include "loop.hpp"
#include "print.hpp"
#include "cst.hpp"
#include "report_board_timings.hpp"
#include "system/scheduler.hpp"

using std::cout;
using std::endl;
using std::left;
using std::right;
using std::setw;
using std::string;
using std::vector;

string GetSystem();
string GetBits();
string GetCompiler();


const vector<string> DDS_SYSTEM_PLATFORM =
{
  "",
  "Windows",
  "Cygwin",
  "Linux",
  "Apple"
};

const vector<string> DDS_SYSTEM_COMPILER =
{
  "",
  "Microsoft Visual C++",
  "MinGW",
  "GNU g++",
  "clang"
};


extern OptionsType options;
TestTimer timer;
extern Scheduler scheduler;


void main_identify();


int real_main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[])
{
  bool GIBmode = false;

  int stepsize = 0;
  if (options.solver_ == Solver::DTEST_SOLVER_SOLVE)
    stepsize = MAXNOOFBOARDS;
  else if (options.solver_ == Solver::DTEST_SOLVER_CALC)
    stepsize = MAXNOOFTABLES;
  else if (options.solver_ == Solver::DTEST_SOLVER_PLAY)
    stepsize = MAXNOOFBOARDS;
  else if (options.solver_ == Solver::DTEST_SOLVER_PAR)
    stepsize = 1;
  else if (options.solver_ == Solver::DTEST_SOLVER_DEALERPAR)
    stepsize = 1;

  set_constants();
  main_identify();

  int number = 0;
  int * dealer_list = nullptr;
  int * vul_list = nullptr;
  DealPBN * deal_list = nullptr;
  FutureTricks * fut_list = nullptr;
  DdTableResults * table_list = nullptr;
  ParResults * par_list = nullptr;
  ParResultsDealer * dealerpar_list = nullptr;
  PlayTracePBN * play_list = nullptr;
  SolvedPlay * trace_list = nullptr;
  if (read_file(options.fname_, number, GIBmode, &dealer_list, &vul_list,
        &deal_list, &fut_list, &table_list, &par_list, &dealerpar_list,
        &play_list, &trace_list) == false)
  {
    cout << "read_file failed\n";
    exit(0);
  }

  if (GIBmode && options.solver_ != Solver::DTEST_SOLVER_CALC)
  {
    cout << "GIB file only works with calc\n";
    exit(0);
  }

  timer.reset();
  timer.set_name("Hand stats");

  BoardsPBN bop;
  SolvedBoards solvedbdp;
  PlayTracesPBN playsp;
  SolvedPlays solvedplp;

  std::vector<std::pair<int, int>> board_times;
  if (options.report_slow_boards_)
    board_times.reserve(static_cast<std::size_t>(number));

  if (options.solver_ == Solver::DTEST_SOLVER_SOLVE)
  {
    loop_solve(
      &bop,
      &solvedbdp,
      deal_list,
      fut_list,
      number,
      stepsize,
      options.report_slow_boards_ ? &board_times : nullptr);
  }
  else if (options.solver_ == Solver::DTEST_SOLVER_CALC)
  {
    loop_calc(deal_list, table_list, number);
  }
  else if (options.solver_ == Solver::DTEST_SOLVER_PLAY)
  {
    loop_play(&bop, &playsp, &solvedplp, deal_list, play_list, trace_list, 
      number, stepsize);
  }
  else if (options.solver_ == Solver::DTEST_SOLVER_PAR)
  {
    loop_par(vul_list, table_list, par_list, number, stepsize);
  }
  else if (options.solver_ == Solver::DTEST_SOLVER_DEALERPAR)
  {
    loop_dealerpar(dealer_list, vul_list, table_list, dealerpar_list, 
      number, stepsize);
  }
  else
  {
    cout << "Unknown type " << 
      static_cast<unsigned>(options.solver_) << "\n";
    exit(0);
  }

  timer.print_hands(cout);

  if (options.report_slow_boards_)
  {
    if (board_times.empty())
    {
      if (options.solver_ == Solver::DTEST_SOLVER_CALC)
      {
        cout << "Per-board timing data not available for calc (use -s solve -r)."
             << std::endl;
      }
      else
      {
        cout << "Per-board timing data not available." << std::endl;
      }
    }
    else
    {
      print_per_board_timings(cout, std::move(board_times));
    }
  }

  free(dealer_list);
  free(vul_list);
  free(deal_list);
  free(fut_list);
  free(table_list);
  free(par_list);
  free(dealerpar_list);
  free(play_list);
  free(trace_list);

  // Release heavy timing storage before program exit to avoid long destructor work.
  scheduler.ClearTiming();

  return (0);
}


//////////////////////////////////////////////////////////////////////
//                     Self-identification                          //
//////////////////////////////////////////////////////////////////////

string GetSystem()
{
  unsigned sys;
#if defined(_WIN32)
  sys = 1;
#elif defined(__CYGWIN__)
  sys = 2;
#elif defined(__linux)
  sys = 3;
#elif defined(__APPLE__)
  sys = 4;
#else
  sys = 0;
#endif
  
  return DDS_SYSTEM_PLATFORM[sys];
}


string GetBits()
{
  if constexpr (sizeof(void *) == 4)
    return "32 bits";
  else if constexpr (sizeof(void *) == 8)
    return "64 bits";
  else
    return "unknown";
}


string GetCompiler()
{
  unsigned comp;
#if defined(_MSC_VER)
  comp = 1;
#elif defined(__MINGW32__)
  comp = 2;
#elif defined(__clang__)
  comp = 4; // Out-of-order on purpose
#elif defined(__GNUC__)
  comp = 3;
#else
  comp = 0;
#endif

  return DDS_SYSTEM_COMPILER[comp];
}


void main_identify()
{
  cout << "test program\n";
  cout << string(13, '-') << "\n";

  const string strSystem = GetSystem();
  cout << left << setw(13) << "System" <<
    setw(20) << right << strSystem << "\n";

  const string strBits = GetBits();
  cout << left << setw(13) << "Word size" <<
    setw(20) << right << strBits << "\n";

  const string strCompiler = GetCompiler();
  cout << left << setw(13) << "Compiler" <<
    setw(20) << right << strCompiler << "\n\n";
}




