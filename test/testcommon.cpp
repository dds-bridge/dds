/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "../include/dll.h"
#include "../include/portab.h"
#include "TestTimer.h"
#include "parse.h"
#include "compare.h"
#include "cst.h"
#include "testcommon.h"

using namespace std;

extern OptionsType options;

TestTimer timer;


void print_rank_count();
unsigned short int dbitMapRank[16];
unsigned char dcardRank[16];
unsigned char dcardSuit[5];

void main_identify();

void set_constants();

bool print_PBN(
  struct dealPBN * dl);

bool print_FUT(
  struct futureTricks * fut);

void equals_to_string(
  int equals,
  char * res);

bool print_TABLE(
  struct ddTableResults * table);

bool print_PAR(
  struct parResults * par);

bool print_DEALERPAR(
  struct parResultsDealer * par);

bool print_PLAY(
  struct playTracePBN * play);

bool print_TRACE(
  struct solvedPlay * solvedp);

void loop_solve(
  struct boardsPBN * bop,
  struct solvedBoards * solvedbdp,
  struct dealPBN * deal_list,
  struct futureTricks * fut_list,
  int number);

bool loop_calc(
  struct ddTableDealsPBN * dealsp,
  struct ddTablesRes * resp,
  struct allParResults * parp,
  struct dealPBN * deal_list,
  struct ddTableResults * table_list,
  int number);

bool loop_par(
  int * vul_list,
  struct ddTableResults * table_list,
  struct parResults * par_list,
  int number);

bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  struct ddTableResults * table_list,
  struct parResultsDealer * dealerpar_list,
  int number);

bool loop_play(
  struct boardsPBN * bop,
  struct playTracesPBN * playsp,
  struct solvedPlays * solvedplp,
  struct dealPBN * deal_list,
  struct playTracePBN * play_list,
  struct solvedPlay * trace_list,
  int number);

void print_times(
  int number);

#ifndef _WIN32
int timeval_diff(
  timeval x,
  timeval y);
#endif

void timer_start();

int timer_end();




#define SOLVE_SIZE MAXNOOFBOARDS
#define BOARD_SIZE MAXNOOFTABLES
#define TRACE_SIZE MAXNOOFBOARDS
#define PAR_REPEAT 1

int input_number;


// #define DEBUG
#define BATCHTIMES
#define ZERO (static_cast<int>('0'))
#define NINE (static_cast<int>('9'))
#define SPACE (static_cast<int>(' '))
#define QUOTE (static_cast<int>('"'))


int realMain(int argc, char * argv[]);

int realMain(int argc, char * argv[])
{
  UNUSED(argc);
  UNUSED(argv);
  input_number = 0;
  timer.reset();
  timer.setname("Hand stats");

  bool GIBmode = false;

  // TODO: Make one.
  if (options.solver == DTEST_SOLVER_SOLVE)
    input_number = SOLVE_SIZE;
  else if (options.solver == DTEST_SOLVER_CALC)
    input_number = BOARD_SIZE;
  else if (options.solver == DTEST_SOLVER_PLAY)
    input_number = TRACE_SIZE;
  else if (options.solver == DTEST_SOLVER_PAR)
    input_number = PAR_REPEAT;
  else if (options.solver == DTEST_SOLVER_DEALERPAR)
    input_number = PAR_REPEAT;

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
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_solve(&bop, &solvedbdp, deal_list, fut_list, number);
  }
  else if (options.solver == DTEST_SOLVER_CALC)
  {
    loop_calc(&dealsp, &resp, &parp,
              deal_list, table_list, number);
  }
  else if (options.solver == DTEST_SOLVER_PLAY)
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_play(&bop, &playsp, &solvedplp,
              deal_list, play_list, trace_list, number);
  }
  else if (options.solver == DTEST_SOLVER_PAR)
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_par(vul_list, table_list, par_list, number);
  }
  else if (options.solver == DTEST_SOLVER_DEALERPAR)
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_dealerpar(dealer_list, vul_list, table_list,
                   dealerpar_list, number);
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


void set_constants()
{
  dbitMapRank[15] = 0x2000;
  dbitMapRank[14] = 0x1000;
  dbitMapRank[13] = 0x0800;
  dbitMapRank[12] = 0x0400;
  dbitMapRank[11] = 0x0200;
  dbitMapRank[10] = 0x0100;
  dbitMapRank[ 9] = 0x0080;
  dbitMapRank[ 8] = 0x0040;
  dbitMapRank[ 7] = 0x0020;
  dbitMapRank[ 6] = 0x0010;
  dbitMapRank[ 5] = 0x0008;
  dbitMapRank[ 4] = 0x0004;
  dbitMapRank[ 3] = 0x0002;
  dbitMapRank[ 2] = 0x0001;
  dbitMapRank[ 1] = 0;
  dbitMapRank[ 0] = 0;

  dcardRank[ 2] = '2';
  dcardRank[ 3] = '3';
  dcardRank[ 4] = '4';
  dcardRank[ 5] = '5';
  dcardRank[ 6] = '6';
  dcardRank[ 7] = '7';
  dcardRank[ 8] = '8';
  dcardRank[ 9] = '9';
  dcardRank[10] = 'T';
  dcardRank[11] = 'J';
  dcardRank[12] = 'Q';
  dcardRank[13] = 'K';
  dcardRank[14] = 'A';
  dcardRank[15] = '-';

  dcardSuit[0] = 'S';
  dcardSuit[1] = 'H';
  dcardSuit[2] = 'D';
  dcardSuit[3] = 'C';
  dcardSuit[4] = 'N';
}


