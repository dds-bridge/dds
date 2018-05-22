/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <vector>

#include "../include/dll.h"
#include "../include/portab.h"

#include "testcommon.h"
#include "TestTimer.h"
#include "parse.h"
#include "loop.h"
#include "compare.h"
#include "print.h"
#include "cst.h"

using namespace std;

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


void main_identify();


int realMain(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);
  bool GIBmode = false;

  int stepsize = 0;
  if (options.solver == DTEST_SOLVER_SOLVE)
    stepsize = MAXNOOFBOARDS;
  else if (options.solver == DTEST_SOLVER_CALC)
    stepsize = MAXNOOFTABLES;
  else if (options.solver == DTEST_SOLVER_PLAY)
    stepsize = MAXNOOFBOARDS;
  else if (options.solver == DTEST_SOLVER_PAR)
    stepsize = 1;
  else if (options.solver == DTEST_SOLVER_DEALERPAR)
    stepsize = 1;

  set_constants();
  main_identify();

  int number;
  int * dealer_list;
  int * vul_list;
  dealPBN * deal_list;
  futureTricks * fut_list;
  ddTableResults * table_list;
  parResults * par_list;
  parResultsDealer * dealerpar_list;
  playTracePBN * play_list;
  solvedPlay * trace_list;
  if (read_file(options.fname, number, GIBmode, &dealer_list, &vul_list,
        &deal_list, &fut_list, &table_list, &par_list, &dealerpar_list,
        &play_list, &trace_list) == false)
  {
    cout << "read_file failed\n";
    exit(0);
  }

  if (GIBmode && options.solver != DTEST_SOLVER_CALC)
  {
    cout << "GIB file only works works with calc\n";
    exit(0);
  }

  timer.reset();
  timer.setname("Hand stats");

  boardsPBN bop;
  solvedBoards solvedbdp;
  ddTableDealsPBN dealsp;
  ddTablesRes resp;
  allParResults parp;
  playTracesPBN playsp;
  solvedPlays solvedplp;

  if (options.solver == DTEST_SOLVER_SOLVE)
  {
    loop_solve(&bop, &solvedbdp, deal_list, fut_list, number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_CALC)
  {
    loop_calc(&dealsp, &resp, &parp, deal_list, table_list, 
      number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_PLAY)
  {
    loop_play(&bop, &playsp, &solvedplp, deal_list, play_list, trace_list, 
      number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_PAR)
  {
    loop_par(vul_list, table_list, par_list, number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_DEALERPAR)
  {
    loop_dealerpar(dealer_list, vul_list, table_list, dealerpar_list, 
      number, stepsize);
  }
  else
  {
    cout << "Unknown type " << 
      static_cast<unsigned>(options.solver) << "\n";
    exit(0);
  }

  timer.printHands();

  free(dealer_list);
  free(vul_list);
  free(deal_list);
  free(fut_list);
  free(table_list);
  free(par_list);
  free(dealerpar_list);
  free(play_list);
  free(trace_list);

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
  if (sizeof(void *) == 4)
    return "32 bits";
  else if (sizeof(void *) == 8)
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




