/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "Memory.hpp"
#include <iostream>
#include <cstdlib>
#include "trans_table/TransTable.hpp"

// After moving ThreadData ownership into SolverContext, Memory no longer
// holds per-thread ThreadData pointers. Keep a minimal implementation that
// tracks configured thread sizes (S/L) and exposes lightweight queries.

Memory::Memory()
{
}

Memory::~Memory()
{
  // Clear configured thread sizes
  Resize(0, DDS_TT_SMALL, 0, 0);
}


void Memory::ReturnThread(const unsigned /*thrId*/)
{
  // No-op: ThreadData is now owned by SolverContext instances. Returning
  // per-thread TT memory requires a SolverContext with the appropriate
  // ThreadData pointer; call sites should perform that via their
  // SolverContext instances.
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Memory::Resize(
  const unsigned n,
  const TTmemory flag,
  const int /*memDefault_MB*/,
  const int /*memMaximum_MB*/) // NOLINT(bugprone-easily-swappable-parameters)
{
  // Resize the lightweight thread size vector. Each entry is a short
  // diagnostic token: "S" = small TT, "L" = large TT.
  threadSizes.resize(n);
  for (unsigned i = 0; i < n; ++i)
    threadSizes[i] = (flag == DDS_TT_SMALL ? "S" : "L");
}


unsigned Memory::NumThreads() const
{
  return static_cast<unsigned>(threadSizes.size());
}


double Memory::MemoryInUseMB(const unsigned /*thrId*/) const
{
  // We can only account for the static relRanksType footprint here. Any
  // transposition table memory is owned by SolverContext/transposition
  // table instances and must be queried via those contexts when available.
  return 8192. * sizeof(relRanksType) / static_cast<double>(1024.);
}


std::string Memory::ThreadSize(const unsigned thrId) const
{
  if (thrId >= threadSizes.size()) return std::string();
  return threadSizes[thrId];
}

