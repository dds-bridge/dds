/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TIMESTATLIST_H
#define DDS_TIMESTATLIST_H

#include <string>
#include <vector>

#include <system/time_stat.hpp>


/**
 * @brief Collection of timing statistics for bridge double dummy solver.
 *
 * The TimeStatList class manages a collection of TimeStat objects, providing
 * interfaces for profiling and reporting timing data for various solver components.
 * It is used internally for performance analysis and optimization.
 */
class TimeStatList
{
  private:

    std::vector<TimeStat> list;

    std::string name;

  public:

    /**
     * @brief Construct a new TimeStatList object.
     *
     * Initializes the timing statistics list and prepares for profiling.
     */
    TimeStatList();

    /**
     * @brief Destroy the TimeStatList object and clean up resources.
     *
     * Releases all memory and performs cleanup of the statistics list.
     */
    ~TimeStatList();

    void Reset();

    void Init(
      const std::string& tname,
      const unsigned len);

    void Add(
      const unsigned Pos,
      const TimeStat& add);

    bool Used() const;

    std::string List() const;
    // Release stored data early to speed up shutdown.
    void Clear();
};

#endif
