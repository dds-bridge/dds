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


void ThreadMgr::Reset(const int nThreads)
{
  const unsigned n = static_cast<unsigned>(nThreads);
  if (n > numRealThreads)
  {
    realThreads.resize(n);
    for (unsigned t = numRealThreads; t < n; t++)
      realThreads[t] = false;
    numRealThreads = n;
  }

  if (n > numMachineThreads)
  {
    machineThreads.resize(n);
    for (unsigned t = numMachineThreads; t < n; t++)
      machineThreads[t] = -1;
    numMachineThreads = n;
  }
}


int ThreadMgr::Occupy(const int machineId)
{
  const unsigned m = static_cast<unsigned>(machineId);
  if (m >= numMachineThreads)
  {
    numMachineThreads = m + 1;
    machineThreads.resize(numMachineThreads);
    for (unsigned t = m; t < numMachineThreads; t++)
      machineThreads[t] = -1;
  }

  if (machineThreads[m] != -1)
  {
    // Error: Already in use.
    return -1;
  }

  int res = -1;

  do
  {
    mtx.lock();
    for (unsigned t = 0; t < numRealThreads; t++)
    {
      if (realThreads[t] == false)
      {
        const int ti = static_cast<int>(t);
        realThreads[t] = true;
        machineThreads[m] = ti;
        res = ti;
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


bool ThreadMgr::Release(const int machineId)
{
  mtx.lock();

  bool ret;
  const unsigned m = static_cast<unsigned>(machineId);
  const int r = machineThreads[m];
  const unsigned ru = static_cast<unsigned>(r);

  if (r == -1)
  {
    // Error: Not in use.
    ret = false;
  }
  else if (! realThreads[ru])
  {
    // Error: Refers to a real thread that is not in use.
    ret = false;
  }
  else
  {
    realThreads[ru] = false;
    machineThreads[m] = -1;
    ret = true;
  }

  mtx.unlock();
  return ret;
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
  for (unsigned t = 0; t < numRealThreads; t++)
  {
    if (realThreads[t])
      fo << t << endl;
  }
  fo << endl;

  fo << "Machine threads overview:\n";
  for (unsigned t = 0; t < numMachineThreads; t++)
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

