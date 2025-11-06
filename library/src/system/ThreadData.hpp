#ifndef DDS_THREAD_DATA_H
#define DDS_THREAD_DATA_H

#include <api/dds.h>
#include <moves/Moves.h>
#include <string>


#ifdef DDS_AB_STATS
  #include "ABstats.h"
#endif

#ifdef DDS_TIMING
  #include "TimerList.h"
#endif
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

  // Deferred TT configuration for context-owned construction
  TTmemory ttType = DDS_TT_SMALL;
  int ttMemDefault_MB = 0;
  int ttMemMaximum_MB = 0;

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

  // Initialize per-thread debug/stat files with a suffix (e.g., "<thrId>_suffix").
  void init_debug_files(const std::string& suffix);

  // Close any open per-thread debug/stat files.
  void close_debug_files();
};


#endif // DDS_THREAD_DATA_H