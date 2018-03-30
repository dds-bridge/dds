/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SCHEDULER_H
#define DDS_SCHEDULER_H

#include <atomic>

#include "dds.h"
#include "TimeStatList.h"
#include "Timer.h"

using namespace std;

#define HASH_MAX 200

#ifdef DDS_SCHEDULER
  #define START_BLOCK_TIMER scheduler.StartBlockTimer()
  #define END_BLOCK_TIMER scheduler.EndBlockTimer()
  #define START_THREAD_TIMER(a) scheduler.StartThreadTimer(a)
  #define END_THREAD_TIMER(a) scheduler.EndThreadTimer(a)
#else
  #define START_BLOCK_TIMER 1
  #define END_BLOCK_TIMER 1
  #define START_THREAD_TIMER(a) 1
  #define END_THREAD_TIMER(a) 1
#endif


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

    vector<int> threadGroup;
    vector<int> threadCurrGroup;
    vector<int> threadToHand;

    int numThreads;
    int numHands;

    vector<int> highCards;

    void InitHighCards();

    void SortHands(const enum RunMode mode);

    int Strength(const deal& dl) const;
    int Fanout(const deal& dl) const;

    void Reset();

    vector<Timer> timersThread;
    Timer timerBlock;

    void MakeGroups(const boards& bds);

    void FinetuneGroups();

    bool SameHand(
      const int hno1,
      const int hno2) const;

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
      deal& dl,
      int number) const;


  public:

    Scheduler();

    ~Scheduler();

    void RegisterThreads(
      const int n);

    void RegisterRun(
      const enum RunMode mode,
      const boards& bds,
      const playTracesBin& pl);

    void RegisterRun(
      const enum RunMode mode,
      const boards& bds);

    schedType GetNumber(const int thrId);

    int NumGroups() const;

#ifdef DDS_SCHEDULER
    void StartThreadTimer(const int thrId);

    void EndThreadTimer(const int thrId);

    void StartBlockTimer();

    void EndBlockTimer();

    void PrintTiming() const;
#endif

};

#endif
