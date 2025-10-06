/*
   DDS, a bridge double dummy solver.

   Scaffolding for an instance-scoped API. This is a no-behavior-change
   adapter that allows driving the solver with an explicit context,
   while internally delegating to existing code paths.
*/

#ifndef DDS_SYSTEM_SOLVERCONTEXT_H
#define DDS_SYSTEM_SOLVERCONTEXT_H

struct ThreadData;     // defined in system/Memory.h
struct deal;           // defined in dds/dll.h
struct futureTricks;   // defined in dds/dll.h
#include "trans_table/TransTable.h" // ensure complete type and enums

// Minimal configuration scaffold for future expansion.
// TT configuration without depending on Memory headers.
enum class TTKind { Small, Large };

struct SolverConfig
{
  TTKind ttKind = TTKind::Small;
  int ttMemDefaultMB = 0;
  int ttMemMaximumMB = 0;
};

class SolverContext
{
public:
  explicit SolverContext(ThreadData* thread, SolverConfig cfg = {})
  : thr_(thread), cfg_(cfg) {}

  ~SolverContext();

  ThreadData* thread() const { return thr_; }
  const SolverConfig& config() const { return cfg_; }

  TransTable* transTable() const;
  TransTable* maybeTransTable() const;

  // Dispose and erase the TT instance associated with this thread, if any.
  void DisposeTransTable() const;

  // Lightweight facades used by tests and call sites; no-ops if no TT exists.
  void ResetForSolve() const;   // Calls ResetMemory(TT_RESET_FREE_MEMORY)
  void ClearTT() const;         // Calls ReturnAllMemory()
  void ResizeTT(int defMB, int maxMB) const; // Updates sizes if TT exists

private:
  ThreadData* thr_ = nullptr;
  SolverConfig cfg_{};
};

#endif // DDS_SYSTEM_SOLVERCONTEXT_H