bool print_PBN(dealPBN * dl)
{
  printf("%10s %d\n", "trump", dl->trump);
  printf("%10s %d\n", "first", dl->first);
  printf("%10s %s\n", "cards", dl->remainCards);
  return true;
}


bool print_FUT(futureTricks * fut)
{
  printf("%6s %d\n", "cards", fut->cards);
  printf("%6s %-6s %-6s %-6s %-6s\n",
         "", "suit", "rank", "equals", "score");

  for (int i = 0; i < fut->cards; i++)
  {
    char res[15] = "";
    equals_to_string(fut->equals[i], res);
    printf("%6d %-6c %-6c %-6s %-6d\n",
           i,
           dcardSuit[ fut->suit[i] ],
           dcardRank[ fut->rank[i] ],
           res,
           fut->score[i]);
  }
  return true;
}


void equals_to_string(int equals, char * res)
{
  int p = 0;
  for (int i = 15; i >= 2; i--)
  {
    if (equals & dbitMapRank[i])
      res[p++] = static_cast<char>(dcardRank[i]);
  }
  res[p] = 0;
}


bool print_TABLE(ddTableResults * table)
{
  printf("%5s %-5s %-5s %-5s %-5s\n",
         "", "North", "South", "East", "West");

  printf("%5s %5d %5d %5d %5d\n",
         "NT",
         table->resTable[4][0],
         table->resTable[4][2],
         table->resTable[4][1],
         table->resTable[4][3]);

  for (int suit = 0; suit <= 3; suit++)
  {
    printf("%5c %5d %5d %5d %5d\n",
           dcardSuit[suit],
           table->resTable[suit][0],
           table->resTable[suit][2],
           table->resTable[suit][1],
           table->resTable[suit][3]);
  }
  return true;
}


bool print_PAR(parResults * par)
{
  printf("NS score: %s\n", par->parScore[0]);
  printf("EW score: %s\n", par->parScore[1]);
  printf("NS list : %s\n", par->parContractsString[0]);
  printf("EW list : %s\n", par->parContractsString[1]);
  return true;
}


bool print_DEALERPAR(parResultsDealer * par)
{
  printf("Score : %d\n", par->score);
  printf("Pars : %d\n", par->number);

  for (int i = 0; i < par->number; i++)
  {
    printf("Par %d: %s\n", i, par->contracts[i]);
  }
  return true;
}


bool print_PLAY(playTracePBN * play)
{
  printf("Number : %d\n", play->number);

  for (int i = 0; i < play->number; i++)
  {
    printf("Play %d: %c%c\n", i, play->cards[2 * i], play->cards[2 * i + 1]);
  }
  return true;
}


bool print_TRACE(solvedPlay * solvedp)
{
  printf("Number : %d\n", solvedp->number);

  for (int i = 0; i < solvedp->number; i++)
  {
    printf("Trick %d: %d\n", i, solvedp->tricks[i]);
  }
  return true;
}


bool print_double_TRACE(solvedPlay * solvedp, solvedPlay * refp)
{
  printf("Number solved vs ref : %d vs %d\n", 
    solvedp->number, refp->number);

  const int m = min(solvedp->number, refp->number);
  for (int i = 0; i < m;i++)
  {
    printf("Trick %d: %d vs %d %s\n", 
      i, solvedp->tricks[i], refp->tricks[i],
      (solvedp->tricks[i] == refp->tricks[i] ? "" : "ERROR"));
  }

  if (solvedp->number > m)
  {
    for (int i = m; i < solvedp->number; i++)
      printf("Solved %d: %d\n", i, solvedp->tricks[i]);
  }
  else if (refp->number > m)
  {
    for (int i = m; i < solvedp->number; i++)
      printf("Ref %d: %d\n", i, refp->tricks[i]);
  }

  return true;
}


