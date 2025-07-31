/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_LOOP_H
#define DTEST_LOOP_H

#include "../include/dll.h"


void loop_solve(
  boardsPBN * bop,
  solvedBoards * solvedbdp,
  dealPBN * deal_list,
  futureTricks * fut_list,
  const int number,
  const int stepsize);

bool loop_calc(
  ddTableDealsPBN * dealsp,
  ddTablesRes * resp,
  allParResults * parp,
  dealPBN * deal_list,
  ddTableResults * table_list,
  const int number,
  const int stepsize);

bool loop_par(
  int * vul_list,
  ddTableResults * table_list,
  parResults * par_list,
  const int number,
  const int stepsize);

bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  ddTableResults * table_list,
  parResultsDealer * dealerpar_list,
  const int number,
  const int stepsize);

bool loop_play(
  boardsPBN * bop,
  playTracesPBN * playsp,
  solvedPlays * solvedplp,
  dealPBN * deal_list,
  playTracePBN * play_list,
  solvedPlay * trace_list,
  const int number,
  const int stepsize);

#endif

