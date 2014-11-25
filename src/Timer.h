/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/* 
   It is a simple object for timing functions or code pieces.
   As it stands, it is somewhat specific to AB searches,
   but it can of course be generalized.

   There are groups of 50 timers.  
   
   Each group corresponds to something that should be timed at 
   multiple AB depths, i.e. cards played.  The first card of a 
   new game is number 48, and the last card is number 0.

   The AB timer is special, as the AB functions are recursive
   and so their timing includes not only the other functions they
   contain, but also their own recursive calls at lower depths.
   The AB timer group must be the first one.

   The object calculates an approximation to exclusive function
   times, so it is a "poor man's profiler".  
   
   For AB, first the times at depth-1 are subtracted out, and then 
   the times for all calls at the same depth are subtracted out.  
   This still leaves the overhead of the timing itself.  As an 
   approximation, there is one timing overhead left for each 
   function, and it is on the order of the execution time of 
   Evaluate(), which is a very fast function.
 */


#ifndef _DDS_TIMING
#define _DDS_TIMING

#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <sys/time.h>
#endif

/*
   TIMER_START and TIMER_END are macros for bracketing code
   to be timed, so

   TIMER_START(TIMER_AB + depth);
   ABsearch(...);
   TIMER_END;

   This avoid the tedious #ifdef's at every place of a timer.
*/

#ifdef DDS_TIMING
#define TIMER_START(a) thrp->timer.Start(a)
#define TIMER_END(a)   thrp->timer.End(a)
#else
#define TIMER_START(a)    1
#define TIMER_END(a)      1
#endif

#define LINE_LEN         20
#define TIMER_SPACING    50
#define TIMER_GROUPS     10

#define TIMER_NO_AB       0
#define TIMER_NO_MAKE     1
#define TIMER_NO_UNDO     2
#define TIMER_NO_EVALUATE 3
#define TIMER_NO_NEXTMOVE 4
#define TIMER_NO_QT       5
#define TIMER_NO_LT       6
#define TIMER_NO_MOVEGEN  7
#define TIMER_NO_LOOKUP   8
#define TIMER_NO_BUILD    9

#define TIMER_AB          TIMER_NO_AB
#define TIMER_MAKE      ( TIMER_NO_MAKE     * TIMER_SPACING)
#define TIMER_UNDO      ( TIMER_NO_UNDO     * TIMER_SPACING)
#define TIMER_EVALUATE  ( TIMER_NO_EVALUATE * TIMER_SPACING)
#define TIMER_NEXTMOVE  ( TIMER_NO_NEXTMOVE * TIMER_SPACING)
#define TIMER_QT        ( TIMER_NO_QT       * TIMER_SPACING)
#define TIMER_LT        ( TIMER_NO_LT       * TIMER_SPACING)
#define TIMER_MOVEGEN   ( TIMER_NO_MOVEGEN  * TIMER_SPACING)
#define TIMER_LOOKUP    ( TIMER_NO_LOOKUP   * TIMER_SPACING)
#define TIMER_BUILD     ( TIMER_NO_BUILD    * TIMER_SPACING)

#define DDS_TIMERS      (TIMER_GROUPS * TIMER_SPACING)


class Timer
{
  private:
    FILE                * fp;
    char                fname[DDS_FNAME_LEN];
    char                name[DDS_TIMERS][LINE_LEN];

    clock_t             systTimes0[DDS_TIMERS],
                        systTimes1[DDS_TIMERS];

#ifdef _WIN32
    LARGE_INTEGER       userTimes0[DDS_TIMERS],
                        userTimes1[DDS_TIMERS];
#else
    timeval             userTimes0[DDS_TIMERS],
                        userTimes1[DDS_TIMERS];
#endif

    int                 count[DDS_TIMERS];
    __int64             userCum[DDS_TIMERS];
    double              systCum[DDS_TIMERS];

    void                OutputStats(char * t);
    int                 TimevalDiff(timeval x, timeval y);
    void                OutputDetails();

  public:
    Timer();
    ~Timer();
    void                Reset();
    void                SetFile(char * fname);
    void                SetName(int no, char * name);
    void                SetNames();
    void                Start(int no);
    void                End(int no);
    void                PrintStats();
};

#endif
