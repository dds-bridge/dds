/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "Memory.h"


Memory::Memory()
{
  Memory::Reset();
}


Memory::~Memory()
{
}


void Memory::Reset()
{
  nThreads = 0;
  defThrMB = 0;
  maxThrMB = 0;
  Memory::Resize(1);
}


void Memory::ResetThread(const int thrId)
{
  memory[thrId]->transTable.ResetMemory(FREE_THREAD_MEM);
  memory[thrId]->memUsed = Memory::MemoryInUseMB(thrId);
}


void Memory::ReturnThread(const int thrId)
{
  memory[thrId]->transTable.ReturnAllMemory();
  memory[thrId]->memUsed = Memory::MemoryInUseMB(thrId);
}


void Memory::SetThreadSize(
  const int memDefault_MB,
  const int memMaximum_MB)
{
  defThrMB = memDefault_MB;
  maxThrMB = memMaximum_MB;

  for (int i = 0; i < nThreads; i++)
  {
    memory[i]->transTable.SetMemoryDefault(defThrMB);
    memory[i]->transTable.SetMemoryMaximum(maxThrMB);
  }
}


void Memory::Resize(const int n)
{
  if (nThreads == n)
    return;

  if (nThreads > n)
  {
    for (int i = n; i < nThreads; i++)
      delete memory[i];
  }
  else
  {
    for (int i = nThreads; i < n; i++)
      memory[i] = new ThreadData;
  }

  nThreads = n;
  memory.resize(nThreads);
}


ThreadData * Memory::GetPtr(const int thrId)
{
  return memory[thrId];
}


double Memory::MemoryInUseMB(const int thrId) const
{
  return memory[thrId]->transTable.MemoryInUse() +
    8192. * sizeof(relRanksType) / static_cast<double>(1024.);
}


void Memory::ReturnAllMemory()
{
  Memory::Resize(0);
}

