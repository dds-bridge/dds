/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <sstream>
#include <math.h>

#include "TimeStat.h"


TimeStat::TimeStat()
{
  TimeStat::Reset();
}


TimeStat::~TimeStat()
{
}


void TimeStat::Reset()
{
  number = 0;
  cum = 0;
  cumsq = 0.;
}


void TimeStat::Set(const int timeUser)
{
  number = 1;
  cum = timeUser;
  cumsq = static_cast<double>(timeUser) * static_cast<double>(timeUser);
}


void TimeStat::Set(
  const int timeUser,
  const double timesq)
{
  number = 1;
  cum = timeUser;
  cumsq = timesq;
}


void TimeStat::operator +=(const TimeStat& add)
{
  number += add.number;
  cum += add.cum;
  cumsq += add.cumsq;
}


bool TimeStat::Used() const
{
  return (number > 0);
}

string TimeStat::Header() const
{
  stringstream ss;
  ss << setw(5) << right << "n" <<
    setw(9) << right << "Number" <<
    setw(13) << "Cum time" <<
    setw(13) << "Average" <<
    setw(13) << "Sdev" <<
    setw(13) << "Sdev/mu" << "\n";

  return ss.str();
}


string TimeStat::Line() const
{
  if (number == 0)
    return "";

  double avg = static_cast<double>(cum) / static_cast<double>(number);
  double arg = (cumsq / static_cast<double>(number)) - avg * avg;
  double sdev = (arg >= 0. ? sqrt(arg) : 0.);

  stringstream ss;
  ss << setw(9) << right << number <<
    setw(13) << cum <<
    setw(13) << setprecision(0) << fixed << avg <<
    setw(13) << setprecision(0) << fixed << sdev <<
    setw(13) << setprecision(2) << fixed << sdev/avg << "\n";

  return ss.str();
}
