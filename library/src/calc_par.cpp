/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <api/calc_par.hpp>
#include <api/calc_dd_table.hpp>
#include <api/dll.h>

auto calc_par(
    const DdTableDeal& table_deal,
    int vulnerable,
    DdTableResults* table_results,
    ParResults* par_results) -> int
{
    // For now, delegate to C API CalcPar which handles the full computation
    // (CalcDDtable + Par).
    return CalcPar(
        table_deal,
        vulnerable,
        table_results,
        par_results
    );
}

auto calc_par(
    SolverContext& ctx,
    const DdTableDeal& table_deal,
    int vulnerable,
    DdTableResults* table_results,
    ParResults* par_results) -> int
{
    // Context-aware path: compute table with context, then calculate par
    int res = calc_dd_table(ctx, table_deal, table_results);

    if (res != RETURN_NO_FAULT) {
        return res;
    }

    // Par calculation doesn't need context (pure computation on table results)
    return Par(table_results, par_results, vulnerable);
}

auto calc_par_from_table(
    const DdTableResults* table_results,
    int vulnerable,
    ParResults* par_results) -> int
{
    // Direct delegation to C API Par function - no solver resources needed
    return Par(table_results, par_results, vulnerable);
}
