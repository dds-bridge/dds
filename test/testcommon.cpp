/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


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

extern OptionsType options;
TestTimer timer;


void main_identify();


#define SOLVE_SIZE MAXNOOFBOARDS
#define BOARD_SIZE MAXNOOFTABLES
#define TRACE_SIZE MAXNOOFBOARDS
#define PAR_REPEAT 1


int realMain(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);
  timer.reset();
  timer.setname("Hand stats");

  bool GIBmode = false;

  // TODO: Make one.
  int stepsize = 0;
  if (options.solver == DTEST_SOLVER_SOLVE)
    stepsize = SOLVE_SIZE;
  else if (options.solver == DTEST_SOLVER_CALC)
    stepsize = BOARD_SIZE;
  else if (options.solver == DTEST_SOLVER_PLAY)
    stepsize = TRACE_SIZE;
  else if (options.solver == DTEST_SOLVER_PAR)
    stepsize = PAR_REPEAT;
  else if (options.solver == DTEST_SOLVER_DEALERPAR)
    stepsize = PAR_REPEAT;

  set_constants();
  main_identify();

  boardsPBN bop;
  solvedBoards solvedbdp;
  ddTableDealsPBN dealsp;
  ddTablesRes resp;
  allParResults parp;
  playTracesPBN playsp;
  solvedPlays solvedplp;

  int * dealer_list;
  int * vul_list;
  dealPBN * deal_list;
  futureTricks * fut_list;
  ddTableResults * table_list;
  parResults * par_list;
  parResultsDealer * dealerpar_list;
  playTracePBN * play_list;
  solvedPlay * trace_list;
  int number;

  if (read_file(options.fname, number, GIBmode, &dealer_list, &vul_list,
        &deal_list, &fut_list, &table_list, &par_list, &dealerpar_list,
        &play_list, &trace_list) == false)
  {
    printf("read_file failed.\n");
    exit(0);
  }

  if (options.solver == DTEST_SOLVER_SOLVE)
  {
    if (GIBmode)
    {
      printf("GIB file only works works with calc\n");
      exit(0);
    }
    loop_solve(&bop, &solvedbdp, deal_list, fut_list, number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_CALC)
  {
    loop_calc(&dealsp, &resp, &parp, deal_list, table_list, 
      number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_PLAY)
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_play(&bop, &playsp, &solvedplp, deal_list, play_list, trace_list, 
      number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_PAR)
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_par(vul_list, table_list, par_list, number, stepsize);
  }
  else if (options.solver == DTEST_SOLVER_DEALERPAR)
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_dealerpar(dealer_list, vul_list, table_list, dealerpar_list, 
      number, stepsize);
  }
  else
  {
    printf("Unknown type %d\n", options.solver);
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


void main_identify()
{
  printf("dtest main\n----------\n");

#if defined(_WIN32) || defined(__CYGWIN__)
  printf("%-12s %20s\n", "System", "Windows");
#if defined(_MSC_VER)
  printf("%-12s %20s\n", "Compiler", "Microsoft Visual C++");
#elif defined(__MINGW32__)
  printf("%-12s %20s\n", "Compiler", "MinGW");
#else
  printf("%-12s %20s\n", "Compiler", "GNU g++");
#endif

#elif defined(__linux)
  printf("%-12s %20s\n", "System", "Linux");
  printf("%-12s %20s\n", "Compiler", "GNU g++");

#elif defined(__APPLE__)
  printf("%-12s %20s\n", "System", "Apple");
#if defined(__clang__)
  printf("%-12s %20s\n", "Compiler", "clang");
#else
  printf("%-12s %20s\n", "Compiler", "GNU g++");
#endif
#endif

#if defined(__cplusplus)
  printf("%-12s %20ld\n", "Dialect", __cplusplus);
#endif

  printf("\n");
}

