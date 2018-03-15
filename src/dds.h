/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_DDS_H
#define DDS_DDS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "debug.h"
#include "../include/portab.h"

#include "TransTable.h"
#include "Timer.h"
#include "ABstats.h"


#if defined(DDS_MEMORY_LEAKS) && defined(_MSC_VER)
  #define DDS_MEMORY_LEAKS_WIN32
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif


#if defined(SMALL_MEMORY_OPTION)
  #define THREADMEM_MAX_MB 30
  #define THREADMEM_DEF_MB 20
#else
  #define THREADMEM_MAX_MB 160
  #define THREADMEM_DEF_MB 95
#endif

#define MAXNOOFTHREADS 16

#define MAXNODE 1
#define MINNODE 0

#define MOVESVALID 1
#define MOVESLOCKED 2

#define SIMILARDEALLIMIT 5
#define SIMILARMAXWINNODES 700000

#define Max(x, y) (((x) >= (y)) ? (x) : (y))
#define Min(x, y) (((x) <= (y)) ? (x) : (y))

#define DDS_NOTRUMP 4

/* "hand" is leading hand, "relative" is hand relative leading
hand.
The handId macro implementation follows a solution
by Thomas Andrews.
All hand identities are given as
0=NORTH, 1=EAST, 2=SOUTH, 3=WEST. */

#define handId(hand, relative) (hand + relative) & 3


extern int lho[DDS_HANDS];
extern int rho[DDS_HANDS];
extern int partner[DDS_HANDS];

extern unsigned short int bitMapRank[16];

extern unsigned char cardRank[16];
extern unsigned char cardSuit[DDS_STRAINS];
extern unsigned char cardHand[DDS_HANDS];

// These five together take up 440 KB
extern int highestRank[8192];
extern int lowestRank[8192];
extern int counttable[8192];
extern char relRank[8192][15];
extern unsigned short int winRanks[8192][14];


extern int noOfThreads;

struct playparamType
{
  int noOfBoards;
  struct playTracesBin * plp;
  struct solvedPlays * solvedp;
  int error;
};


struct moveGroupType
{
  // There are at most 7 groups of bit "runs" in a 13-bit vector
  int lastGroup;
  int rank[7];
  int sequence[7];
  int fullseq[7];
  int gap[7];
};

extern moveGroupType groupData[8192];


extern int stat_contr[DDS_STRAINS];

struct moveType
{
  int suit;
  int rank;
  int sequence; /* Whether or not this move is the
                                     first in a sequence */
  int weight; /* Weight used at sorting */
};

struct movePlyType
{
  struct moveType move[14];
  int current;
  int last;
};

struct highCardType
{
  int rank;
  int hand;
};


struct pos
{
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS];
  unsigned short int aggr[DDS_SUITS];
  unsigned char length[DDS_HANDS][DDS_SUITS];
  int handDist[DDS_HANDS];

  unsigned short int winRanks[50][DDS_SUITS];
  /* Cards that win by rank, firstindex is depth. */
  int first[50];
  /* Hand that leads the trick for each ply */
  struct moveType move[50];
  /* Presently winning move */
  int handRelFirst;
  /* The current hand, relative first hand */
  int tricksMAX;
  /* Aggregated tricks won by MAX */
  struct highCardType winner[DDS_SUITS];
  /* Winning rank of trick. */
  struct highCardType secondBest[DDS_SUITS];
  /* Second best rank. */
};


struct evalType
{
  int tricks;
  unsigned short int winRanks[DDS_SUITS];
};

struct card
{
  int suit;
  int rank;
};

struct extCard
{
  int suit;
  int rank;
  int sequence;
};

struct paramType
{
  int noOfBoards;
  struct boards * bop;
  struct solvedBoards * solvedp;
  int error;
};

struct absRankType // 2 bytes
{
  char rank;
  char hand;
};

struct relRanksType // 120 bytes
{
  struct absRankType absRank[15][DDS_SUITS];
};

#include "Moves.h"
#include "Scheduler.h"

#endif
