/* 
   DDS 2.7.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund   
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
   permission to include a C++ adaptation in DDS.
   						
   The PlayAnalyser analyses the played cards of the deal and presents 
   their double dummy values. The par calculation function DealerPar 
   provides an alternative way of calculating and presenting par 
   results.  Both these functions have been written by Soren Hein.
   He has also made numerous contributions to the code, especially in 
   the initialization part.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
   implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "dds.h"
#include "Stats.h"

#define NUM_TIMERS	2000
#define COUNTER_SLOTS	 200

#include <time.h>

#ifdef _WIN32
LARGE_INTEGER 	timerFreq, 
		timerUser0, 
		timerUser1, 
              	timerListUser0[NUM_TIMERS], 
	      	timerListUser1[NUM_TIMERS];
#else
#include <sys/time.h>
int 		timevalDiff(timeval x, timeval y);

timeval 	timerUser0, 
		timerUser1,
              	timerListUser0[NUM_TIMERS], 
	      	timerListUser1[NUM_TIMERS];
#endif

clock_t 	timerSys0, 
		timerSys1,
		timerListSys0[NUM_TIMERS], 
		timerListSys1[NUM_TIMERS];

int		timerCount,
		timerListCount[NUM_TIMERS];

int		timerNameSet;

char		timerName[80];

int     	timerUserCum, 
		timerSysCum,
        	timerListUserCum[NUM_TIMERS], 
		timerListSysCum[NUM_TIMERS];


void InitTimer()
{
  timerCount   = 0;
  timerUserCum = 0;
  timerSysCum  = 0;
  timerNameSet = 0;
}


void SetTimerName(char * name)
{
  strcpy(timerName, name);
  timerNameSet = 1;
}


void StartTimer()
{
  timerCount++;
  timerSys0 = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&timerUser0);
#else
  gettimeofday(&timerUser0, NULL);
#endif
}


void EndTimer()
{
  timerSys1 = clock();

#ifdef _WIN32
  // To get "real" seconds we would have to divide by
  // timerFreq.QuadPart which needs to be initialized.
  QueryPerformanceCounter(&timerUser1);
  int timeUser = (timerUser1.QuadPart - timerUser0.QuadPart);
#else
  gettimeofday(&timerUser1, NULL);
  int timeUser = timevalDiff(timerUser1, timerUser0);
#endif
  
  timerUserCum += timeUser;

  timerSysCum += (int) (1000 * (timerSys1-timerSys0) / 
                 (double) CLOCKS_PER_SEC);
}


void PrintTimer()
{
  if (timerNameSet)
    printf("%-18s : %s\n", "Timer name", timerName);

  printf("%-18s : %10d\n", "Number of calls", timerCount);
  if (timerCount == 0) return;

  if (timerUserCum == 0)
    printf("%-18s : %s\n", "User time", "zero");
  else
  {
    printf("%-18s : %10d\n", "User time/ticks", timerUserCum);
    printf("%-18s : %10.2f\n", "User per call",  
      (float) timerUserCum / timerCount);
  }

  if (timerSysCum == 0)
    printf("%-18s : %s\n", "Sys time", "zero");
  else
  {
    printf("%-18s : %10d\n", "Sys time/ticks", timerSysCum);
    printf("%-18s : %10.2f\n", "Sys per call",  
      (float) timerSysCum / timerCount);
    printf("%-18s : %10.2f\n", "Ratio", 
      (float) timerSysCum / timerUserCum);
  }
  printf("\n");
}


void InitTimerList()
{
  for (int i = 0; i < NUM_TIMERS; i++)
  {
    timerListCount  [i] = 0;
    timerListUserCum[i] = 0;
    timerListSysCum [i] = 0;
  }
}


void StartTimerNo(int no)
{
  timerListCount[no]++;
  timerListSys0[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&timerListUser0[no]);
#else
  gettimeofday(&timerListUser0[no], NULL);
#endif
}


void EndTimerNo(int no)
{
  timerListSys1[no] = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&timerListUser1[no]);
  int timeUser = (timerListUser1[no].QuadPart - 
                  timerListUser0[no].QuadPart);
#else
  gettimeofday(&timerListUser1[no], NULL);
  int timeUser = timevalDiff(timerListUser1[no], 
                             timerListUser0[no]);
#endif
  
  timerListUserCum[no] += timeUser;

  timerListSysCum[no] += 
    (int) (1000 * (timerListSys1[no] - timerListSys0[no]) / 
    (double) CLOCKS_PER_SEC);
}


void PrintTimerList()
{
  printf("%5s  %10s  %10s  %8s  %10s\n",
    "n", "Number", "User ticks", "Avg", "Syst time");

  for (int no = 0; no < NUM_TIMERS; no++)
  {
    if (timerListCount[no] == 0)
      continue;

    printf("%5d  %10d  %10d  %8.2f  %10d\n",
      no, 
      timerListCount[no],
      timerListUserCum[no],
      timerListUserCum[no] / (double) timerListCount[no],
      timerListSysCum[no]);
  }
  printf("\n");
}


#ifndef _WIN32
int timevalDiff(timeval x, timeval y)
{
  /* Elapsed time, x-y, in milliseconds */
  return 1000 * (x.tv_sec  - y.tv_sec )
       +        (x.tv_usec - y.tv_usec) / 1000;
}
#endif


long long counter[COUNTER_SLOTS];

void InitCounter()
{
  for (int i = 0; i < COUNTER_SLOTS; i++)
    counter[i] = 0;
}


void PrintCounter()
{
  for (int i = 0; i < COUNTER_SLOTS; i++)
  {
    if (counter[i])
      printf("%d\t%12ld\n", i, counter[i]);
  }
  printf("\n");
}


