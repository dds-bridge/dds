#include "SolverContext.h"

// Keep dependencies local to this implementation to avoid include churn.
#include "system/Memory.h"       // for ThreadData definition
#include "trans_table/TransTable.h"
#include "trans_table/TransTableS.h"
#include "trans_table/TransTableL.h"


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
  // Temporarily adopt TT ownership so all TT access goes through the context
  // during the solve. Release before returning to preserve legacy layout.
  const bool adopted = ctx.adoptTransTableOwnership();
  const int rc = SolveBoardInternal(ctx.thread(), dl, target, solutions, mode, futp);
  if (adopted)
    ctx.releaseTransTableOwnership();
  return rc;
}

TransTable* SolverContext::transTable() const
{
  // Transitional: ThreadData currently owns the TT pointer.
  // Expose it here so call sites can start using the context.
  if (!thr_)
    return nullptr;
  // If context owns a TT, prefer that.
  if (ownedTT_)
    return ownedTT_.get();

  // Lazily construct a TT owned by the context if not present and configuration is set.
  if (!thr_->transTable)
  {
    TransTable* created = nullptr;
    if (config().ttKind == TTKind::Small)
      created = new TransTableS();
    else
      created = new TransTableL();

    created->SetMemoryDefault(config().ttMemDefaultMB);
    created->SetMemoryMaximum(config().ttMemMaximumMB);
    created->MakeTT();

    ownedTT_.reset(created);
    // Ownership of TT is now transferred to SolverContext (ownedTT_).
    // Set ttExternallyOwned to true so ThreadData/Memory will not delete TT while context owns it.
    thr_->ttExternallyOwned = true;
    return ownedTT_.get();
  }

  // Fallback to thread’s TT while legacy ownership remains.
  return thr_->transTable;
}

TransTable* SolverContext::maybeTransTable() const
{
  if (!thr_)
    return nullptr;
  if (ownedTT_)
    return ownedTT_.get();
  return thr_->transTable;
}

SolverContext::~SolverContext() = default;

bool SolverContext::adoptTransTableOwnership()
{
  if (!thr_ || ownedTT_)
    return false;
  // Steal the pointer; Memory/ThreadData must skip deletion for this thr.
  if (thr_->transTable)
  {
    ownedTT_.reset(thr_->transTable);
    thr_->transTable = nullptr;
    thr_->ttExternallyOwned = true;
    return true;
  }
  return false;
}

TransTable* SolverContext::releaseTransTableOwnership()
{
  if (!ownedTT_)
    return nullptr;
  TransTable* raw = ownedTT_.release();
  // Caller is responsible for reattaching if desired.
  if (thr_)
  {
    thr_->transTable = raw;
    thr_->ttExternallyOwned = false;
  }
  return raw;
}
