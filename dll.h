/* portability-macros header prefix */

/* Windows requires a __declspec(dllexport) tag, etc */
#if defined(_WIN32)
#    define DLLEXPORT __declspec(dllexport)
#    define STDCALL __stdcall
#else
#    define DLLEXPORT
#    define STDCALL
#    define INT8 char
#endif

#ifdef __cplusplus
#    define EXTERN_C extern "C"
#else
#    define EXTERN_C
#endif

#if defined(_WIN32) && defined(__MINGW32__) 
#define WINVER 0x500	/* Dirty trick, but it works. */
#    include <windows.h>
#    include <process.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#    include <windows.h>
#    include <process.h>
#endif

#if defined(_MSC_VER)
#    include <intrin.h>
#else
#    include <omp.h>
#endif

/* end of portability-macros section */

#define DDS_VERSION		20300	/* Version 2.3.0. Allowing for 2 digit
					minor versions */

#define PBN

#define PBN_PLUS

/*#define BENCH*/

#include <stdio.h>
/*#define _CRTDBG_MAP_ALLOC */ /* MEMORY LEAK? */
#include <stdlib.h>
/*#include <crtdbg.h> */  /* MEMORY LEAK? */
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>

/*#define STAT*/	/* Define STAT to generate a statistics log, stat.txt */
/*#define TTDEBUG*/     /* Define TTDEBUG to generate transposition table debug information.
						Only for a single thread! */

#ifdef  TTDEBUG
#define SEARCHSIZE  20000
#else
#define SEARCHSIZE  1
#endif

#if defined(INFINITY)
#    undef INFINITY
#endif
#define INFINITY    32000

#define MAXNOOFTHREADS	16

#define MAXNODE     1
#define MINNODE     0

#define TRUE        1
#define FALSE       0

#define MOVESVALID  1
#define MOVESLOCKED 2

#define NSIZE	100000
#define WSIZE   100000
#define LSIZE   20000
#define NINIT	250000
#define WINIT	700000
#define LINIT	50000

#define SIMILARDEALLIMIT	5
#define SIMILARMAXWINNODES  700000

#define MAXNOOFBOARDS		100

#define Max(x, y) (((x) >= (y)) ? (x) : (y))
#define Min(x, y) (((x) <= (y)) ? (x) : (y))

/* "hand" is leading hand, "relative" is hand relative leading
hand.
The handId macro implementation follows a solution 
by Thomas Andrews. 
All hand identities are given as
0=NORTH, 1=EAST, 2=SOUTH, 3=WEST. */

#define handId(hand, relative) (hand + relative) & 3
#define CMP_SWAP(i, j) if (a[i].weight < a[j].weight)  \
  { struct moveType tmp = a[i]; a[i] = a[j]; a[j] = tmp; } 


struct gameInfo  {          /* All info of a particular deal */
  int declarer;
  int leadHand;
  int leadSuit;
  int leadRank;
  int first;
  int noOfCards;
  unsigned short int suit[4][4];
    /* 1st index is hand id, 2nd index is suit id */
};


struct moveType {
  unsigned char suit;
  unsigned char rank;
  unsigned short int sequence;          /* Whether or not this move is
                                        the first in a sequence */
  short int weight;                     /* Weight used at sorting */
};

struct movePlyType {
  struct moveType move[14];             
  int current;
  int last;
};

struct highCardType {
  int rank;
  int hand;
};

struct futureTricks {
  int nodes;
  #ifdef BENCH
  int totalNodes;
  #endif
  int cards;
  int suit[13];
  int rank[13];
  int equals[13];
  int score[13];
};

struct deal {
  int trump;
  int first;
  int currentTrickSuit[3];
  int currentTrickRank[3];
  unsigned int remainCards[4][4];
};


struct dealPBN {
  int trump;
  int first;
  int currentTrickSuit[3];
  int currentTrickRank[3];
  char remainCards[80];
};


