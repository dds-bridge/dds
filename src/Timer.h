/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TIMING_H
#define DDS_TIMING_H

#include <string>
#include <chrono>

using Clock = std::chrono::steady_clock;
using std::chrono::time_point;

using namespace std;


class Timer
{
  private:

    string name;
    int count;
    long userCum;
    long systCum;

    time_point<Clock> user0;
    clock_t syst0;

  public:

    Timer();

    ~Timer();

    void Reset();

    void SetName(const string& nameIn);

    void Start();

    void End();

    bool Used() const;

    int UserTime() const;

    void operator += (const Timer& add);

    void operator -= (const Timer& deduct);

    string SumLine(
      const Timer& divisor,
      const string& bname = "") const;

    string DetailLine() const;
};

#endif
