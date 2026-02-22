/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>

/// @file loop.hpp
/// @brief Main test loop implementations for DDS solver testing.
/// 
/// Executes test loops for various solver operations (solve, calc,
/// par score, play tracing) over sets of deals.

/// Solve loop: execute solve_board for multiple deals.
/// @param bop User-level boards structure (output)
/// @param solvedbdp Solved boards structure (output)
/// @param deal_list Input deals in PBN format
/// @param fut_list Expected future tricks results
/// @param number Number of deals in test set
/// @param stepsize Reporting frequency
void loop_solve(
    BoardsPBN * bop,
    SolvedBoards * solvedbdp,
    DealPBN * deal_list,
    FutureTricks * fut_list,
    const int number,
    const int stepsize);

/// Calculate loop: execute calc_dd_table for multiple deals.
bool loop_calc(
    DdTableDealsPBN * dealsp,
    DdTablesRes * resp,
    AllParResults * parp,
    DealPBN * deal_list,
    DdTableResults * table_list,
    const int number,
    const int stepsize);

/// PAR loop: calculate PAR scores for multiple deals.
bool loop_par(
    int * vul_list,
    DdTableResults * table_list,
    ParResults * par_list,
    const int number,
    const int stepsize);

/// Dealer PAR loop: calculate dealer PAR scores.
bool loop_dealerpar(
    int * dealer_list,
    int * vul_list,
    DdTableResults * table_list,
    ParResultsDealer * dealerpar_list,
    const int number,
    const int stepsize);

/// Play loop: execute play_trace for multiple deals.
bool loop_play(
    BoardsPBN * bop,
    PlayTracesPBN * playsp,
    SolvedPlays * solvedplp,
    DealPBN * deal_list,
    PlayTracePBN * play_list,
    SolvedPlay * trace_list,
    const int number,
    const int stepsize);
