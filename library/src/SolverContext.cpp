#include "SolverContext.h"

// Keep dependencies local to this implementation to avoid include churn.
#include "system/Memory.h"       // for ThreadData definition
#include "trans_table/TransTable.h"


// Legacy global (to be removed in future phases); used only for scaffolding.
// Forward declaration of the existing internal entry point to avoid heavy includes.
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
  // Forward directly to the existing internal implementation.
  // No behavior changes.
  return SolveBoardInternal(ctx.thread(), dl, target, solutions, mode, futp);
}

TransTable* SolverContext::transTable() const
{
  // Transitional: ThreadData currently owns the TT pointer.
  // Expose it here so call sites can start using the context.
  return thr_ ? thr_->transTable : nullptr;
}
