#include "system/SolverContext.h"

// Keep dependencies local to this implementation to avoid include churn.
#include "system/Memory.h"       // for ThreadData definition
#include "trans_table/TransTable.h"
#include "trans_table/TransTableS.h"
#include "trans_table/TransTableL.h"

TransTable* SolverContext::transTable() const
{
  if (!thr_)
    return nullptr;
  if (ownedTT_)
    return ownedTT_.get();

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
    thr_->ttExternallyOwned = true; // ensure Memory won’t delete while adopted
    return ownedTT_.get();
  }

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
  if (thr_)
  {
    thr_->transTable = raw;
    thr_->ttExternallyOwned = false;
  }
  return raw;
}
