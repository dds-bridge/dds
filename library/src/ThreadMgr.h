/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_THREADMGR_H
#define DDS_THREADMGR_H

#include <string>
#include <vector>

using namespace std;


/**
 * @brief Thread manager for bridge double dummy solver.
 *
 * The ThreadMgr class manages the allocation, occupation, and release of threads
 * used in parallel double dummy analysis. It maps solver threads to machine threads,
 * tracks thread usage, and provides utilities for managing parallel execution.
 * ThreadMgr is an internal component and not part of the public API.
 */
class ThreadMgr
{
  private:

    vector<bool> realThreads;
    vector<int> machineThreads;
    unsigned numRealThreads;
    unsigned numMachineThreads;

  public:

    /**
     * @brief Construct a new ThreadMgr object.
     *
     * Initializes thread tracking structures and prepares the manager for use.
     */
    ThreadMgr();

    /**
     * @brief Destroy the ThreadMgr object and clean up resources.
     *
     * Releases all memory and performs cleanup of thread manager state.
     */
    ~ThreadMgr();

    void Reset(const int nThreads);

    int Occupy(const int MachineThrId);

    bool Release(const int MachineThrId);

    void Print(
      const string& fname,
      const string& tag) const;
};

#endif
