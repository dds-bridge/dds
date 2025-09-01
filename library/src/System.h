/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SYSTEM_H
#define DDS_SYSTEM_H

/*
   This class encapsulates all the system-dependent stuff.
 */

#include <string>
#include <vector>

#include "dds/dds.h"

using namespace std;

typedef void (*fptrType)(const int thid);
typedef void (*fduplType)(
  const boards& bds, vector<int>& uniques, vector<int>& crossrefs);
typedef void (*fsingleType)(const int thid, const int bno);
typedef void (*fcopyType)(const vector<int>& crossrefs);


/**
 * @brief System-dependent manager for bridge double dummy solver.
 *
 * The System class encapsulates all system-dependent logic, including management
 * of threading, memory allocation, and concurrency models for the solver. It
 * provides an abstraction layer for different threading backends and system
 * resources, optimizing solver execution for the host environment. System is an
 * internal component and not part of the public API.
 */
class System
{
  private:

    RunMode runCat; // SOLVE / CALC / PLAY

    int numThreads;
    int sysMem_MB;

    unsigned preferredSystem;

    vector<bool> availableSystem;

    vector<fptrType> CallbackSimpleList;
    vector<fduplType> CallbackDuplList;
    vector<fsingleType> CallbackSingleList;
    vector<fcopyType> CallbackCopyList;

    typedef int (System::*RunPtr)();
    vector<RunPtr> RunPtrList;

    fptrType fptr;

    boards const * bop;

    int RunThreadsBasic();
    int RunThreadsBoost();
    int RunThreadsOpenMP();
    int RunThreadsGCD();
    int RunThreadsWinAPI();
    int RunThreadsSTL();
    int RunThreadsTBB();
    int RunThreadsSTLIMPL();
    int RunThreadsPPLIMPL();

    string GetVersion(
      int& major,
      int& minor,
      int& patch) const;
    string GetSystem(int& sys) const;
    string GetBits(int& bits) const;
    string GetCompiler(int& comp) const;
    string GetCores(int& comp) const;
    string GetConstructor(int& cons) const;
    string GetThreading(int& thr) const;
    string GetThreadSizes(char * c) const;


  public:
    /**
     * @brief Construct a new System object.
     *
     * Initializes system-dependent state, threading, and memory management for
     * the double dummy solver.
     */
    System();

    /**
     * @brief Destroy the System object and clean up resources.
     *
     * Releases all memory and performs cleanup of system state.
     */
    ~System();

    void Reset();

    int RegisterParams(
      const int nThreads,
      const int mem_usable_MB);

    int RegisterRun(
      const RunMode r,
      const boards& bop);

    bool IsSingleThreaded() const;

    bool IsIMPL() const;

    bool ThreadOK(const int thrId) const;

    void GetHardware(
      int& ncores,
      unsigned long long& kilobytesFree) const;

    int PreferThreading(const unsigned code);

    int RunThreads();

    string str(DDSInfo * info) const;
};

#endif

