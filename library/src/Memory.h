/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_MEMORY_H
#define DDS_MEMORY_H

#include <vector>

#include "trans_table/TransTable.h"
// ...existing code...

#include "moves/Moves.h"
#include "File.h"
// debug.h is not required in this header; include it where needed in sources

#ifdef DDS_AB_STATS
  #include "ABstats.h"
#endif

#ifdef DDS_TIMING
  #include "TimerList.h"
#endif

using namespace std;


enum TTmemory
{
  DDS_TT_SMALL = 0,
  DDS_TT_LARGE = 1
};

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

  TransTable * transTable;

  Moves moves;

#ifdef DDS_TOP_LEVEL
  File fileTopLevel;
#endif

#ifdef DDS_AB_STATS
  ABstats ABStats;
  File fileABstats;
#endif

#ifdef DDS_AB_HITS
  File fileRetrieved;
  File fileStored;
#endif

#ifdef DDS_TT_STATS
  File fileTTstats;
#endif 

#ifdef DDS_TIMING
  TimerList timerList;
  File fileTimerList;
#endif

#ifdef DDS_MOVES
  File fileMoves;
#endif

};


/**
 * @brief Thread-local memory manager for bridge double dummy solver.
 *
 * The Memory class manages per-thread resources, including allocation and cleanup
 * of thread-local data structures required for double dummy analysis. It provides
 * interfaces for resizing, accessing, and reporting memory usage for each thread.
 * Memory is an internal component and not part of the public API.
 */
class Memory
{
  private:

    vector<ThreadData *> memory;

    vector<string> threadSizes;

  public:

    /**
     * @brief Construct a new Memory object.
     *
     * Initializes thread-local memory tracking and prepares for allocation.
     */
    Memory();

    /**
     * @brief Destroy the Memory object and clean up resources.
     *
     * Releases all memory and performs cleanup of thread-local resources.
     */
    ~Memory();

    void ReturnThread(const unsigned thrId);

    void Resize(
      const unsigned n,
      const TTmemory flag,
      const int memDefault_MB,
      const int memMaximum_MB);

    unsigned NumThreads() const;

    ThreadData * GetPtr(const unsigned thrId);

    double MemoryInUseMB(const unsigned thrId) const;

    string ThreadSize(const unsigned thrId) const;
};

#endif
