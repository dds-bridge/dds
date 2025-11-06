/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TIMERGROUP_H
#define DDS_TIMERGROUP_H

#include <string>
#include <vector>

#include "Timer.hpp"

using namespace std;


/**
 * @brief Group of timers for profiling bridge double dummy solver operations.
 *
 * The TimerGroup class manages a collection of Timer objects, allowing
 * simultaneous profiling of multiple solver operations or phases. It supports
 * starting, stopping, and reporting on grouped timing measurements for detailed
 * performance analysis. Used internally for fine-grained profiling.
 */
class TimerGroup
{
  private:

    vector<Timer> timers;
    string bname;

  public:

    /**
     * @brief Construct a new TimerGroup object.
     *
     * Initializes the group of timers and prepares for grouped profiling.
     */
    TimerGroup();

    /**
     * @brief Destroy the TimerGroup object and clean up resources.
     *
     * Releases all memory and resets the group state.
     */
    ~TimerGroup();

    void Reset();

    void SetNames(const string& baseName);

    void Start(const unsigned no);

    void End(const unsigned no);

    bool Used() const;

    void Differentiate();

    void Sum(Timer& sum) const;

    void operator -= (const TimerGroup& deduct);

    string Header() const;
    string DetailHeader() const;
    string SumLine(const Timer& sumTotal) const;
    string TimerLines(const Timer& sumTotal) const;
    string DetailLines() const;
    string DashLine() const;
};

#endif
