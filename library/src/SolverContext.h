/*
   DDS, a bridge double dummy solver.

   Scaffolding for an instance-scoped API. This is a no-behavior-change
   adapter that allows driving the solver with an explicit context,
   while internally delegating to existing code paths.
*/

#ifndef DDS_SOLVERCONTEXT_H
#define DDS_SOLVERCONTEXT_H

// Forward declarations to decouple header from heavy includes.
#include <memory>
struct ThreadData;     // defined in system/Memory.h
struct deal;           // defined in dds/dll.h
struct futureTricks;   // defined in dds/dll.h
class TransTable;      // defined in trans_table/TransTable.h

// Minimal configuration scaffold for future expansion.
struct SolverConfig
{
  // Placeholder for future tunables (TT size, buffers, etc.).
};

// An instance-scoped handle that owns (or references) all per-solve mutable state.
// For scaffolding, it wraps a ThreadData* acquired from the legacy Memory.
// It also provides optional ownership of the TransTable to prepare for
// transitioning away from ThreadData-managed ownership.
class SolverContext
{
public:
  explicit SolverContext(ThreadData* thread, SolverConfig cfg = {})
  : thr_(thread), cfg_(cfg) {}

  // Defined in .cpp to keep unique_ptr<TransTable> destruction with a complete type
  ~SolverContext();

  ThreadData* thread() const { return thr_; }
  const SolverConfig& config() const { return cfg_; }

  // Transitional access to the thread's transposition table.
  // This will later be owned/configured by the context instead of ThreadData.
  TransTable* transTable() const;

  // Ownership scaffolding (no-op unless explicitly called):
  // Adopt ownership of the current thread's TransTable instance so it will be
  // destroyed with this context instead of by Memory/ThreadData. This requires
  // Memory to skip deletion (guarded by a flag on ThreadData). Not called by
  // default to avoid changing lifetime behavior.
  bool adoptTransTableOwnership();
  // Release ownership back to non-owned state; keeps thr_->transTable usable.
  TransTable* releaseTransTableOwnership();

private:
  ThreadData* thr_ = nullptr;
  SolverConfig cfg_{};
  // Optional owned TT instance; by default null so legacy ownership remains.
  std::unique_ptr<TransTable> ownedTT_{};
};

// New additive API (no behavior change):
// Solve a board using an explicit instance context.
// Semantics and return codes match the legacy SolveBoard.
int SolveBoardWithContext(
  SolverContext& ctx,
  const deal& dl,
  int target,
  int solutions,
  int mode,
  futureTricks* futp);

#endif // DDS_SOLVERCONTEXT_H
