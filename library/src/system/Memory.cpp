/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "system/Memory.h"
#include <iostream>
#include <cstdlib>
#include "trans_table/TransTable.h"
#include "system/SolverContext.h"


Memory::Memory()
{
}


Memory::~Memory()
{
  Memory::Resize(0, DDS_TT_SMALL, 0, 0);
}


void Memory::ReturnThread(const unsigned thrId)
{
  SolverContext ctx{memory[thrId]};
  if (auto* tt = ctx.maybeTransTable())
    tt->return_all_memory();
  memory[thrId]->memUsed = Memory::MemoryInUseMB(thrId);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void Memory::Resize(
  const unsigned n,
  const TTmemory flag,
  const int memDefault_MB,
  const int memMaximum_MB) // NOLINT(bugprone-easily-swappable-parameters)
{
  if (memory.size() == n)
    return;

  if (memory.size() > n)
  {
    // Downsize.
    for (unsigned i = n; i < memory.size(); i++)
    {
      if (memory[i])
      {
        // Return and release TT memory via context API.
        SolverContext ctx{memory[i]};
        if (auto* tt = ctx.maybeTransTable())
          tt->return_all_memory();
        ctx.DisposeTransTable();
      }
      if (memory[i])
        delete memory[i];
    }
    memory.resize(static_cast<unsigned>(n));
    threadSizes.resize(static_cast<unsigned>(n));
  }
  else
  {
    // Upsize.
    unsigned oldSize = memory.size();
    memory.resize(n);
    threadSizes.resize(n);
    for (unsigned i = oldSize; i < n; i++)
    {
      memory[i] = new ThreadData();
      // Context will lazily construct and own the TT
      threadSizes[i] = (flag == DDS_TT_SMALL ? "S" : "L");

      // Defer TT configuration to SolverContext via ThreadData fields
      memory[i]->ttType = flag;
      memory[i]->ttMemDefault_MB = memDefault_MB;
      memory[i]->ttMemMaximum_MB = memMaximum_MB;
      // Optional eager creation to mimic legacy behavior for debugging
      if (std::getenv("DDS_TT_EAGER"))
      {
        SolverContext ctx{memory[i]};
        (void)ctx.transTable(); // force creation
      }
    }
  }
}


unsigned Memory::NumThreads() const
{
  return static_cast<unsigned>(memory.size());
}


ThreadData * Memory::GetPtr(const unsigned thrId)
{
  if (thrId >= memory.size())
  {
    cout << "Memory::GetPtr: " << thrId << " vs. " << memory.size() << endl;
    exit(1);
  }
  return memory[thrId];
}


double Memory::MemoryInUseMB(const unsigned thrId) const
{
  SolverContext ctx{memory[thrId]};
  double ttMem = 0.0;
  if (auto* tt = ctx.maybeTransTable())
    ttMem = tt->memory_in_use();
  return ttMem + 8192. * sizeof(relRanksType) / static_cast<double>(1024.);
}


string Memory::ThreadSize(const unsigned thrId) const
{
  return threadSizes[thrId];
}

