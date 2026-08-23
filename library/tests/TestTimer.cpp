/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <ratio>
#include <string>

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
  pending_hands_ = 0;
  sys_time_known_ = true;
}


void TestTimer::mark_sys_time_unavailable()
{
  sys_time_known_ = false;
}


bool TestTimer::sys_time_known() const
{
  return sys_time_known_;
}


void TestTimer::set_name(const string& s)
{
  name_ = s;
}


long clock_delta_to_ms(clock_t delta)
{
  return static_cast<long>(
    (1000.0 * static_cast<double>(delta)) /
    static_cast<double>(CLOCKS_PER_SEC));
}


void TestTimer::start(const int number)
{
  pending_hands_ = number;
  user0_ = Clock::now();
  sys0_ = clock();
  if (sys0_ == static_cast<clock_t>(-1))
    sys_time_known_ = false;
}


void TestTimer::end()
{
  time_point<Clock> user1 = Clock::now();
  clock_t sys1 = clock();

  duration<double, std::milli> d = user1 - user0_;
  const long tuser = static_cast<long>(d.count());
  long tsys = 0;
  if (sys_time_known_)
  {
    if (sys1 == static_cast<clock_t>(-1))
      sys_time_known_ = false;
    else
      tsys = clock_delta_to_ms(sys1 - sys0_);
  }

  TestTimer::record(pending_hands_, tuser, tsys);
  pending_hands_ = 0;
}


void TestTimer::record(const int hands, const long user_ms, const long sys_ms)
{
  if (hands <= 0)
    return;

  count_ += hands;
  user_cum_ += user_ms;
  sys_cum_ += sys_ms;
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

  if (!sys_time_known_)
    cout << setw(19) << left << "Sys time (ms)" << ": " << "n/a" << "\n";
  else if (sys_cum_ == 0)
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


void TestTimer::print_hands(ostream& out) const
{
  struct StreamFormatGuard
  {
    explicit StreamFormatGuard(ostream& os)
      : os_(os),
        flags_(os.flags()),
        precision_(os.precision()),
        fill_(os.fill())
    {
    }

    ~StreamFormatGuard()
    {
      os_.flags(flags_);
      os_.precision(precision_);
      os_.fill(fill_);
    }

    ostream& os_;
    const std::ios_base::fmtflags flags_;
    const std::streamsize precision_;
    const char fill_;
  };

  const StreamFormatGuard format_guard(out);

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
  }

  if (!sys_time_known_)
    out << setw(21) << left << "Sys time (ms)" <<
      setw(12) << right << "n/a" << "\n";
  else if (sys_cum_ == 0)
    out << setw(21) << left << "Sys time (ms)" << 
      setw(12) << right << "zero" << "\n";
  else
  {
    out << setw(21) << left << "Sys time (ms)" <<
      setw(12) << right << fixed << setprecision(0) << sys_cum_ << "\n";
    out << setw(21) << left << "Avg sys time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << sys_cum_ / 
        static_cast<float>(count_) << "\n";
    if (user_cum_ > 0) {
      out << setw(21) << left << "Ratio" << 
        setw(12) << right << fixed << setprecision(2) << 
        sys_cum_ / static_cast<float>(user_cum_);
    }
  }
  out << endl;
}
