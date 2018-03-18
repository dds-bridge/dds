/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_TIMING_H
#define DDS_TIMING_H

#include <string>

#include <time.h>
#include <inttypes.h>

#ifdef _MSC_VER
  #include <windows.h>
#else
  #include <sys/time.h>
#endif

using namespace std;


class Timer
{
  private:

    clock_t systTimes0;
    clock_t systTimes1;

#ifdef _MSC_VER
    LARGE_INTEGER userTimes0;
    LARGE_INTEGER userTimes1;
#else
    timeval userTimes0;
    timeval userTimes1;
#endif

    string name;
    int count;
    int64_t userCum;
    double systCum;

  public:

    Timer();

    ~Timer();

    void Reset();

    void SetName(const string& s);

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
