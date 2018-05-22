/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <string.h>

#include "loop.h"
#include "TestTimer.h"
#include "compare.h"
#include "print.h"

using namespace std;

#define BATCHTIMES

extern TestTimer timer;


void loop_solve(
  boardsPBN * bop,
  solvedBoards * solvedbdp,
  dealPBN * deal_list,
  futureTricks * fut_list,
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
    if ((ret = SolveAllChunks(bop, solvedbdp, 1)) != RETURN_NO_FAULT)
    {
      cout << "loop_solve: i " << i << ", return " << ret << "\n";
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.printRunning(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (compare_FUT(solvedbdp->solvedBoard[j], fut_list[i + j]))
        continue;

      cout << "loop_solve: i " << i << ", j " << j << ": " <<
        "Difference\n\n";
      print_FUT(solvedbdp->solvedBoard[j]);
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
  ddTableDealsPBN * dealsp,
  ddTablesRes * resp,
  allParResults * parp,
  dealPBN * deal_list,
  ddTableResults * table_list,
  const int number,
  const int stepsize)
{
#ifdef BATCHTIMES
  cout << setw(8) << left << "Hand no." << 
    setw(25) << right << "Time" << "\n";
#endif

  int filter[5] = {0, 0, 0, 0, 0};

  for (int i = 0; i < number; i += stepsize)
  {
    int count = (i + stepsize > number ? number - i : stepsize);
    dealsp->noOfTables = count;
    for (int j = 0; j < count; j++)
      strcpy(dealsp->deals[j].cards, deal_list[i+j].remainCards);

    timer.start(count);
    int ret;
    if ((ret = CalcAllTablesPBN(dealsp, -1, filter, resp, parp))
        != RETURN_NO_FAULT)
    {
      cout << "loop_calc: i " << i << ", return " << ret << "\n";
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.printRunning(i+count, number);
#endif

    for (int j = 0; j < count; j++)
    {
      if (compare_TABLE(resp->results[j], table_list[i + j]))
        continue;

      cout << "loop_calc: i " << i << ", j " << j << ": " <<
        "Difference\n\n";
      print_TABLE(resp->results[j] );
      cout << "\n";
      print_TABLE(table_list[i + j]) ;
      cout << "\n";
    }
  }

#ifdef BATCHTIMES
  cout << "\n";
#endif

  return true;
}



bool loop_par(
  int * vul_list,
  ddTableResults * table_list,
  parResults * par_list,
  const int number,
  const int stepsize)
{
  // This is so fast that there is no batch or multi-threaded
  // version. We run it many times just to get meaningful times.

  parResults presp;

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
  ddTableResults * table_list,
  parResultsDealer * dealerpar_list,
  const int number,
  const int stepsize)
{
  // This is so fast that there is no batch or multi-threaded
  // version. We run it many times just to get meaningful times.

  parResultsDealer presp;

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
      cout << "loop_play: i " << i << ": " << "return " << ret << "\n";
      exit(0);
    }
    timer.end();

#ifdef BATCHTIMES
    timer.printRunning(i+count, number);
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

