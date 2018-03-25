/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>

#include "TimeStatList.h"


TimeStatList::TimeStatList()
{
  TimeStatList::Reset();
}


TimeStatList::~TimeStatList()
{
}


void TimeStatList::Reset()
{
}


void TimeStatList::Init(
  const string& tname,
  const unsigned len)
{
  name = tname;
  list.resize(len);
}


void TimeStatList::Add(
  const unsigned pos,
  const TimeStat& add)
{
  list[pos] += add;
}


bool TimeStatList::Used() const
{
  for (unsigned i = 0; i < list.size(); i++)
  {
    if (list[i].Used())
      return true;
  }
  return false;
}


string TimeStatList::List() const
{
  if (! TimeStatList::Used())
    return "";

  stringstream ss;
  ss << name << "\n\n";
  ss << list[0].Header();

  TimeStat tsum;
  for (unsigned i = 0; i < list.size(); i++)
  {
    if (! list[i].Used())
      continue;

    tsum += list[i];
    ss << setw(5) << right << i << list[i].Line();
  }

  ss << setw(5) << right << "Avg" << tsum.Line() << "\n";

  return ss.str();
}