struct pos {
  unsigned short int rankInSuit[4][4];   /* 1st index is hand, 2nd index is
                                        suit id */
  int orderSet[4];
  int winOrderSet[4];
  int winMask[4];
  int leastWin[4];
  unsigned short int removedRanks[4];    /* Ranks removed from board,
                                        index is suit */
  unsigned short int winRanks[50][4];  /* Cards that win by rank,
                                       indices are depth and suit */
  unsigned char length[4][4];
  char ubound;
  char lbound;
  char bestMoveSuit;
  char bestMoveRank;
  int first[50];                 /* Hand that leads the trick for each ply*/
  int high[50];                  /* Hand that is presently winning the trick */
  struct moveType move[50];      /* Presently winning move */              
  int handRelFirst;              /* The current hand, relative first hand */
  int tricksMAX;                 /* Aggregated tricks won by MAX */
  struct highCardType winner[4]; /* Winning rank of the trick,
                                    index is suit id. */
  struct highCardType secondBest[4]; /* Second best rank, index is suit id. */
};

struct posSearchType {
  struct winCardType * posSearchPoint; 
  long long suitLengths;
  struct posSearchType * left;
  struct posSearchType * right;
};


struct nodeCardsType {
  char ubound;	/* ubound and
			lbound for the N-S side */
  char lbound;
  char bestMoveSuit;
  char bestMoveRank;
  char leastWin[4];
};

struct winCardType {
  int orderSet;
  int winMask;
  struct nodeCardsType * first;
  struct winCardType * prevWin;
  struct winCardType * nextWin;
  struct winCardType * next;
}; 


struct evalType {
  int tricks;
  unsigned short int winRanks[4];
};

struct absRankType {
  char rank;
  char hand;
};

struct relRanksType {
  int aggrRanks[4];
  int winMask[4];
  char relRank[15][4];
  struct absRankType absRank[15][4];
};

struct adaptWinRanksType {
  unsigned short int winRanks[14];
};


struct ttStoreType {
  struct nodeCardsType * cardsP;
  char tricksLeft;
  char target;
  char ubound;
  char lbound;
  unsigned char first;
  unsigned short int suit[4][4];
};

struct card {
  int suit;
  int rank;
};

struct boards {
  int noOfBoards;
  struct deal deals[MAXNOOFBOARDS];
  int target[MAXNOOFBOARDS];
  int solutions[MAXNOOFBOARDS];
  int mode[MAXNOOFBOARDS];
};

struct boardsPBN {
  int noOfBoards;
  struct dealPBN deals[MAXNOOFBOARDS];
  int target[MAXNOOFBOARDS];
  int solutions[MAXNOOFBOARDS];
  int mode[MAXNOOFBOARDS];
};

struct solvedBoards {
  int noOfBoards;
  struct futureTricks solvedBoard[MAXNOOFBOARDS];
};

struct paramType {
  int noOfBoards;
  struct boards *bop;
  struct solvedBoards *solvedp;
  int error;
};

struct ddTableDeal {
  unsigned int cards[4][4];
};

struct ddTableDealPBN {
  char cards[80];
};

struct ddTableResults {
  int resTable[5][4];
};

struct parResults {
  char parScore[2][16];	/* index = 0 is NS view and index = 1 is EW view. */
  char parContractsString[2][128]; /* index = 0 is NS view and index = 1 
				      is EW view. By “view” is here meant 
				      which side that starts the bidding. */
};

struct par_suits_type {
  int suit;
  int tricks;
  int score;
}; 


