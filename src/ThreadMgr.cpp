/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <thread>

#include "ThreadMgr.h"

mutex mtx;
mutex mtxPrint;


ThreadMgr::ThreadMgr()
{
  numRealThreads = 0;
  numMachineThreads = 0;
}


ThreadMgr::~ThreadMgr()
{
}


void ThreadMgr::Reset(const unsigned int nThreads)
{
  if (nThreads > numRealThreads)
  {
    realThreads.resize(nThreads);
    for (unsigned int t = numRealThreads; t < nThreads; t++)
      realThreads[t] = false;
    numRealThreads = nThreads;
  }

  if (nThreads > numMachineThreads)
  {
    machineThreads.resize(nThreads);
    for (unsigned int t = numMachineThreads; t < nThreads; t++)
      machineThreads[t] = -1;
    numMachineThreads = nThreads;
  }
}


int ThreadMgr::Occupy(const unsigned int machineId)
{
  if (machineId >= numMachineThreads)
  {
    numMachineThreads = machineId + 1;
    machineThreads.resize(numMachineThreads);
    for (unsigned int t = machineId; t < numMachineThreads; t++)
      machineThreads[t] = -1;
  }

  if (machineThreads[machineId] != -1)
  {
    // Error: Already in use.
    return -1;
  }

  int res = -1;

  do
  {
    mtx.lock();
    for (unsigned int t = 0; t < numRealThreads; t++)
    {
      if (realThreads[t] == false)
      {
        realThreads[t] = true;
        machineThreads[machineId] = static_cast<int>(t);
        res = static_cast<int>(t);
        break;
      }
    }
    // ThreadMgr::Print("thr.txt", "In Occupy " + 
      // to_string(machineId) + " " + to_string(res));
    mtx.unlock();

    if (res == -1)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  while (res == -1);

  return res;
}


bool ThreadMgr::Release(const unsigned int machineId)
{
  const int r = machineThreads[machineId];
  if (r == -1)
  {
    // Error: Not in use.
    return false;
  }

  if (! realThreads[static_cast<unsigned int>(r)])
  {
    // Error: Refers to a real thread that is not in use.
    return false;
  }

  mtx.lock();
  realThreads[static_cast<unsigned int>(r)] = false;
  machineThreads[machineId] = -1;
  mtx.unlock();
  return true;
}


void ThreadMgr::Print(
  const string& fname,
  const string& tag) const
{
  mtxPrint.lock();
  ofstream fo;
  fo.open(fname, std::ios_base::app);

  fo << tag << 
    ": Real threads occupied (out of " << numRealThreads << "):\n";
  for (unsigned int t = 0; t < numRealThreads; t++)
  {
    if (realThreads[t])
      fo << t << endl;
  }
  fo << endl;

  fo << "Machine threads overview:\n";
  for (unsigned int t = 0; t < numMachineThreads; t++)
  {
    if (machineThreads[t] != -1)
    {
      fo << setw(4) << left << t << machineThreads[t] << endl;
    }
  }
  fo << endl;
  fo.close();
  mtxPrint.unlock();
}

