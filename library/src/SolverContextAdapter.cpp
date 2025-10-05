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
  // Temporarily adopt TT ownership so all TT access goes through the context
  // during the solve. Release before returning to preserve legacy layout.
  const bool adopted = ctx.adoptTransTableOwnership();
  const int rc = SolveBoardInternal(ctx.thread(), dl, target, solutions, mode, futp);
  if (adopted)
    ctx.releaseTransTableOwnership();
  return rc;
}
