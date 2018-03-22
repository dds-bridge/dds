/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>

#include "TestTimer.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;


TestTimer::TestTimer()
{
  TestTimer::reset();
}


TestTimer::~TestTimer()
{
}


void TestTimer::reset()
{
  name = "";
  count = 0;
  userCum = 0;
  userCumOld = 0;
  sysCum = 0;
}


void TestTimer::setname(const string& s)
{
  name = s;
}


void TestTimer::start(const int number)
{
  count += number;
  user0 = Clock::now();
  sys0 = clock();
}


void TestTimer::end()
{
  time_point<Clock> user1 = Clock::now();
  clock_t sys1 = clock();

  chrono::duration<double, milli> d = user1 - user0;
  int tuser = static_cast<int>(1000. * d.count());

  userCum += tuser;
  sysCum += static_cast<int>((1000 * (sys1 - sys0)) /
    static_cast<double>(CLOCKS_PER_SEC));
}


void TestTimer::printRunning(
  const int reached,
  const int divisor)
{
  if (count == 0)
    return;

  cout << setw(8) << reached << " (" <<
    setw(6) << setprecision(1) << right << fixed <<
      100. * reached / 
        static_cast<float>(divisor) << "%)" <<
    setw(15) << right << fixed << setprecision(0) << 
      (userCum - userCumOld) / 1000. << endl;
  
  userCumOld = userCum;
}


void TestTimer::printBasic() const
{
  if (count == 0) 
    return;

  if (name != "")
    cout << setw(19) << left << "Timer name" << ": " << name << "\n";

  cout << setw(19) << left << "Number of calls" << ": " << count << "\n";

  if (userCum == 0)
    cout << setw(19) << left << "User time" << ": " << "zero" << "\n";
  else
  {
    cout << setw(19) << left << "User time/ticks" << ": " <<
      userCum << "\n";
    cout << setw(19) << left << "User per call" << ": " <<
      setprecision(2) << userCum / static_cast<float>(count) << "\n";
  }

  if (sysCum == 0)
    cout << setw(19) << left << "Sys time" << ": " << "zero" << "\n";
  else
  {
    cout << setw(19) << left << "Sys time/ticks" << ": " <<
      sysCum << "\n";
    cout << setw(19) << left << "Sys per call" << ": " <<
      setprecision(2) << sysCum / static_cast<float>(count) << "\n";
    cout << setw(19) << left << "Ratio" << ": " <<
      setprecision(2) << sysCum / static_cast<float>(userCum);
  }
  cout << endl;
}


void TestTimer::printHands() const
{
  if (name != "")
    cout << setw(21) << left << "Timer name" << 
      setw(12) << right << name << "\n";

  cout << setw(21) << left << "Number of hands" << 
    setw(12) << right << count << "\n";

  if (count == 0)
    return;
  
  if (userCum == 0)
    cout << setw(21) << left << "User time (ms)" <<
      setw(12) << right << "zero" << "\n";
  else
  {
    cout << setw(21) << left << "User time (ms)" <<
      setw(12) << right << fixed << 
        setprecision(0) << userCum / 1000. << "\n";
    cout << setw(21) << left << "Avg user time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << userCum / 
        static_cast<float>(1000. * count) << "\n";
  }

  if (sysCum == 0)
    cout << setw(21) << left << "Sys time" << 
      setw(12) << right << "zero" << "\n";
  else
  {
    cout << setw(21) << left << "Sys time (ms)" <<
      setw(12) << right << fixed << setprecision(0) << sysCum << "\n";
    cout << setw(21) << left << "Avg sys time (ms)" <<
      setw(12) << right << fixed << setprecision(2) << sysCum / 
        static_cast<float>(count) << "\n";
    cout << setw(21) << left << "Ratio" << 
      setw(12) << right << fixed << setprecision(2) << 
      1000. * sysCum / static_cast<float>(userCum);
  }
  cout << endl;
}

