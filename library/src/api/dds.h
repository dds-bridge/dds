/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include "portab.h"
#include "dll.h"


#if defined(DDS_MEMORY_LEAKS) && defined(_MSC_VER)
  #define DDS_MEMORY_LEAKS_WIN32
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif


#define THREADMEM_SMALL_MAX_MB 30
#define THREADMEM_SMALL_DEF_MB 20
#define THREADMEM_LARGE_MAX_MB 160
#define THREADMEM_LARGE_DEF_MB 95

#define MAXNODE 1
#define MINNODE 0

#define SIMILARDEALLIMIT 5
#define SIMILARMAXWINNODES 700000


/* "hand" is leading hand, "relative" is hand relative leading
hand.
The handId macro implementation follows a solution
by Thomas Andrews.
All hand identities are given as
0=NORTH, 1=EAST, 2=SOUTH, 3=WEST. */

#include "utility/constants.h"

/**
 * @brief Calculate relative hand position.
 * @param hand Base hand position (0=NORTH, 1=EAST, 2=SOUTH, 3=WEST)
 * @param relative Relative offset (0-3)
 * @return Resulting hand position (0-3)
 */
#define HAND_ID(hand, relative) ((hand + relative) & 3)


struct MoveType
{
  int suit;
  int rank;
  int sequence; /* Whether or not this move is the
                                     first in a sequence */
  int weight; /* Weight used at sorting */
};

struct MovePlyType
{
  MoveType move[14];
  int current;
  int last;
};

struct HighCardType
{
  int rank;
  int hand;
};

struct Pos
{
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS];
  unsigned short int aggr[DDS_SUITS];
  unsigned char length[DDS_HANDS][DDS_SUITS];
  int handDist[DDS_HANDS];

  unsigned short int winRanks[50][DDS_SUITS];
  /* Cards that win by rank, firstindex is depth. */
  int first[50];
  /* Hand that leads the trick for each ply */
  MoveType move[50];
  /* Presently winning move */
  int handRelFirst;
  /* The current hand, relative first hand */
  int tricksMAX;
  /* Aggregated tricks won by MAX */
  HighCardType winner[DDS_SUITS];
  /* Winning rank of trick. */
  HighCardType secondBest[DDS_SUITS];
  /* Second best rank. */
};

struct TrickDataType
{
  int playCount[DDS_SUITS];
  int bestRank;
  int bestSuit;
  int bestSequence;
  int relWinner;
  int nextLeadHand;
};

struct EvalType
{
  int tricks;
  unsigned short int winRanks[DDS_SUITS];
};

struct Card
{
  int suit;
  int rank;
};

struct ExtCard
{
  int suit;
  int rank;
  int sequence;
};

struct AbsRankType // 2 bytes
{
  char rank;
  signed char hand;
};

struct RelRanksType // 120 bytes
{
  AbsRankType absRank[15][DDS_SUITS];
};

struct ParamType
{
  int noOfBoards;
  Boards * bop;
  SolvedBoards * solvedp;
  int error;
};

enum class RunMode
{
  DDS_RUN_SOLVE = 0,
  DDS_RUN_CALC = 1,
  DDS_RUN_TRACE = 2,
  DDS_RUN_SIZE = 3
};
