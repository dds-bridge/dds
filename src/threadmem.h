/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2015 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_THREADMEM_H
#define DDS_THREADMEM_H

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


struct localVarType
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

  TransTable transTable; // Object

  Moves moves; // Object

#ifdef DDS_AB_STATS
  ABstats ABStats; // Object
#endif

#ifdef DDS_TIMING
  Timer timer; // Object
#endif

#ifdef DDS_TOP_LEVEL
  FILE * fpTopLevel;
#endif

#ifdef DDS_AB_HITS
  FILE * fpRetrieved;
  FILE * fpStored;
#endif

};

extern Scheduler scheduler;

extern struct localVarType localVar[MAXNOOFTHREADS];

#endif
