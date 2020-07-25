/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#define TIMER_DEPTH 50

#include <iostream>
#include <iomanip>
#include <sstream>

#include "TimerGroup.h"


TimerGroup::TimerGroup()
{
  TimerGroup::Reset();
}


TimerGroup::~TimerGroup()
{
}


void TimerGroup::Reset()
{
  timers.resize(TIMER_DEPTH);
  for (unsigned i = 0; i < timers.size(); i++)
    timers[i].Reset();
}


void TimerGroup::SetNames(const string& baseName)
{
  string st;
  if (baseName == "AB")
  {
    // Special format emphasizing the card number within the trick.
    for (unsigned i = 0; i < timers.size(); i++)
    {
      st = baseName + to_string(i % 4) + " " + to_string(i);
      timers[i].SetName(st);
    }
  }
  else
  {
    for (unsigned i = 0; i < timers.size(); i++)
    {
      st = baseName + to_string(i);
      timers[i].SetName(st);
    }
  }
  bname = baseName;
}


void TimerGroup::Start(const unsigned no)
{
  timers[no].Start();
}


void TimerGroup::End(const unsigned no)
{
  timers[no].End();
}


bool TimerGroup::Used() const
{
  for (unsigned i = 0; i < timers.size(); i++)
  {
    if (timers[i].Used())
      return true;
  }
  return false;
}


void TimerGroup::Differentiate()
{
  for (unsigned r = 0; r < timers.size()-1; r++)
  {
    size_t i = timers.size() - 1 - r;
    timers[i] -= timers[i-1];
  }
}


void TimerGroup::Sum(Timer& sum) const
{
  sum = timers[0];
  for (unsigned i = 1; i < timers.size(); i++)
    sum += timers[i];
}


void TimerGroup::operator -= (const TimerGroup& deduct)
{
  for (unsigned i = 0; i < timers.size(); i++)
    timers[i] -= deduct.timers[i];
}


string TimerGroup::Header() const
{
  stringstream ss;
  ss << setw(14) << left << "Name" <<
    setw(9) << right << "Count" <<
    setw(11) << "User" <<
    setw(7) << "Avg" <<
    setw(5) << "%" <<
    setw(11) << "Syst" <<
    setw(7) << "Avg" <<
    setw(5) << "%" << "\n";
  return ss.str();
}


string TimerGroup::DetailHeader() const
{
  stringstream ss;
  ss << setw(14) << left << "Name " <<
    setw(11) << right << "Number" <<
    setw(11) << "User ticks" <<
    setw(11) << "Avg" <<
    setw(11) << "System" <<
    setw(11) << "Avg ms" << "\n";
  return ss.str();
}


string TimerGroup::SumLine(const Timer& sumTotal) const
{
  Timer ownSum;
  TimerGroup::Sum(ownSum);

  return ownSum.SumLine(sumTotal, bname);
}


string TimerGroup::TimerLines(const Timer& sumTotal) const
{
  string st = "";
  for (unsigned r = 0; r < timers.size(); r++)
  {
    size_t i = timers.size() - r - 1;
    if (timers[i].Used())
      st += timers[i].SumLine(sumTotal);
  }
  return st;
}


string TimerGroup::DetailLines() const
{
  stringstream ss;
  for (unsigned i = 0; i < timers.size(); i++)
  {
    if (timers[i].Used())
      ss << timers[i].DetailLine();
  }

  return ss.str();
}


string TimerGroup::DashLine() const
{
  return string(69, '-') + "\n";
}

