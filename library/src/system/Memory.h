/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_MEMORY_H
#define DDS_MEMORY_H

#include <vector>

#include <dds/dds.h>
#include <moves/Moves.h>
#include "ThreadData.h"


#ifdef DDS_AB_STATS
  #include "ABstats.h"
#endif

#ifdef DDS_TIMING
  #include "TimerList.h"
#endif


/**
 * @brief Thread-local memory manager for bridge double dummy solver.
 *
 * The Memory class manages per-thread resources, including allocation and cleanup
 * of thread-local data structures required for double dummy analysis. It provides
 * interfaces for resizing, accessing, and reporting memory usage for each thread.
 * Memory is an internal component and not part of the public API.
 */
class Memory
{
  private:
    // Per static_memory_genmove.md step one: ThreadData instances are no
    // longer stored in a central Memory::memory vector. ThreadData will be
    // owned/allocated by the SolverContext (as a member) and passed down to
    // call sites. This header therefore no longer keeps per-thread pointers.

    // Keep a lightweight record of configured thread sizes for diagnostics
    // and reporting. This replaces the previous `memory` vector which held
    // full ThreadData instances.
    std::vector<std::string> threadSizes;

  public:

    /**
     * @brief Construct a new Memory object.
     *
     * Initializes thread-local memory tracking and prepares for allocation.
     */
    Memory();

    /**
     * @brief Destroy the Memory object and clean up resources.
     *
     * Releases all memory and performs cleanup of thread-local resources.
     */
    ~Memory();

    void ReturnThread(const unsigned thrId);

    void Resize(
      const unsigned n,
      const TTmemory flag,
      const int memDefault_MB,
      const int memMaximum_MB); // NOLINT(bugprone-easily-swappable-parameters)

    unsigned NumThreads() const;
    double MemoryInUseMB(const unsigned thrId) const;

    string ThreadSize(const unsigned thrId) const;
};

#endif
