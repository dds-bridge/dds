#include <solver_context/SolverContext.h>
#include "SolverIF.h"

int SolveBoard(
  SolverContext& ctx,
  const deal& dl,
  int target,
  int solutions,
  int mode,
  futureTricks* futp)
{
  // Use ThreadData-attached TT so all contexts created in lower layers
  // observe the same table. No ownership adoption to avoid duplication.
  return SolveBoardInternal(ctx, dl, target, solutions, mode, futp);
}
