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
    string name_;
    long count_;
    long user_cum_;
    long user_cum_old_;
    long sys_cum_;

    time_point<Clock> user0_;
    clock_t sys0_;

  public:

    TestTimer();
    ~TestTimer();

    void reset();

    void set_name(const string& s);

    void start(const int number = 1);
    void end();

    void print_running(
      const int reached,
      const int number);
    void print_basic() const;
    void print_hands() const;
};

#endif
