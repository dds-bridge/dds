/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*
   TimerList consists of a number of groups, one for each piece
   of the code being timed (ABsearch etc).

   Each group corresponds to something that should be timed at
   multiple AB depths, i.e. cards played. The first card of a
   new game is number 48, and the last card is number 0.

   The AB timer is special, as the AB functions are recursive
   and so their timing includes not only the other functions they
   contain, but also their own recursive calls at lower depths.
   The AB timer group must be the first one.

   The object calculates an approximation to exclusive function
   times, so it is a "poor man's profiler".

   For AB, first the times at depth-1 are subtracted out, and then
   the times for all calls at the same depth are subtracted out.
   This still leaves the overhead of the timing itself. As an
   approximation, there is one timing overhead left for each
   function, and it is on the order of the execution time of
   Evaluate(), which is a very fast function.

   TIMER_START and TIMER_END are macros for bracketing code
   to be timed, so

   TIMER_START(TIMER_NO_AB, depth);
   ABsearch(...);
   TIMER_END(TIMER_NO_AB, depth);

   This avoids the tedious #ifdef's at every place of a timer.
 */

#ifndef DDS_TIMERLIST_H
#define DDS_TIMERLIST_H

#include <iostream>
#include <fstream>
#include <vector>

#include "TimerGroup.h"
#include "debug.h"

using namespace std;


#ifdef DDS_TIMING
  #define TIMER_START(g, a) thrp->timerList.Start(g, a)
  #define TIMER_END(g, a) thrp->timerList.End(g, a)
#else
  #define TIMER_START(g, a) 1
  #define TIMER_END(g, a) 1
#endif

#define TIMER_NO_AB 0
#define TIMER_NO_MAKE 1
#define TIMER_NO_UNDO 2
#define TIMER_NO_EVALUATE 3
#define TIMER_NO_NEXTMOVE 4
#define TIMER_NO_QT 5
#define TIMER_NO_LT 6
#define TIMER_NO_MOVEGEN 7
#define TIMER_NO_LOOKUP 8
#define TIMER_NO_BUILD 9

#define TIMER_GROUPS 10


class TimerList
{
  private:

    string fname;

    vector<TimerGroup> timerGroups;

  public:
    TimerList();

    ~TimerList();

    void Reset();

    void SetFile(const string& fnameIn);

    void Start(
      const unsigned groupno,
      const unsigned timerno);

    void End(
      const unsigned groupno,
      const unsigned timerno);

    bool Used() const;

    void PrintStats() const;
};

#endif
