/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef _DDS_SCHEDULER
#define _DDS_SCHEDULER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dds.h"
#include "../include/dll.h"

#ifndef _WIN32
#include <sys/time.h>
#endif

#define SCHEDULER_NOSORT	0
#define SCHEDULER_SOLVE		1
#define SCHEDULER_CALC		2
#define SCHEDULER_TRACE		3

#define HASH_MAX	128


struct schedType {
  int			number,
  			repeatOf;
};


class Scheduler
{
  private:

#if defined(_OPENMP) && !defined(DDDS_THREADS_SINGLE)
    omp_lock_t		lock;
#endif
    
    struct listType {
      int		first,
      			last,
			length;
    };

    struct groupType {
      int		strain,
      			hash,
			pred,
			actual,
			head,
			repeatNo;
    };

    struct sortType {
      int		number,
      			value;
    };

    struct handType {
      int		next,
      			spareKey;
      int		NTflag,
			first,
			strain,
      			repeatNo,
			depth,
			strength,
			fanout,
			thread,
			selectFlag,
			time;
    };

    handType		hands[MAXNOOFBOARDS];

    groupType		group[MAXNOOFBOARDS];
    int			numGroups,
    			extraGroups;
#ifdef _WIN32
    LONG volatile	currGroup;
#else
    int volatile	currGroup;
#endif

    listType		list[DDS_SUITS+2][HASH_MAX];

    sortType		sortList[MAXNOOFBOARDS];
    int			sortLen;

    int			threadGroup[MAXNOOFTHREADS],
    			threadCurrGroup[MAXNOOFTHREADS];

    int			threadToHand[MAXNOOFTHREADS];

    int			numHands;

    int			highCards[8192];

    int Strength(
      deal		* dl);

    int Fanout(
      deal		* dl);

    void Reset();

#ifdef _WIN32
    LARGE_INTEGER	timeStart[MAXNOOFTHREADS],
    			timeEnd[MAXNOOFTHREADS],
			blockStart,
			blockEnd;
#else
    int timeDiff(
      timeval 		x, 
      timeval 		y);

    timeval		startTime[MAXNOOFTHREADS],
    			endTime[MAXNOOFTHREADS],
			blockStart,
			blockEnd;
#endif
    
    void MakeGroups(
      boards		* bop);

    void FinetuneGroups();

    void SortSolve(),
         SortCalc(),
	 SortTrace();

#ifdef DDS_SCHEDULER
    FILE		* fp;

    char		fname[80];

    int 		timeHist[10000],
			timeHistNT[10000],
			timeHistSuit[10000];
    struct timeType {
      long long		cum;
      double		cumsq;
      int		number;
    };

    timeType		timeStrain[2],
    			timeRepeat[16],
			timeDepth[60],
			timeStrength[60],
			timeFanout[100],
			timeThread[MAXNOOFTHREADS];
    long long		timeMax,
    			blockMax,
    			timeBlock;
    timeType		timeGroupActualStrain[2],
    			timeGroupPredStrain[2],
			timeGroupDiffStrain[2];

    void InitTimes();

    void PrintTimingList(
      timeType		* tp,
      int		length,
      const char	title[]);
#endif

    int PredictedTime(
      deal		* dl,
      int		number);



  public:

    Scheduler();

    ~Scheduler();

    void SetFile(char * fname);

    void RegisterTraceDepth(
      playTracesBin	* plp,
      int		number);
    
    void Register(
      boards		* bop,
      int		sortMode);
    
    schedType GetNumber(
      int		thrId);
    
#ifdef DDS_SCHEDULER
    void StartThreadTimer(
      int		thrId);

    void EndThreadTimer(
      int		thrId);

    void StartBlockTimer();

    void EndBlockTimer();

    void PrintTiming();
#endif

};

#endif
