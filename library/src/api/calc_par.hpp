/**
 * @file calc_par.hpp
 * @brief C++ interface for par calculation with explicit solver context.
 *
 * This header provides C++ overloads for par calculation that accept explicit
 * SolverContext, allowing clients to manage solver state across multiple operations.
 *
 * @copyright (C) 2006-2014 by Bo Haglund / 2014-2018 by Bo Haglund & Soren Hein.
 * @see LICENSE and README.
 */

#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>

// Naming note: New C++ APIs in DDS 3 use snake_case (calc_par, calc_par_from_table).
// Legacy C/C++ wrapper APIs may still use historical PascalCase names (for example SolveBoard).

/**
 * @brief Calculate par score and contracts for a deal table.
 *
 * Computes the double dummy table for the given deal, then calculates
 * par score and contracts based on vulnerability. This version creates
 * a temporary SolverContext internally.
 *
 * @param table_deal Deal represented as card holdings for each hand
 * @param vulnerable Vulnerability (0=None, 1=Both, 2=NS, 3=EW)
 * @param table_results Output: double dummy table results
 * @param par_results Output: par score and contract strings
 * @return Error code (RETURN_NO_FAULT on success)
 *
 * @note This function is equivalent to calling CalcDDtable + Par from C API
 */
auto calc_par(
    const DdTableDeal& table_deal,
    int vulnerable,
    DdTableResults* table_results,
    ParResults* par_results) -> int;

/**
 * @brief Calculate par score and contracts with explicit solver context.
 *
 * C++ overload that accepts an explicit SolverContext. The context enables
 * efficient reuse of allocated solver resources (memory buffers, threading
 * state, and TT allocation metadata) across multiple par calculations.
 *
 * @note DD table calculation evaluates all 5 strains and resets TT contents on
 * each trump change, so this path primarily reuses allocations/thread state
 * rather than TT entries between strains.
 * 
 * Internally computes the DD table using context-aware calc_dd_table(),
 * then calculates par score from the table.
 *
 * @param ctx Solver context for resource management and TT reuse
 * @param table_deal Deal represented as card holdings for each hand
 * @param vulnerable Vulnerability (0=None, 1=Both, 2=NS, 3=EW)
 * @param table_results Output: double dummy table results
 * @param par_results Output: par score and contract strings
 * @return Error code (RETURN_NO_FAULT on success)
 */
auto calc_par(
    SolverContext& ctx,
    const DdTableDeal& table_deal,
    int vulnerable,
    DdTableResults* table_results,
    ParResults* par_results) -> int;

/**
 * @brief Calculate par from pre-computed double dummy table.
 *
 * When DD table is already available, this function computes only the par
 * analysis without recalculating the table. Does not require SolverContext.
 * This is a thin wrapper around the C API Par() function.
 *
 * @param table_results Input: pre-computed double dummy table
 * @param vulnerable Vulnerability (0=None, 1=Both, 2=NS, 3=EW)
 * @param par_results Output: par score and contract strings
 * @return Error code (RETURN_NO_FAULT on success)
 */
auto calc_par_from_table(
    const DdTableResults* table_results,
    int vulnerable,
    ParResults* par_results) -> int;
