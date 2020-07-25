/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "Memory.h"


Memory::Memory()
{
}


Memory::~Memory()
{
  Memory::Resize(0, DDS_TT_SMALL, 0, 0);
}


void Memory::ReturnThread(const unsigned thrId)
{
  memory[thrId]->transTable->ReturnAllMemory();
  memory[thrId]->memUsed = Memory::MemoryInUseMB(thrId);
}


void Memory::Resize(
  const unsigned n,
  const TTmemory flag,
  const int memDefault_MB,
  const int memMaximum_MB)
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

      memory[i]->transTable->SetMemoryDefault(memDefault_MB);
      memory[i]->transTable->SetMemoryMaximum(memMaximum_MB);

      memory[i]->transTable->MakeTT();
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
  return memory[thrId]->transTable->MemoryInUse() +
    8192. * sizeof(relRanksType) / static_cast<double>(1024.);
}


string Memory::ThreadSize(const unsigned thrId) const
{
  return threadSizes[thrId];
}

