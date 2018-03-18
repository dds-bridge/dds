/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/portab.h"
#include "testStats.h"

#define NUM_TIMERS 2000
#define COUNTER_SLOTS 200


#ifdef _WIN32
  LARGE_INTEGER ttimerFreq;
  LARGE_INTEGER ttimerUser0;
  LARGE_INTEGER ttimerUser1;
  LARGE_INTEGER ttimerListUser0[NUM_TIMERS];
  LARGE_INTEGER ttimerListUser1[NUM_TIMERS];
#else
  #include <sys/time.h>

  int TesttimevalDiff(timeval x, timeval y);

  timeval ttimerUser0;
  timeval ttimerUser1;
  timeval ttimerListUser0[NUM_TIMERS];
  timeval ttimerListUser1[NUM_TIMERS];
#endif

clock_t ttimerSys0;
clock_t ttimerSys1;
clock_t ttimerListSys0[NUM_TIMERS];
clock_t ttimerListSys1[NUM_TIMERS];

int ttimerCount;
int ttimerListCount[NUM_TIMERS];

int ttimerNameSet;

char ttimerName[80];

long long ttimerUserCum;
long long ttimerSysCum;
long long ttimerListUserCum[NUM_TIMERS];
long long ttimerListSysCum[NUM_TIMERS];
long long tpredError;
long long tpredAbsError;


void TestInitTimer()
{
  ttimerCount = 0;
  ttimerUserCum = 0;
  ttimerSysCum = 0;
  ttimerNameSet = 0;

  tpredError = 0;
  tpredAbsError = 0;
}


void TestSetTimerName(const char * name)
{
  strcpy(ttimerName, name);
  ttimerNameSet = 1;
}


void TestStartTimer()
{
  ttimerCount++;
  ttimerSys0 = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&ttimerUser0);
#else
  gettimeofday(&ttimerUser0, NULL);
#endif
}


void TestEndTimer()
{
  ttimerSys1 = clock();

#ifdef _WIN32
  // To get "real" seconds we would have to divide by
  // timerFreq.QuadPart which needs to be initialized.
  QueryPerformanceCounter(&ttimerUser1);
  int ttimeUser = static_cast<int>
                  ((ttimerUser1.QuadPart - ttimerUser0.QuadPart));
#else
  gettimeofday(&ttimerUser1, NULL);
  int ttimeUser = TesttimevalDiff(ttimerUser1, ttimerUser0);
#endif

  ttimerUserCum += ttimeUser;

  ttimerSysCum += static_cast<int>((1000 * (ttimerSys1 - ttimerSys0)) /
                                   static_cast<double>(CLOCKS_PER_SEC));
}


void TestPrintTimer()
{
  if (ttimerCount == 0) return;

  if (ttimerNameSet)
    printf("%-18s : %s\n", "Timer name", ttimerName);

  printf("%-18s : %10d\n", "Number of calls", ttimerCount);

  if (ttimerUserCum == 0)
    printf("%-18s : %s\n", "User time", "zero");
  else
  {
    printf("%-18s : %10lld\n", "User time/ticks", ttimerUserCum);
    printf("%-18s : %10.2f\n", "User per call",
           static_cast<float>(ttimerUserCum / ttimerCount));
  }

  if (ttimerSysCum == 0)
    printf("%-18s : %s\n", "Sys time", "zero");
  else
  {
    printf("%-18s : %10lld\n", "Sys time/ticks", ttimerSysCum);
    printf("%-18s : %10.2f\n", "Sys per call",
           static_cast<float>(ttimerSysCum / ttimerCount));
    printf("%-18s : %10.2f\n", "Ratio",
           static_cast<float>(ttimerSysCum / ttimerUserCum));
  }
  printf("\n");
}


void TestInitTimerList()
{
  for (int i = 0; i < NUM_TIMERS; i++)
  {
    ttimerListCount [i] = 0;
    ttimerListUserCum[i] = 0;
    ttimerListSysCum [i] = 0;
  }
}


