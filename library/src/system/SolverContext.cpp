#include "system/SolverContext.h"

// Keep dependencies local to this implementation to avoid include churn.
#include "system/Memory.h"       // for ThreadData definition
#include "trans_table/TransTableS.h"
#include "trans_table/TransTableL.h"
#include "data_types/dds.h" // THREADMEM_* defaults
#include <cstdlib>
#include <iostream>
#include <unordered_map>

namespace {
// Central registry mapping ThreadData* to its TransTable instance.
// Kept as a function-local static to avoid global non-const warning.
static std::unordered_map<ThreadData*, TransTable*>& registry()
{
  static std::unordered_map<ThreadData*, TransTable*> map;
  return map;
}
}

TransTable* SolverContext::transTable() const
{
  if (!thr_)
    return nullptr;
  auto it = registry().find(thr_);
  if (it == registry().end() || it->second == nullptr)
  {
    TransTable* created = nullptr;
    // Prefer thread-stored configuration determined by Init/Memory
    TTKind kind = (thr_->ttType == DDS_TT_SMALL ? TTKind::Small : TTKind::Large);
    if (kind == TTKind::Small)
      created = new TransTableS();
    else
      created = new TransTableL();

    int defMB = (cfg_.ttMemDefaultMB > 0 ? cfg_.ttMemDefaultMB : thr_->ttMemDefault_MB);
    int maxMB = (cfg_.ttMemMaximumMB > 0 ? cfg_.ttMemMaximumMB : thr_->ttMemMaximum_MB);
    // Fallback to conservative defaults if absent
    if (defMB <= 0 || maxMB <= 0)
    {
      if (kind == TTKind::Small)
      {
        defMB = THREADMEM_SMALL_DEF_MB;
        maxMB = THREADMEM_SMALL_MAX_MB;
      }
      else
      {
        defMB = THREADMEM_LARGE_DEF_MB;
        maxMB = THREADMEM_LARGE_MAX_MB;
      }
    }

    // Optional environment overrides for tuning
    if (const char* s = std::getenv("DDS_TT_DEFAULT_MB"))
    {
      int v = std::atoi(s);
      if (v > 0)
        defMB = v;
    }
    if (const char* s = std::getenv("DDS_TT_LIMIT_MB"))
    {
      int v = std::atoi(s);
      if (v > 0)
        maxMB = std::min(maxMB, v);
    }
    if (maxMB < defMB)
      maxMB = defMB;

    // Optional one-time debug print per creation
    if (const char* dbg = std::getenv("DDS_DEBUG_TT_CREATE"))
    {
      if (*dbg)
      {
        std::cerr << "[DDS] TT create: kind="
                  << (kind == TTKind::Small ? 'S' : 'L')
                  << " defMB=" << defMB
                  << " maxMB=" << maxMB
                  << std::endl;
      }
    }

    created->SetMemoryDefault(defMB);
    created->SetMemoryMaximum(maxMB);
    created->MakeTT();

    // Attach to registry
    registry()[thr_] = created;
  }
  return registry()[thr_];
}

TransTable* SolverContext::maybeTransTable() const
{
  if (!thr_)
    return nullptr;
  auto it = registry().find(thr_);
  return (it == registry().end() ? nullptr : it->second);
}

void SolverContext::DisposeTransTable() const
{
  auto it = registry().find(thr_);
  if (it != registry().end())
  {
    delete it->second;
    it->second = nullptr;
    registry().erase(it);
  }
}

SolverContext::~SolverContext() = default;

void SolverContext::ResetForSolve() const
{
  if (auto* tt = maybeTransTable())
    tt->ResetMemory(TT_RESET_FREE_MEMORY);
}

void SolverContext::ClearTT() const
{
  if (auto* tt = maybeTransTable())
    tt->ReturnAllMemory();
}

void SolverContext::ResizeTT(int defMB, int maxMB) const
{
  if (auto* tt = maybeTransTable())
  {
    if (maxMB < defMB) maxMB = defMB;
    tt->SetMemoryDefault(defMB);
    tt->SetMemoryMaximum(maxMB);
  }
}
