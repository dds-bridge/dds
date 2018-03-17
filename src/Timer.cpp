/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


/*
   See Timer.h for some description.
*/

#include <iostream>
#include <iomanip>
#include <sstream>

#include "Timer.h"

using namespace std;


Timer::Timer()
{
  Timer::Reset();
}


Timer::~Timer()
{
}


void Timer::Reset()
{
  count = 0;
  userCum = 0;
  systCum = 0.;
}


void Timer::SetName(const string& s)
{
  name = s;
}


void Timer::Start()
{
  systTimes0 = clock();

#ifdef _MSC_VER
  QueryPerformanceCounter(&userTimes0);
#else
  gettimeofday(&userTimes0, nullptr);
#endif
}


void Timer::End()
{
  systTimes1 = clock();

#ifdef _MSC_VER
  QueryPerformanceCounter(&userTimes1);
  int timeUser = static_cast<int>
    (userTimes1.QuadPart - userTimes0.QuadPart);
#else
  gettimeofday(&userTimes1, nullptr);
  return 1000 * (userTimes1.tv_sec - userTimer0.tv_sec )
         + (userTimes1.tv_usec - userTimer0.tv_usec) / 1000;
#endif

  count++;

  // This is more or less in milli-seconds except on Windows,
  // where it is in "wall ticks". It is possible to convert
  // to milli-seconds, but the resolution is so poor for fast
  // functions that I leave it in integer form.

  userCum += timeUser;
  systCum += systTimes1 - systTimes0;
}


bool Timer::Used() const
{
  return (count > 0);
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
      setw(11) << setprecision(0) << fixed << 1000. * systCum <<
      setw(7) << setprecision(2) << fixed <<
        1000. * systCum / static_cast<double>(count) <<
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
      setw(11) << 1000 * systCum <<
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
      1000. * systCum <<
    setw(11) << setprecision(2) << fixed <<
      1000. * systCum / static_cast<double>(count) << "\n";

  return ss.str();
}
