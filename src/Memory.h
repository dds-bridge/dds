/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_MEMORY_H
#define DDS_MEMORY_H

#include <vector>

#include "TransTable.h"
#include "Moves.h"
#include "debug.h"
#include "../include/dll.h"

#ifdef DDS_AB_STATS
  include "ABstats.h"
#endif

#ifdef DDS_TIMING
  include "TimerList.h"
#endif

using namespace std;


struct WinnerEntryType
{
  int suit;
  int winnerRank;
  int winnerHand;
  int secondRank;
  int secondHand;
};

struct WinnersType
{
  int number;
  WinnerEntryType winner[4];
};


struct ThreadData
{
  int nodeTypeStore[DDS_HANDS];
  int iniDepth;
  bool val;

  unsigned short int suit[DDS_HANDS][DDS_SUITS];
  int trump;

  struct pos lookAheadPos; // Recursive alpha-beta data
  bool analysisFlag;
  unsigned short int lowestWin[50][DDS_SUITS];
  WinnersType winners[13];
  struct moveType forbiddenMoves[14];
  struct moveType bestMove[50];
  struct moveType bestMoveTT[50];

  double memUsed;
  int nodes;
  int trickNodes;

  // Constant for a given hand.
  // 960 KB
  struct relRanksType rel[8192];

  TransTable transTable;

  Moves moves;

#ifdef DDS_AB_STATS
  ABstats ABStats;
#endif

#ifdef DDS_TIMING
  TimerList timerList;
#endif

#ifdef DDS_TOP_LEVEL
  FILE * fpTopLevel;
#endif

#ifdef DDS_AB_HITS
  FILE * fpRetrieved;
  FILE * fpStored;
#endif

};


class Memory
{
  private:

    int defThrMB;
    int maxThrMB;

    vector<ThreadData *> memory;
    int nThreads;

  public:

    Memory();

    ~Memory();

    void Reset();

    void ResetThread(const int thrId);

    void ReturnThread(const int thrId);

    void SetThreadSize(
      const int memDefault_MB,
      const int memMaximum_MB);

    void Resize(const int n);

    ThreadData * GetPtr(const int thrId);

    double MemoryInUseMB(const int thrId) const;

    void ReturnAllMemory();

};

#endif
