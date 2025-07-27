/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TIMESTAT_H
#define DDS_TIMESTAT_H

#include <string>

using namespace std;


/**
 * @brief Timing statistics accumulator for bridge double dummy solver.
 *
 * The TimeStat class accumulates, stores, and manipulates timing statistics
 * (such as cumulative time and squared time) for performance analysis of
 * double dummy solving runs. It is used internally for profiling and optimization.
 */
class TimeStat
{
  private:

    int number;
    long long cum;
    double cumsq;

  public:

    /**
     * @brief Construct a new TimeStat object.
     *
     * Initializes the timing statistics accumulator.
     */
    TimeStat();

    /**
     * @brief Destroy the TimeStat object and clean up resources.
     *
     * Releases any resources and resets the accumulator state.
     */
    ~TimeStat();

    void Reset();

    void Set(const int timeUser);
    void Set(
      const int timeUser,
      const double timesq);

    void operator += (const TimeStat& add);

    bool Used() const;

    string Header() const;
    string Line() const;
};

#endif
