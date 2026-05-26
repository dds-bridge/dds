/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <memory>
#include <api/dll.h>
#ifdef _VCXPROJ
    #include <dds.hpp>
#else
    #include <dds/dds.hpp>
#endif 

void solve_legacy(const Deal& deal)
{
    SetMaxThreads(4);
    SetResources(2000, 4);

    FutureTricks fut;
    int res = SolveBoard(deal, -1, 3, 0, &fut, 0);
    (void)res;

    FreeMemory();
}

void solve_modern(const Deal& deal)
{
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_default_mb_ = 2000;
    cfg.tt_mem_maximum_mb_ = 2000;

    SolverContext ctx(cfg);

    FutureTricks fut;
    int res = solve_board(ctx, deal, -1, 3, 0, &fut);
    (void)res;
}

auto main() -> int
{
    Deal deal{};
    solve_legacy(deal);
    solve_modern(deal);
    return 0;
}
