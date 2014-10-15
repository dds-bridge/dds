#ifndef DDS_DDSH
#define DDS_DDSH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "debug.h"
#include "portab.h"

#include "TransTable.h"
#include "Timer.h"
#include "ABstats.h"

#if defined(DDS_MEMORY_LEAKS) && defined(_MSC_VER)
  #define DDS_MEMORY_LEAKS_WIN32
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif


#define	THREADMEM_MAX_MB	160
#define THREADMEM_DEF_MB	 95

#define MAXNOOFTHREADS		16

#define MAXNODE     		1
#define MINNODE     		0

#define MOVESVALID  		1
#define MOVESLOCKED 		2

#define SIMILARDEALLIMIT	5
#define SIMILARMAXWINNODES      700000

#define Max(x, y) (((x) >= (y)) ? (x) : (y))
#define Min(x, y) (((x) <= (y)) ? (x) : (y))

#define DDS_NOTRUMP 		4

/* "hand" is leading hand, "relative" is hand relative leading
hand.
The handId macro implementation follows a solution 
by Thomas Andrews. 
All hand identities are given as
0=NORTH, 1=EAST, 2=SOUTH, 3=WEST. */

#define handId(hand, relative) (hand + relative) & 3
#define CMP_SWAP(i, j) if (a[i].weight < a[j].weight)  \
  { struct moveType tmp = a[i]; a[i] = a[j]; a[j] = tmp; } 


extern int 			lho[DDS_HANDS];
extern int 			rho[DDS_HANDS];
extern int 			partner[DDS_HANDS];

extern unsigned short int 	bitMapRank[16];

extern unsigned char 		cardRank[16];
extern unsigned char 		cardSuit[DDS_STRAINS];
extern unsigned char 		cardHand[DDS_HANDS];

// These four together take up 408 KB
extern int 			highestRank[8192];
extern int 			counttable[8192];
extern char			relRank[8192][15];
extern unsigned short int 	winRanks[8192][14];

extern int 			noOfThreads;


extern int 			stat_contr[DDS_STRAINS];

struct moveType {
  int 			suit;
  int 			rank;
  unsigned short int 	sequence; /* Whether or not this move is the 
                                     first in a sequence */
  short int 		weight;   /* Weight used at sorting */
};

struct movePlyType {
  struct moveType 	move[14];
  int 			current;
  int 			last;
};

struct highCardType {
  int 			rank;
  int 			hand;
};


struct pos {
  unsigned short int 	rankInSuit[DDS_HANDS][DDS_SUITS];   
  unsigned short int	aggr[DDS_SUITS];

  unsigned short int 	removedRanks[DDS_SUITS]; 
  			/* Ranks removed from board.  */
  unsigned short int 	winRanks[50][DDS_SUITS]; 
  			/* Cards that win by rank, firstindex 
			   is depth. */
  unsigned char 	length[DDS_HANDS][DDS_SUITS];
  int			handDist[DDS_HANDS];
  int 			first[50];       
  			/* Hand that leads the trick for each ply   */
  int 			high[50];        
  			/* Hand that is presently winning the trick */
  struct 		moveType move[50];  
  			/* Presently winning move                   */
  int 			handRelFirst;              
  			/* The current hand, relative first hand    */
  int 			tricksMAX;
 			 /* Aggregated tricks won by MAX            */
  struct highCardType 	winner[DDS_SUITS]; 
  			/* Winning rank of trick. */
  struct highCardType 	secondBest[DDS_SUITS]; 
    			/* Second best rank.      */
};

struct evalType {
  int 			tricks;
  unsigned short int 	winRanks[DDS_SUITS];
};

struct card {
  int 			suit;
  int 			rank;
};

struct paramType {
  int 			noOfBoards;
  struct boards 	* bop;
  struct solvedBoards 	* solvedp;
  int 			error;
};

struct gameInfo // 56 bytes
{          
  /* All info of a particular deal */
  int 			declarer;
  int 			leadHand;
  int 			leadSuit;
  int 			leadRank;
  int 			first;
  int 			noOfCards;
  unsigned short int 	suit[DDS_HANDS][DDS_SUITS];
};

struct absRankType // 2 bytes
{
  char 			rank;
  char 			hand;
};

struct relRanksType // 120 bytes
{
  struct absRankType 	absRank[15][DDS_SUITS];
};


struct localVarType 
{
  int 			nodeTypeStore[DDS_HANDS];
  int 			nodes;
  int 			trickNodes;

  double		memUsed;

  int 			trump;
  unsigned short int 	lowestWin[50][DDS_SUITS];
  int 			no[50];
  int 			iniDepth;
  int 			handToPlay;
  int 			payOff;
  bool 			val;

  struct pos 		iniPosition;
  struct pos 		lookAheadPos; 
  			/* Is initialized for starting 
			   alpha-beta search */
  struct moveType 	forbiddenMoves[14];
  struct moveType 	initialMoves[DDS_HANDS];
  struct movePlyType 	movePly[50];
  int 			tricksTarget;
  struct gameInfo 	game;
  bool 			newDeal;
  bool 			newTrump;
  bool 			similarDeal;
  unsigned short int 	diffDeal;
  unsigned short int 	aggDeal;
  int 			estTricks[DDS_HANDS];


  struct moveType 	bestMove[50];
  struct moveType 	bestMoveTT[50];
  unsigned short int 	iniRemovedRanks[DDS_SUITS];

  // Constant for a given hand.
  // 960 KB
  struct relRanksType 	rel[8192];

  TransTable		transTable; 	// Object

#ifdef DDS_AB_STATS
  ABstats		ABstats; 	// Object
#endif

#ifdef DDS_TIMING
  Timer			timer; 		// Object
#endif

#ifdef DDS_TOP_LEVEL
  FILE			* fpTopLevel;
#endif

#ifdef DDS_AB_HITS
  FILE			* fpRetrieved;
  FILE			* fpStored;
#endif

};

extern struct localVarType localVar[MAXNOOFTHREADS];


#endif
