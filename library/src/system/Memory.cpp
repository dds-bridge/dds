/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "system/Memory.h"
#include <iostream>
#include <cstdlib>
#include "trans_table/TransTableS.h"
#include "trans_table/TransTableL.h"
#include "SolverContext.h"


Memory::Memory()
{
}


Memory::~Memory()
{
  std::cerr << "[D] Memory::~Memory() start\n";
  Memory::Resize(0, DDS_TT_SMALL, 0, 0);
  std::cerr << "[D] Memory::~Memory() end\n";
}


void Memory::ReturnThread(const unsigned thrId)
{
  SolverContext ctx{memory[thrId]};
  ctx.transTable()->ReturnAllMemory();
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
      delete memory[i]->transTable;
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
      if (flag == DDS_TT_SMALL)
      {
        memory[i]->transTable = new TransTableS;
        threadSizes[i] = "S";
      }
      else
      {
        memory[i]->transTable = new TransTableL;
        threadSizes[i] = "L";
      }

      // Route TT setup via SolverContext for consistency
      {
        SolverContext ctx{memory[i]};
        ctx.transTable()->SetMemoryDefault(memDefault_MB);
        ctx.transTable()->SetMemoryMaximum(memMaximum_MB);
        ctx.transTable()->MakeTT();
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
  return ctx.transTable()->MemoryInUse() +
    8192. * sizeof(relRanksType) / static_cast<double>(1024.);
}


string Memory::ThreadSize(const unsigned thrId) const
{
  return threadSizes[thrId];
}

