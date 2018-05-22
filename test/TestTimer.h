/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_TESTTIMER_H
#define DTEST_TESTTIMER_H

#include <string>
#include <chrono>

using Clock = std::chrono::steady_clock;
using std::chrono::time_point;

using namespace std;


class TestTimer
{
  private:
    string name;
    long count;
    long userCum;
    long userCumOld;
    long sysCum;

    time_point<Clock> user0;
    clock_t sys0;

  public:

    TestTimer();
    ~TestTimer();

    void reset();

    void setname(const string& s);

    void start(const int number = 1);
    void end();

    void printRunning(
      const int reached,
      const int number);
    void printBasic() const;
    void printHands() const;
};

#endif
