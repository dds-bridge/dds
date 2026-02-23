/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>

#include "TestTimer.hpp"

using std::chrono::duration;
using std::cout;
using std::endl;
using std::setw;
using std::string;
using std::setprecision;
using std::right;
using std::fixed;
using std::left;


TestTimer::TestTimer()
{
  TestTimer::reset();
}


TestTimer::~TestTimer()
{
}


void TestTimer::reset()
{
  name_ = "";
  count_ = 0;
  user_cum_ = 0;
  user_cum_old_ = 0;
  sys_cum_ = 0;
}


void TestTimer::set_name(const string& s)
{
  name_ = s;
}


void TestTimer::start(const int number)
{
  count_ += number;
  user0_ = Clock::now();
  sys0_ = clock();
}


void TestTimer::end()
{
  time_point<Clock> user1 = Clock::now();
  clock_t sys1 = clock();

  duration<double, std::milli> d = user1 - user0_;
  int tuser = static_cast<int>(d.count());

  user_cum_ += tuser;
  sys_cum_ += static_cast<int>((1000 * (sys1 - sys0_)) /
    static_cast<double>(CLOCKS_PER_SEC));
}


void TestTimer::print_running(
  const int reached,
  const int divisor)
{
  if (count_ == 0)
    return;

  cout << setw(8) << reached << " (" <<
    setw(6) << setprecision(1) << right << fixed <<
      100. * reached / 
        static_cast<float>(divisor) << "%)" <<
    setw(15) << right << fixed << setprecision(0) << 
      (user_cum_ - user_cum_old_) << endl;
  
  user_cum_old_ = user_cum_;
}


void TestTimer::print_basic() const
{
  if (count_ == 0) 
    return;

  if (name_ != "")
    cout << setw(19) << left << "Timer name" << ": " << name_ << "\n";

  cout << setw(19) << left << "Number of calls" << ": " << count_ << "\n";

  if (user_cum_ == 0)
    cout << setw(19) << left << "User time" << ": " << "zero" << "\n";
  else
  {
    cout << setw(19) << left << "User time/ticks" << ": " <<
      user_cum_ << "\n";
    cout << setw(19) << left << "User per call" << ": " <<
      setprecision(2) << user_cum_ / static_cast<float>(count_) << "\n";
  }

  if (sys_cum_ == 0)
    cout << setw(19) << left << "Sys time (ms)" << ": " << "zero" << "\n";
  else
  {
    cout << setw(19) << left << "Sys time/ticks" << ": " <<
      sys_cum_ << "\n";
    cout << setw(19) << left << "Sys per call" << ": " <<
      setprecision(2) << sys_cum_ / static_cast<float>(count_) << "\n";
    if (user_cum_ > 0) {
      cout << setw(19) << left << "Ratio" << ": " <<
        setprecision(2) << sys_cum_ / static_cast<float>(user_cum_);
    }
  }
  cout << endl;
}


void TestTimer::print_hands() const
{
  if (name_ != "")
    cout << setw(21) << left << "Timer name" << 
      setw(12) << right << name_ << "\n";

  cout << setw(21) << left << "Number of hands" << 
    setw(12) << right << count_ << "\n";

  if (count_ == 0)
    return;
  
  if (user_cum_ == 0)
    cout << setw(21) << left << "User time (ms)" <<
      setw(12) << right << "zero" << "\n";
  else
  {
    cout << setw(21) << left << "User time (ms)" <<
      setw(12) << right << fixed << 
        setprecision(0) << user_cum_ << "\n";
    cout << setw(21) << left << "Avg user time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << user_cum_ / 
        static_cast<float>(count_) << "\n";
  }

  if (sys_cum_ == 0)
    cout << setw(21) << left << "Sys time (ms)" << 
      setw(12) << right << "zero" << "\n";
  else
  {
    cout << setw(21) << left << "Sys time (ms)" <<
      setw(12) << right << fixed << setprecision(0) << sys_cum_ << "\n";
    cout << setw(21) << left << "Avg sys time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << sys_cum_ / 
        static_cast<float>(count_) << "\n";
    if (user_cum_ > 0) {
      cout << setw(21) << left << "Ratio" << 
        setw(12) << right << fixed << setprecision(2) << 
        sys_cum_ / static_cast<float>(user_cum_);
    }
  }
  cout << endl;
}

