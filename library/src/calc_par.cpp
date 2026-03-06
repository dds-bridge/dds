/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "api/calc_par.hpp"
#include "api/dll.h"

/**
 * @brief Calculate par score and contracts for a deal table (without context).
 *
 * This version creates a temporary SolverContext internally (or uses global
 * resources via the C API). For now, this delegates to the C API CalcPar.
 *
 * @param table_deal Deal represented as card holdings for each hand
 * @param vulnerable Vulnerability (0=None, 1=Both, 2=NS, 3=EW)
 * @param table_results Output: double dummy table results
 * @param par_results Output: par score and contract strings
 * @return Error code (RETURN_NO_FAULT on success)
 */
auto calc_par(
    const DdTableDeal& table_deal,
    int vulnerable,
    DdTableResults* table_results,
    ParResults* par_results) -> int
{
    // For now, delegate to C API CalcPar which handles the full computation
    // (CalcDDtable + Par). In future, this could create a temporary context
    // if CalcDDtable gains context support.
    return CalcPar(
        table_deal,
        vulnerable,
        table_results,
        par_results
    );
}

/**
 * @brief Calculate par score and contracts with explicit solver context.
 *
 * C++ overload that accepts an explicit SolverContext. For now, the context
 * parameter is accepted for API consistency but not yet utilized, as the
 * underlying CalcDDtable function doesn't yet support context-based solving.
 * This will be enhanced when CalcDDtable gains context support.
 *
 * @param ctx Solver context (accepted but not yet utilized)
 * @param table_deal Deal represented as card holdings for each hand
 * @param vulnerable Vulnerability (0=None, 1=Both, 2=NS, 3=EW)
 * @param table_results Output: double dummy table results
 * @param par_results Output: par score and contract strings
 * @return Error code (RETURN_NO_FAULT on success)
 */
auto calc_par(
    [[maybe_unused]] SolverContext& ctx,
    const DdTableDeal& table_deal,
    int vulnerable,
    DdTableResults* table_results,
    ParResults* par_results) -> int
{
    // Currently delegates to C API CalcPar. The context parameter is accepted
    // to provide a consistent API that can be enhanced in the future without
    // breaking client code.
    // TODO: Once CalcDDtable supports SolverContext, use ctx here for
    //       context-aware table computation.
    return CalcPar(
        table_deal,
        vulnerable,
        table_results,
        par_results
    );
}

/**
 * @brief Calculate par from pre-computed double dummy table.
 *
 * When DD table is already available, this function computes only the par
 * analysis without recalculating the table. This is a thin wrapper around
 * the C API Par() function and does not require SolverContext.
 *
 * @param table_results Input: pre-computed double dummy table
 * @param vulnerable Vulnerability (0=None, 1=Both, 2=NS, 3=EW)
 * @param par_results Output: par score and contract strings
 * @return Error code (RETURN_NO_FAULT on success)
 */
auto calc_par_from_table(
    const DdTableResults* table_results,
    int vulnerable,
    ParResults* par_results) -> int
{
    // Direct delegation to C API Par function - no solver resources needed
    return Par(table_results, par_results, vulnerable);
}
