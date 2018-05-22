/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>

#include "Timer.h"

using std::chrono::duration_cast;
using std::chrono::microseconds;


Timer::Timer()
{
  Timer::Reset();
}


Timer::~Timer()
{
}


void Timer::Reset()
{
  name = "";
  count = 0;
  userCum = 0;
  systCum = 0;
}


void Timer::SetName(const string& nameIn)
{
  name = nameIn;
}


void Timer::Start()
{
  user0 = Clock::now();
  syst0 = clock();
}


void Timer::End()
{
  time_point<Clock> user1 = Clock::now();
  clock_t syst1 = clock();

  chrono::duration<double, micro> d = user1 - user0;
  int tuser = static_cast<int>(d.count());

  count++;
  userCum += tuser;
  systCum += static_cast<long>(syst1) - 
    static_cast<long>(syst0);
}


bool Timer::Used() const
{
  return (count > 0);
}


int Timer::UserTime() const
{
  return static_cast<int>(userCum);
}


void Timer::operator +=(const Timer& add)
{
  count += add.count;
  userCum += add.userCum;
  systCum += add.systCum;
}


void Timer::operator -=(const Timer& deduct)
{
  if (deduct.userCum > userCum)
    userCum = 0;
  else
    userCum -= deduct.userCum;

  if (deduct.systCum > systCum)
    systCum = 0;
  else
    systCum -= deduct.systCum;
}


string Timer::SumLine(
  const Timer& divisor,
  const string& bname) const
{
  stringstream ss;
  if (count > 0)
  {
    ss << setw(14) << left << (bname == "" ? name : bname) <<
      setw(9) << right << count <<
      setw(11) << userCum <<
      setw(7) << setprecision(2) << fixed << 
        userCum / static_cast<double>(count) <<
      setw(5) << setprecision(1) << fixed << 
        100. * userCum / divisor.userCum <<
      setw(11) << setprecision(0) << fixed << 
        1000000 * systCum / static_cast<double>(CLOCKS_PER_SEC) <<
      setw(7) << setprecision(2) << fixed <<
        1000000 * systCum / static_cast<double>(count * CLOCKS_PER_SEC) <<
      setw(5) << setprecision(1) << fixed << 
        100. * systCum / divisor.systCum << "\n";
  }
  else
  {
    ss << setw(14) << left << (bname == "" ? name : bname) <<
      setw(9) << right << count <<
      setw(11) << userCum <<
      setw(7) << "-" <<
      setw(5) << "-" <<
      setw(11) << 1000000 * systCum / static_cast<double>(CLOCKS_PER_SEC) <<
      setw(7) << "-" <<
      setw(5) << "-" << "\n";
  }
  return ss.str();
}


string Timer::DetailLine() const
{
  stringstream ss;
  ss << setw(15) << left << name <<
    setw(10) << right << count <<
    setw(11) << right << userCum <<
    setw(11) << setprecision(2) << fixed << 
      userCum / static_cast<double>(count) <<
    setw(11) << setprecision(0) << fixed <<
      1000000 * systCum / static_cast<double>(CLOCKS_PER_SEC) <<
    setw(11) << setprecision(2) << fixed <<
      1000000 * systCum / 
        static_cast<double>(count * CLOCKS_PER_SEC) << "\n";

  return ss.str();
}
