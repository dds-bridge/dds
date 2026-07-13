/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <algorithm>
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
using std::ostream;


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
  user_min_ = 0;
  user_max_ = 0;
  sys_min_ = 0;
  sys_max_ = 0;
  batch_count_ = 0;
  pending_hands_ = 0;
}


void TestTimer::set_name(const string& s)
{
  name_ = s;
}


void TestTimer::start(const int number)
{
  pending_hands_ = number;
  user0_ = Clock::now();
  sys0_ = clock();
}


void TestTimer::end()
{
  time_point<Clock> user1 = Clock::now();
  clock_t sys1 = clock();

  duration<double, std::milli> d = user1 - user0_;
  const long tuser = static_cast<long>(d.count());
  const long tsys = static_cast<long>((1000 * (sys1 - sys0_)) /
    static_cast<double>(CLOCKS_PER_SEC));

  TestTimer::record(pending_hands_, tuser, tsys);
  pending_hands_ = 0;
}


void TestTimer::record(const int hands, const long user_ms, const long sys_ms)
{
  count_ += hands;
  user_cum_ += user_ms;
  sys_cum_ += sys_ms;

  if (hands > 0)
  {
    const double user_per_hand = user_ms / static_cast<double>(hands);
    const double sys_per_hand = sys_ms / static_cast<double>(hands);
    if (batch_count_ == 0)
    {
      user_min_ = user_max_ = user_per_hand;
      sys_min_ = sys_max_ = sys_per_hand;
    }
    else
    {
      user_min_ = std::min(user_min_, user_per_hand);
      user_max_ = std::max(user_max_, user_per_hand);
      sys_min_ = std::min(sys_min_, sys_per_hand);
      sys_max_ = std::max(sys_max_, sys_per_hand);
    }
    batch_count_++;
  }
}


bool TestTimer::has_batch_times() const
{
  return batch_count_ > 0;
}


double TestTimer::user_min_ms() const
{
  return user_min_;
}


double TestTimer::user_max_ms() const
{
  return user_max_;
}


double TestTimer::sys_min_ms() const
{
  return sys_min_;
}


double TestTimer::sys_max_ms() const
{
  return sys_max_;
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


void TestTimer::print_hands(
  ostream& out,
  const bool show_min,
  const bool show_max) const
{
  if (name_ != "")
    out << setw(21) << left << "Timer name" << 
      setw(12) << right << name_ << "\n";

  out << setw(21) << left << "Number of hands" << 
    setw(12) << right << count_ << "\n";

  if (count_ == 0)
    return;
  
  if (user_cum_ == 0)
    out << setw(21) << left << "User time (ms)" <<
      setw(12) << right << "zero" << "\n";
  else
  {
    out << setw(21) << left << "User time (ms)" <<
      setw(12) << right << fixed << 
        setprecision(0) << user_cum_ << "\n";
    out << setw(21) << left << "Avg user time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << user_cum_ / 
        static_cast<float>(count_) << "\n";
    if (show_min && has_batch_times())
      out << setw(21) << left << "Min user time (ms)" <<
        setw(12) << right << fixed << setprecision(2) << user_min_ << "\n";
    if (show_max && has_batch_times())
      out << setw(21) << left << "Max user time (ms)" <<
        setw(12) << right << fixed << setprecision(2) << user_max_ << "\n";
  }

  if (sys_cum_ == 0)
    out << setw(21) << left << "Sys time (ms)" << 
      setw(12) << right << "zero" << "\n";
  else
  {
    out << setw(21) << left << "Sys time (ms)" <<
      setw(12) << right << fixed << setprecision(0) << sys_cum_ << "\n";
    out << setw(21) << left << "Avg sys time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << sys_cum_ / 
        static_cast<float>(count_) << "\n";
    if (show_min && has_batch_times())
      out << setw(21) << left << "Min sys time (ms)" <<
        setw(12) << right << fixed << setprecision(2) << sys_min_ << "\n";
    if (show_max && has_batch_times())
      out << setw(21) << left << "Max sys time (ms)" <<
        setw(12) << right << fixed << setprecision(2) << sys_max_ << "\n";
    if (user_cum_ > 0) {
      out << setw(21) << left << "Ratio" << 
        setw(12) << right << fixed << setprecision(2) << 
        sys_cum_ / static_cast<float>(user_cum_);
    }
  }
  out << endl;
}
