/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SCHEDULER_H
#define DDS_SCHEDULER_H

#include <atomic>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dds.h"
#include "../include/dll.h"

#ifndef _WIN32
  #include <sys/time.h>
#endif

#define SCHEDULER_NOSORT 0
#define SCHEDULER_SOLVE 1
#define SCHEDULER_CALC 2
#define SCHEDULER_TRACE 3

#define HASH_MAX 200


struct schedType
{
  int number;
  int repeatOf;
};


class Scheduler
{
  private:

    struct listType
    {
      int first;
      int last;
      int length;
    };

    struct groupType
    {
      int strain;
      int hash;
      int pred;
      int actual;
      int head;
      int repeatNo;
    };

    struct sortType
    {
      int number;
      int value;
    };

    struct handType
    {
      int next;
      int spareKey;
      unsigned remainCards[DDS_HANDS][DDS_SUITS];
      int NTflag;
      int first;
      int strain;
      int repeatNo;
      int depth;
      int strength;
      int fanout;
      int thread;
      int selectFlag;
      int time;
    };

    handType hands[MAXNOOFBOARDS];

    groupType group[MAXNOOFBOARDS];
    int numGroups;
    int extraGroups;

    std::atomic<int> currGroup;

    listType list[DDS_SUITS + 2][HASH_MAX];

    sortType sortList[MAXNOOFBOARDS];
    int sortLen;

    int threadGroup[MAXNOOFTHREADS];
    int threadCurrGroup[MAXNOOFTHREADS];

    int threadToHand[MAXNOOFTHREADS];

    int numHands;

    int highCards[8192];

    int Strength(
      deal * dl);

    int Fanout(
      deal * dl);

    void Reset();

#ifdef _WIN32
    LARGE_INTEGER timeStart[MAXNOOFTHREADS];
    LARGE_INTEGER timeEnd[MAXNOOFTHREADS];
    LARGE_INTEGER blockStart;
    LARGE_INTEGER blockEnd;
#else
    int timeDiff(
      timeval x,
      timeval y);

    timeval startTime[MAXNOOFTHREADS];
    timeval endTime[MAXNOOFTHREADS];
    timeval blockStart;
    timeval blockEnd;
#endif

    void CreateLock();

    void DestroyLock();

    void MakeGroups(
      boards * bop);

    void FinetuneGroups();

    bool SameHand(
      int hno1,
      int hno2);

    void SortSolve(),
         SortCalc(),
         SortTrace();

#ifdef DDS_SCHEDULER
    FILE * fp;

    char fname[80];

    int timeHist[10000];
    int timeHistNT[10000];
    int timeHistSuit[10000];

    struct timeType
    {
      long long cum;
      double cumsq;
      int number;
    };

    timeType timeStrain[2];
    timeType timeRepeat[16];
    timeType timeDepth[60];
    timeType timeStrength[60];
    timeType timeFanout[100];
    timeType timeThread[MAXNOOFTHREADS];

    long long timeMax;
    long long blockMax;
    long long timeBlock;

    timeType timeGroupActualStrain[2];
    timeType timeGroupPredStrain[2];
    timeType timeGroupDiffStrain[2];

    void InitTimes();

    void PrintTimingList(
      timeType * tp,
      int length,
      const char title[]);
#endif

    int PredictedTime(
      deal * dl,
      int number);


  public:

    Scheduler();

    ~Scheduler();

    void SetFile(char * fname);

    void RegisterTraceDepth(
      playTracesBin * plp,
      int number);

    void Register(
      boards * bop,
      int sortMode);

    schedType GetNumber(
      int thrId);

#ifdef DDS_SCHEDULER
    void StartThreadTimer(
      int thrId);

    void EndThreadTimer(
      int thrId);

    void StartBlockTimer();

    void EndBlockTimer();

    void PrintTiming();
#endif

};

#endif
