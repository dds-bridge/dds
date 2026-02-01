/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_LOOP_H
#define DTEST_LOOP_H

#include <api/dll.h>


void loop_solve(
  BoardsPBN * bop,
  SolvedBoards * solvedbdp,
  DealPBN * deal_list,
  FutureTricks * fut_list,
  const int number,
  const int stepsize);

bool loop_calc(
  DdTableDealsPBN * dealsp,
  DdTablesRes * resp,
  AllParResults * parp,
  DealPBN * deal_list,
  DdTableResults * table_list,
  const int number,
  const int stepsize);

bool loop_par(
  int * vul_list,
  DdTableResults * table_list,
  ParResults * par_list,
  const int number,
  const int stepsize);

bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  DdTableResults * table_list,
  ParResultsDealer * dealerpar_list,
  const int number,
  const int stepsize);

bool loop_play(
  BoardsPBN * bop,
  PlayTracesPBN * playsp,
  SolvedPlays * solvedplp,
  DealPBN * deal_list,
  PlayTracePBN * play_list,
  SolvedPlay * trace_list,
  const int number,
  const int stepsize);

#endif

