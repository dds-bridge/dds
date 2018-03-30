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
#include "File.h"
#include "debug.h"

#ifdef DDS_AB_STATS
  #include "ABstats.h"
#endif

#ifdef DDS_TIMING
  #include "TimerList.h"
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

  pos lookAheadPos; // Recursive alpha-beta data
  bool analysisFlag;
  unsigned short int lowestWin[50][DDS_SUITS];
  WinnersType winners[13];
  moveType forbiddenMoves[14];
  moveType bestMove[50];
  moveType bestMoveTT[50];

  double memUsed;
  int nodes;
  int trickNodes;

  // Constant for a given hand.
  // 960 KB
  relRanksType rel[8192];

  TransTable transTable;

  Moves moves;

#ifdef DDS_AB_STATS
  ABstats ABStats;
  File fileABstats;
#endif

#ifdef DDS_MOVES
  File fileMoves;
#endif

#ifdef DDS_TIMING
  TimerList timerList;
  File fileTimerList;
#endif

#ifdef DDS_TOP_LEVEL
  File fileTopLevel;
#endif

#ifdef DDS_AB_HITS
  File fileRetrieved;
  File fileStored;
#endif

};


class Memory
{
  private:

    int defThrMB;
    int maxThrMB;

    vector<ThreadData *> memory;
    unsigned nThreads;

  public:

    Memory();

    ~Memory();

    void Reset();

    void ResetThread(const unsigned thrId);

    void ReturnThread(const unsigned thrId);

    void SetThreadSize(
      const int memDefault_MB,
      const int memMaximum_MB);

    void Resize(const unsigned n);

    ThreadData * GetPtr(const unsigned thrId);

    double MemoryInUseMB(const unsigned thrId) const;

    void ReturnAllMemory();

};

#endif
