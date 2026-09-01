#ifndef DDS_THREAD_DATA_H
#define DDS_THREAD_DATA_H

#include <utility/debug.h>

#include <api/dds_data_types.hpp>
#include <moves/moves.hpp>
#include <string>

#ifdef DDS_AB_STATS
#include "ab_stats.hpp"
#endif

#if defined(DDS_TOP_LEVEL) || defined(DDS_AB_STATS) || defined(DDS_AB_HITS) || \
    defined(DDS_TT_STATS) || defined(DDS_TIMING) || defined(DDS_MOVES)
#include "file.hpp"
#endif

#ifdef DDS_TIMING
  #include <system/timer_list.hpp>
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

  Pos lookAheadPos; // Recursive alpha-beta data
  bool analysisFlag;
  unsigned short int lowestWin[50][DDS_SUITS];
  WinnersType winners[13];
  MoveType forbiddenMoves[14];
  MoveType bestMove[50];
  MoveType bestMoveTT[50];

  double memUsed;
  int nodes;
  int trickNodes;

  // Constant for a given hand.
  // 960 KB
  RelRanksType rel[8192];

  // Deferred TT configuration for context-owned construction
  // TransTable configuration moved to SolverContext::SolverConfig and
  // per-context member in SolverContext::SearchContext.

  Moves moves;

#ifdef DDS_TOP_LEVEL
  dds::File fileTopLevel;
#endif

#ifdef DDS_AB_STATS
  ABstats ABStats;
  dds::File fileABstats;
#endif

#ifdef DDS_AB_HITS
  dds::File fileRetrieved;
  dds::File fileStored;
#endif

#ifdef DDS_TT_STATS
  dds::File fileTTstats;
#endif 

#ifdef DDS_TIMING
  TimerList timerList;
  dds::File fileTimerList;
#endif

#ifdef DDS_MOVES
  dds::File fileMoves;
#endif

  // True after init_debug_files(); cleared by close_debug_files().
  bool debug_files_initialized_ = false;

  // Initialize per-thread debug/stat files. suffix is appended to each debug
  // prefix (e.g. "0.txt" from SolverContext serial + DDS_DEBUG_SUFFIX).
  void init_debug_files([[maybe_unused]] const std::string& suffix);

  // Close any open per-thread debug/stat files.
  void close_debug_files();

  auto debug_files_initialized() const -> bool
  {
    return debug_files_initialized_;
  }
};


#endif // DDS_THREAD_DATA_H