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


class TimeStat
{
  private:

    int number;
    long long cum;
    double cumsq;

  public:

    TimeStat();

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
