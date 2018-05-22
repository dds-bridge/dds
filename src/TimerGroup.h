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

#include "Timer.h"

using namespace std;


class TimerGroup
{
  private:

    vector<Timer> timers;
    string bname;

  public:

    TimerGroup();

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