void TestStartTimerNo(int no)
{
  ttimerListCount[no]++;
  ttimerListSys0[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&ttimerListUser0[no]);
#else
  gettimeofday(&ttimerListUser0[no], NULL);
#endif
}


void TestEndTimerNo(int no)
{
  ttimerListSys1[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&ttimerListUser1[no]);
  int timeUser = static_cast<int>
                 ((ttimerListUser1[no].QuadPart - ttimerListUser0[no].QuadPart));
#else
  gettimeofday(&ttimerListUser1[no], NULL);
  int timeUser = TesttimevalDiff(ttimerListUser1[no],
                                 ttimerListUser0[no]);
#endif

  ttimerListUserCum[no] += static_cast<long long>(timeUser);

  ttimerListSysCum[no] +=
    static_cast<long long>((1000 *
                            (ttimerListSys1[no] - ttimerListSys0[no])) /
                           static_cast<double>(CLOCKS_PER_SEC));
}


void TestEndTimerNoAndComp(int no, int pred)
{
  ttimerListSys1[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&ttimerListUser1[no]);
  int timeUser = static_cast<int>
                 ((ttimerListUser1[no].QuadPart - ttimerListUser0[no].QuadPart));
#else
  gettimeofday(&ttimerListUser1[no], NULL);
  int timeUser = TesttimevalDiff(ttimerListUser1[no],
                                 ttimerListUser0[no]);
#endif

  ttimerListUserCum[no] += static_cast<long long>(timeUser);

  tpredError += timeUser - pred;

  tpredAbsError += (timeUser >= pred ?
                    timeUser - pred : pred - timeUser);

  ttimerListSysCum[no] +=
    static_cast<long long>(
      (1000 * (ttimerListSys1[no] - ttimerListSys0[no])) /
      static_cast<double>(CLOCKS_PER_SEC));
}


void TestPrintTimerList()
{
  int totNum = 0;
  for (int no = 0; no < NUM_TIMERS; no++)
  {
    if (ttimerListCount[no] == 0)
      continue;

    totNum += ttimerListCount[no];
  }

  if (totNum == 0)
    return;

  printf("%5s %10s %12s %10s %10s\n",
         "n", "Number", "User ticks", "Avg", "Syst time");

  for (int no = 0; no < NUM_TIMERS; no++)
  {
    double avg = static_cast<double>(ttimerListUserCum[no]) /
                 static_cast<double>(ttimerListCount[no]);

    // For some reason I have trouble when putting it on one line...
    printf("%5d %10d %12lld ",
           no,
           ttimerListCount[no],
           ttimerListUserCum[no]);
    printf(" %10.2f %10lld\n",
           avg,
           ttimerListSysCum[no]);
  }
  printf("\n");
  if (tpredError != 0)
  {
    printf("Total number %10d\n", totNum);
    printf("Prediction mean %10.0f\n",
           static_cast<double>(tpredError) /
           static_cast<double>(totNum));
    printf("Prediction abs mean %10.0f\n",
           static_cast<double>(tpredAbsError) /
           static_cast<double>(totNum));
    printf("\n");
  }
}


#ifndef _WIN32
int TesttimevalDiff(timeval x, timeval y)
{
  /* Elapsed time, x-y, in milliseconds */
  return 1000 * (x.tv_sec - y.tv_sec )
         + (x.tv_usec - y.tv_usec) / 1000;
}
#endif


long long tcounter[COUNTER_SLOTS];

void TestInitCounter()
{
  for (int i = 0; i < COUNTER_SLOTS; i++)
    tcounter[i] = 0;
}


void TestPrintCounter()
{
  for (int i = 0; i < COUNTER_SLOTS; i++)
  {
    if (tcounter[i])
      printf("%d\t%12lld\n", i, tcounter[i]);
  }
  printf("\n");
}


