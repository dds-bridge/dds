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

#include <api/dds.h>

using namespace std;

typedef void (*FduplType)(
  const Boards& bds, vector<int>& uniques, vector<int>& crossrefs);
typedef void (*FcopyType)(const vector<int>& crossrefs);


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
    int num_threads_;
    int sys_mem_mb_;

    unsigned preferred_system_;

    vector<bool> available_system_;
  
    public:

    string get_version(
      int& major,
      int& minor,
      int& patch) const;
    string get_system(int& sys) const;
    string get_bits(int& bits) const;
    string get_compiler(int& comp) const;
    int get_cores() const;
    string get_constructor(int& cons) const;
    string get_threading(int& thr) const;
    int get_memory_max() const { return sys_mem_mb_; }
    int get_num_threads() const { return num_threads_; }

    /**
     * @brief Construct a new System object.
     *
     * Initializes system-dependent state and hardware detection for the solver.
     */
    System();

    /**
     * @brief Destroy the System object and clean up resources.
     *
     * Releases all memory and performs cleanup of system state.
     */
    ~System();

    void reset();

    int register_params(
      const int n_threads,
      const int mem_usable_mb);

    void get_hardware(
      int& core_count,
      unsigned long long& kilobytes_free) const;

    int prefer_threading(const unsigned code);
};

#endif