void loop_solve(
  boardsPBN * bop,
  solvedBoards * solvedbdp,
  dealPBN * deal_list,
  futureTricks * fut_list,
  int number)
{
#ifdef BATCHTIMES
  printf("%8s %24s\n", "Hand no.", "Time");
#endif

  for (int i = 0; i < number; i += input_number)
  {
    int count = (i + input_number > number ? number - i : input_number);

    bop->noOfBoards = count;
    for (int j = 0; j < count; j++)
    {
      bop->deals[j] = deal_list[i + j];
      bop->target[j] = -1;
      bop->solutions[j] = 3;
      bop->mode[j] = 1;
    }

    timer.start(count);
    int ret;
    if ((ret = SolveAllChunks(bop, solvedbdp, 1))
        != RETURN_NO_FAULT)
    {
      printf("loop_solve i %i: Return %d\n", i, ret);
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.printRunning(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (! compare_FUT(solvedbdp->solvedBoard[j], fut_list[i + j]))
        printf("loop_solve i %d, j %d: Difference\n", i, j);
    }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

}


bool loop_calc(
  ddTableDealsPBN * dealsp,
  ddTablesRes * resp,
  allParResults * parp,
  dealPBN * deal_list,
  ddTableResults * table_list,
  int number)
{
#ifdef BATCHTIMES
  printf("%8s %24s\n", "Hand no.", "Time");
#endif

  int filter[5] = {0, 0, 0, 0, 0};

  for (int i = 0; i < number; i += input_number)
  {
    int count = (i + input_number > number ? number - i : input_number);
    dealsp->noOfTables = count;
    for (int j = 0; j < count; j++)
    {
      strcpy(dealsp->deals[j].cards, deal_list[i + j].remainCards);
    }

    timer.start(count);
    int ret;
    if ((ret = CalcAllTablesPBN(dealsp, -1, filter, resp, parp))
        != RETURN_NO_FAULT)
    {
      printf("loop_solve i %i: Return %d\n", i, ret);
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.printRunning(i+count, number);
#endif

    for (int j = 0; j < count; j++)
      if (! compare_TABLE(resp->results[j], table_list[i + j]))
      {
        printf("loop_calc table i %d, j %d: Difference\n", i, j);
        print_TABLE( &resp->results[j] );
        print_TABLE( &table_list[i + j]) ;
      }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

  return true;
}



bool loop_par(
  int * vul_list,
  ddTableResults * table_list,
  parResults * par_list,
  int number)
{
  /* This is so fast that there is no batch or multi-threaded
     version. We run it many times just to get meaningful times. */

  parResults presp;

  for (int i = 0; i < number; i++)
  {
    for (int j = 0; j < input_number; j++)
    {
      int ret;
      if ((ret = Par(&table_list[i], &presp, vul_list[i]))
          != RETURN_NO_FAULT)
      {
        printf("loop_par i %i, j %d: Return %d\n", i, j, ret);
        exit(0);
      }
    }

    if (! compare_PAR(presp, par_list[i]))
      printf("loop_par i %d: Difference\n", i);
  }

  return true;
}


bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  ddTableResults * table_list,
  parResultsDealer * dealerpar_list,
  int number)
{
  /* This is so fast that there is no batch or multi-threaded
     version. We run it many times just to get meaningful times. */

  parResultsDealer presp;

  timer.start(number);
  for (int i = 0; i < number; i++)
  {
    for (int j = 0; j < input_number; j++)
    {
      int ret;
      if ((ret = DealerPar(&table_list[i], &presp,
                           dealer_list[i], vul_list[i]))
          != RETURN_NO_FAULT)
      {
        printf("loop_dealerpar i %i, j %d: Return %d\n", i, j, ret);
        exit(0);
      }
    }

    if (! compare_DEALERPAR(presp, dealerpar_list[i]))
    {
      printf("loop_dealerpar i %d: Difference\n", i);
    }
  }
  timer.end();

#ifdef BATCHTIMES
  timer.printRunning(number, number);
#endif

  return true;
}


bool loop_play(
  boardsPBN * bop,
  playTracesPBN * playsp,
  solvedPlays * solvedplp,
  dealPBN * deal_list,
  playTracePBN * play_list,
  solvedPlay * trace_list,
  int number)
{
#ifdef BATCHTIMES
  printf("%8s %24s\n", "Hand no.", "Time");
#endif

  for (int i = 0; i < number; i += input_number)
  {
    int count = (i + input_number > number ? number - i : input_number);

    bop->noOfBoards = count;
    playsp->noOfBoards = count;

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
    if ((ret = AnalyseAllPlaysPBN(bop, playsp, solvedplp, 1))
        != RETURN_NO_FAULT)
    {
      printf("loop_play i %i: Return %d\n", i, ret);
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.printRunning(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (! compare_TRACE(solvedplp->solved[j], trace_list[i + j]))
      {
        printf("loop_play i %d, j %d: Difference\n", i, j);
        print_double_TRACE(&solvedplp->solved[j], &trace_list[i+j]);
      }
    }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

  return true;
}

