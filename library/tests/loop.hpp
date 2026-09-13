/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <utility>
#include <vector>

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
/// @param stepsize Boards per solve batch (typically `MAXNOOFBOARDS`)
/// @param board_times When non-null, appends per-deal timings for every batch
///        with file-relative board indices (for `dtest -r`)
/// @return false on DDS API fault or first expected-result mismatch
auto loop_solve(
    BoardsPBN * bop,
    SolvedBoards * solvedbdp,
    DealPBN * deal_list,
    FutureTricks * fut_list,
    const int number,
    const int stepsize,
    std::vector<std::pair<int, int>>* board_times = nullptr) -> bool;

/// Calculate loop: CalcAllTablesPBNX in batches of `stepsize` deals so progress
/// can update between batches. Uses the unbounded X API (no legacy
/// DdTableDealsPBN / DdTablesRes batch structs).
/// @param deal_list Input deals in PBN format
/// @param table_list Expected DD table results
/// @param number Number of deals in the test set
/// @param stepsize Deals per CalcAllTablesPBNX batch (typically `MAXNOOFBOARDS`)
/// @return false on DDS API fault or first expected-result mismatch
auto loop_calc(
    DealPBN * deal_list,
    DdTableResults * table_list,
    const int number,
    const int stepsize) -> bool;

/// PAR loop: calculate PAR scores for multiple deals.
/// @return false on DDS API fault or first expected-result mismatch
auto loop_par(
    int * vul_list,
    DdTableResults * table_list,
    ParResults * par_list,
    const int number,
    const int stepsize) -> bool;

/// Dealer PAR loop: calculate dealer PAR scores.
/// @return false on DDS API fault or first expected-result mismatch
auto loop_dealerpar(
    int * dealer_list,
    int * vul_list,
    DdTableResults * table_list,
    ParResultsDealer * dealerpar_list,
    const int number,
    const int stepsize) -> bool;

/// Play loop: execute play_trace for multiple deals.
/// @return false on DDS API fault or first expected-result mismatch
auto loop_play(
    BoardsPBN * bop,
    PlayTracesPBN * playsp,
    SolvedPlays * solvedplp,
    DealPBN * deal_list,
    PlayTracePBN * play_list,
    SolvedPlay * trace_list,
    const int number,
    const int stepsize) -> bool;
