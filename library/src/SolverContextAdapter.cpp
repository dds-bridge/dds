#include "system/SolverContext.h"

// Legacy internal entry point; avoid heavy includes here.
int SolveBoardInternal(
  ThreadData * thrp,
  const deal& dl,
  const int target,
  const int solutions,
  const int mode,
  futureTricks * futp);

int SolveBoardWithContext(
  SolverContext& ctx,
  const deal& dl,
  int target,
  int solutions,
  int mode,
  futureTricks* futp)
{
  // Use ThreadData-attached TT so all contexts created in lower layers
  // observe the same table. No ownership adoption to avoid duplication.
  return SolveBoardInternal(ctx.thread(), dl, target, solutions, mode, futp);
}