struct localVarType {
  int nodeTypeStore[4];
  int trump;
  unsigned short int lowestWin[50][4];
  int nodes;
  int trickNodes;
  int no[50];
  int iniDepth;
  int handToPlay;
  int payOff;
  int val;
  struct pos iniPosition;
  struct pos lookAheadPos; /* Is initialized for starting
			      alpha-beta search */
  struct moveType forbiddenMoves[14];
  struct moveType initialMoves[4];
  struct moveType cd;
  struct movePlyType movePly[50];
  int tricksTarget;
  struct gameInfo game;
  int newDeal;
  int newTrump;
  int similarDeal;
  unsigned short int diffDeal;
  unsigned short int aggDeal;
  int estTricks[4];
  FILE *fp2;
  FILE *fp7;
  FILE *fp11;
  /*FILE *fdp */
  struct moveType bestMove[50];
  struct moveType bestMoveTT[50];
  struct winCardType temp_win[5];
  int nodeSetSizeLimit;
  int winSetSizeLimit;
  int lenSetSizeLimit;
  unsigned long long maxmem;		/* bytes */
  unsigned long long allocmem;
  unsigned long long summem;
  int wmem;
  int nmem; 
  int lmem;
  int maxIndex;
  int wcount;
  int ncount;
  int lcount;
  int clearTTflag;
  int windex;
  /*int ttCollect;
  int suppressTTlog;*/
  struct relRanksType * rel;
  struct adaptWinRanksType * adaptWins;
  struct posSearchType *rootnp[14][4];
  struct winCardType **pw;
  struct nodeCardsType **pn;
  struct posSearchType **pl;
  struct nodeCardsType * nodeCards;
  struct winCardType * winCards;
  struct posSearchType * posSearch;
  /*struct ttStoreType * ttStore;
  int lastTTstore;*/
  unsigned short int iniRemovedRanks[4];

  int nodeSetSize; /* Index with range 0 to nodeSetSizeLimit */
  int winSetSize;  /* Index with range 0 to winSetSizeLimit */
  int lenSetSize;  /* Index with range 0 to lenSetSizeLimit */
};

extern int noOfThreads;
extern int noOfCores;
extern struct localVarType localVar[MAXNOOFTHREADS];
extern int * highestRank;
extern int * counttable;
extern struct adaptWinRanksType * adaptWins;
extern unsigned short int bitMapRank[16];
extern unsigned short int relRankInSuit[4][4];
extern int sum;
extern int score1Counts[50], score0Counts[50];
extern int c1[50], c2[50], c3[50], c4[50], c5[50], c6[50], c7[50],
  c8[50], c9[50];
extern int nodeTypeStore[4];            /* Look-up table for determining if
                                        node is MAXNODE or MINNODE */
extern int lho[4], rho[4], partner[4];                                        
extern int nodes;                       /* Number of nodes searched */
extern int no[50];                      /* Number of nodes searched on each
                                        depth level */
extern int payOff;
extern int iniDepth;
extern int treeDepth;
extern int tricksTarget;                /* No of tricks for MAX in order to
                                        meet the game goal, e.g. to make the
                                        contract */
extern int tricksTargetOpp;             /* Target no of tricks for MAX
                                        opponent */
extern int targetNS;
extern int targetEW;
extern int handToPlay;
extern int searchTraceFlag;
extern int countMax;
extern int depthCount;
extern int highHand;
extern int nodeSetSizeLimit;
extern int winSetSizeLimit;
extern int lenSetSizeLimit;
extern int estTricks[4];
extern int recInd; 
extern int suppressTTlog;
extern unsigned char suitChar[4];
extern unsigned char rankChar[15];
extern unsigned char handChar[4];
extern unsigned char cardRank[15], cardSuit[5], cardHand[4];
extern int totalNodes;
extern struct futureTricks fut, ft;
extern struct futureTricks *futp;
extern char stri[128];

extern FILE *fp, /**fp2, *fp7, *fp11,*/ *fpx;
  /* Pointers to logs */

extern struct ttStoreType * ttStore;
extern int lastTTstore;
extern int ttCollect;
extern int suppressTTlog;

EXTERN_C DLLEXPORT int STDCALL SolveBoard(struct deal dl, 
  int target, int solutions, int mode, struct futureTricks *futp, int threadIndex);
EXTERN_C DLLEXPORT int STDCALL CalcDDtable(struct ddTableDeal tableDeal, 
  struct ddTableResults * tablep);

