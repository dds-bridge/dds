/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SCHEDULER_H
#define DDS_SCHEDULER_H

#include <atomic>

#include "TimeStatList.h"
#include "dds.h"

using namespace std;


enum SchedulerMode
{
  SCHEDULER_NOSORT,
  SCHEDULER_SOLVE,
  SCHEDULER_CALC,
  SCHEDULER_TRACE
};

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

    atomic<int> currGroup;

    listType list[DDS_SUITS + 2][HASH_MAX];

    sortType sortList[MAXNOOFBOARDS];
    int sortLen;

    int threadGroup[MAXNOOFTHREADS];
    int threadCurrGroup[MAXNOOFTHREADS];

    int threadToHand[MAXNOOFTHREADS];

    int numHands;

    vector<int> highCards;

    void InitHighCards();

    void SortHands(const enum SchedulerMode mode);

    int Strength(
      deal const * dl);

    int Fanout(
      deal const * dl);

    void Reset();

    vector<Timer> timersThread;
    Timer timerBlock;

    void MakeGroups(
      boards const * bop);

    void FinetuneGroups();

    bool SameHand(
      int hno1,
      int hno2);

    void SortSolve(),
         SortCalc(),
         SortTrace();

#ifdef DDS_SCHEDULER

    int timeHist[10000];
    int timeHistNT[10000];
    int timeHistSuit[10000];

    TimeStatList timeStrain;
    TimeStatList timeRepeat;
    TimeStatList timeDepth;
    TimeStatList timeStrength;
    TimeStatList timeFanout;
    TimeStatList timeThread;
    TimeStatList timeGroupActualStrain;
    TimeStatList timeGroupPredStrain;
    TimeStatList timeGroupDiffStrain;

    long long timeMax;
    long long blockMax;
    long long timeBlock;

    void InitTimes();
#endif

    int PredictedTime(
      deal * dl,
      int number);


  public:

    Scheduler();

    ~Scheduler();

    void RegisterThreads(
      const int n);

    void RegisterRun(
      const enum SchedulerMode mode,
      boards const * bop,
      playTracesBin const * plp);

    void RegisterRun(
      const enum SchedulerMode mode,
      boards const * bop);

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
