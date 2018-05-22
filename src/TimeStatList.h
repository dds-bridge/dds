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

#include "TimeStat.h"

using namespace std;


class TimeStatList
{
  private:

    vector<TimeStat> list;

    string name;

  public:

    TimeStatList();

    ~TimeStatList();

    void Reset();

    void Init(
      const string& tname,
      const unsigned len);

    void Add(
      const unsigned pos,
      const TimeStat& add);

    bool Used() const;

    string List() const;
};

#endif
