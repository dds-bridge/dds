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
#include <array>

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

    array<fptrType, DDS_RUN_SIZE> CallbackSimpleList;
    array<fduplType, DDS_RUN_SIZE> CallbackDuplList;
    array<fsingleType, DDS_RUN_SIZE> CallbackSingleList;
    array<fcopyType, DDS_RUN_SIZE> CallbackCopyList;

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
  
    public:

    string GetVersion(
      int& major,
      int& minor,
      int& patch) const;
    string GetSystem(int& sys) const;
    string GetBits(int& bits) const;
    string GetCompiler(int& comp) const;
    int GetCores() const;
    string GetConstructor(int& cons) const;
    string GetThreading(int& thr) const;
    string GetThreadSizes() const;
    int GetMemoryMax() const { return sysMem_MB; }
    int GetNumThreads() const { return numThreads; }

    /**
     * @brief Construct a new System object.
     *
     * Initializes system-dependent state, threading, and memory management for
     * the double dummy solver.
     */
    System(
      fptrType solve_chunk_common,
      fptrType calc_chunk_common,
      fptrType play_chunk_common,
      fduplType detect_solve_duplicates,
      fduplType detect_calc_duplicates,
      fduplType detect_play_duplicates,
      fsingleType solve_single_common,
      fsingleType calc_single_common,
      fsingleType play_single_common,
      fcopyType copy_solve_single,
      fcopyType copy_calc_single,
      fcopyType copy_play_single
    );

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
};

#endif

