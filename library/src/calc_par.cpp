/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "api/calc_par.hpp"
#include "api/dll.h"

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

auto calc_par_from_table(
    const DdTableResults* table_results,
    int vulnerable,
    ParResults* par_results) -> int
{
    // Direct delegation to C API Par function - no solver resources needed
    return Par(table_results, par_results, vulnerable);
}