#ifdef PBN
EXTERN_C DLLEXPORT int STDCALL SolveBoardPBN(struct dealPBN dlpbn, int target, 
    int solutions, int mode, struct futureTricks *futp, int thrId);
EXTERN_C DLLEXPORT int STDCALL CalcDDtablePBN(struct ddTableDealPBN tableDealPBN, 
  struct ddTableResults * tablep);
#endif

#ifdef PBN_PLUS
EXTERN_C DLLEXPORT int STDCALL SolveAllBoards(struct boardsPBN *bop, struct solvedBoards *solvedp);
EXTERN_C DLLEXPORT int STDCALL CalcPar(struct ddTableDeal tableDeal, int vulnerable, 
    struct ddTableResults * tablep, struct parResults *presp);
EXTERN_C DLLEXPORT int STDCALL CalcParPBN(struct ddTableDealPBN tableDealPBN, 
  struct ddTableResults * tablep, int vulnerable, struct parResults *presp);
#endif

#ifdef __linux
EXTERN_C void InitStart(int gb_ram, int ncores);   /* For usage with ctypes in Linux. */ 
#endif

void InitStart(int gb_ram, int ncores);
void InitGame(int gameNo, int moveTreeFlag, int first, int handRelFirst, int thrId);
void InitSearch(struct pos * posPoint, int depth,
  struct moveType startMoves[], int first, int mtd, int thrId);
int ABsearch(struct pos * posPoint, int target, int depth, int thrId);
void Make(struct pos * posPoint, unsigned short int trickCards[4], 
  int depth, int trump, struct movePlyType *mply, int thrId);
int MoveGen(struct pos * posPoint, int depth, int trump, struct movePlyType *mply, int thrId);
void MergeSort(int n, struct moveType *a);
inline int WinningMove(struct moveType * mvp1, struct moveType * mvp2, int trump, int thrId);
inline int WinningMoveNT(struct moveType * mvp1, struct moveType * mvp2, int thrId);
int AdjustMoveList(int thrId);
int QuickTricks(struct pos * posPoint, int hand, 
	int depth, int target, int trump, int *result, int thrId);
int LaterTricksMIN(struct pos *posPoint, int hand, int depth, int target, int trump, int thrId); 
int LaterTricksMAX(struct pos *posPoint, int hand, int depth, int target, int trump, int thrId);
struct nodeCardsType * CheckSOP(struct pos * posPoint, struct nodeCardsType
  * nodep, int target, int tricks, int * result, int *value, int thrId);
struct nodeCardsType * UpdateSOP(struct pos * posPoint, struct nodeCardsType
  * nodep);  
struct nodeCardsType * FindSOP(struct pos * posPoint,
  struct winCardType * nodeP, int firstHand, 
	int target, int tricks, int * valp, int thrId);  
struct nodeCardsType * BuildPath(struct pos * posPoint, 
  struct posSearchType *nodep, int * result, int thrId);
void BuildSOP(struct pos * posPoint, int tricks, int firstHand, int target,
  int depth, int scoreFlag, int score, int thrId);
struct posSearchType * SearchLenAndInsert(struct posSearchType
	* rootp, long long key, int insertNode, int *result, int thrId);  
void Undo(struct pos * posPoint, int depth, int thrId);
int CheckDeal(struct moveType * cardp, int thrId);
int InvBitMapRank(unsigned short bitMap);
int InvWinMask(int mask);
void ReceiveTTstore(struct pos *posPoint, struct nodeCardsType * cardsP, int target, 
  int depth, int thrId);
int NextMove(struct pos *posPoint, int depth, struct movePlyType *mply, int thrId); 
int DumpInput(int errCode, struct deal dl, int target, int solutions, int mode); 
void Wipe(int thrId);
void AddNodeSet(int thrId);
void AddLenSet(int thrId);
void AddWinSet(int thrId);

void PrintDeal(FILE *fp, unsigned short ranks[4][4]);

int SolveAllBoards4(struct boards *bop, struct solvedBoards *solvedp);
int SolveAllBoards1(struct boards *bop, struct solvedBoards *solvedp);




