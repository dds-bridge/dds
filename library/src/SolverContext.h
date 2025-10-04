/*
   DDS, a bridge double dummy solver.

   Scaffolding for an instance-scoped API. This is a no-behavior-change
   adapter that allows driving the solver with an explicit context,
   while internally delegating to existing code paths.
*/

#ifndef DDS_SOLVERCONTEXT_H
#define DDS_SOLVERCONTEXT_H

// Forward declarations to decouple header from heavy includes.
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
// For scaffolding, it merely wraps a ThreadData* acquired from the legacy Memory.
class SolverContext
{
public:
  explicit SolverContext(ThreadData* thread, SolverConfig cfg = {})
  : thr_(thread), cfg_(cfg) {}

  ThreadData* thread() const { return thr_; }
  const SolverConfig& config() const { return cfg_; }

  // Transitional access to the thread's transposition table.
  // This will later be owned/configured by the context instead of ThreadData.
  TransTable* transTable() const;

private:
  ThreadData* thr_ = nullptr;
  SolverConfig cfg_{};
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
