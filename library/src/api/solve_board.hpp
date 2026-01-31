#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>

// C++-only overload exposed via <api/solve_board.hpp> for clients managing solver state.
auto SolveBoard(
    SolverContext& ctx,
    const Deal& dl,
    int target,
    int solutions,
    int mode,
    FutureTricks* futp) -> int;
