#include <solver_context/solver_context.hpp>
#include "solver_if.hpp"

int SolveBoard(
  SolverContext& ctx,
  const Deal& dl,
  int target,
  int solutions,
  int mode,
  FutureTricks* futp)
{
  // Use ThreadData-attached TT so all contexts created in lower layers
  // observe the same table. No ownership adoption to avoid duplication.
  return SolveBoardInternal(ctx, dl, target, solutions, mode, futp);
}
