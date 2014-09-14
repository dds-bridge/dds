
/* portability-macros header prefix */

/* Windows requires a __declspec(dllexport) tag, etc */
#if defined(_WIN32) || defined(__CYGWIN__)
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
#define WINVER 0x500
#    include <windows.h>
#    include <process.h>
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#    include <windows.h>
#    include <process.h>
#endif


#if defined(_OPENMP)
#    include <omp.h>
#endif


/*#define DDS_THREADS_SINGLE*/  /* For Windows, forces DDS to use a single thread in its
				 multi-thread functions.This is valid both for the Win32
				 thread support alternative and for OpenMP. */

/* When OpenMP support is included in DDS, which leads to
   the macro _OPENMP definition, OpenMP 
   is automatically selected instead of the Win32 API. */ 

/* end of portability-macros section */

#define DDS_VERSION	20600	/* Version 2.6.0. Allowing for 2 digit minor versions */


/*#define DEALER_PAR_ENGINE_ONLY*/  /* The DealerPar engine supplies the results
				       in the output format used for Par. This
				       facilitates comparing that the 2 engines give
				       the same results. */

#include <stdio.h>
/*#define _CRTDBG_MAP_ALLOC */ /* MEMORY LEAK? */
#include <stdlib.h>
/*#include <crtdbg.h> */  /* MEMORY LEAK? */
#include <string.h>
#include <time.h>
#include <assert.h>
#include <math.h>


/*#define STAT*/	/* Define STAT to generate a statistics log, stat.txt */

#ifdef STAT
#if !defined(DDS_THREADS_SINGLE)
#define DDS_THREADS_SINGLE
#endif
#endif


#define MAXNOOFTHREADS	16



#define MAXNOOFBOARDS		200/*100*/

/* Error codes */
#define RETURN_NO_FAULT		   	1
#define	RETURN_UNKNOWN_FAULT	  	-1
#define RETURN_ZERO_CARDS	  	-2
#define RETURN_TARGET_TOO_HIGH	  	-3
#define RETURN_DUPLICATE_CARDS	  	-4
#define RETURN_TARGET_WRONG_LO 	  	-5
#define RETURN_TARGET_WRONG_HI	  	-7
#define RETURN_SOLNS_WRONG_LO	  	-8
#define RETURN_SOLNS_WRONG_HI	  	-9
#define RETURN_TOO_MANY_CARDS	 	-10
#define RETURN_SUIT_OR_RANK	 	-12
#define RETURN_PLAYED_CARD	 	-13
#define RETURN_CARD_COUNT	 	-14
#define RETURN_THREAD_INDEX	 	-15
#define RETURN_PLAY_FAULT	 	-98
#define RETURN_PBN_FAULT	 	-99
#define RETURN_TOO_MANY_BOARDS		-101
#define RETURN_THREAD_CREATE		-102
#define RETURN_THREAD_WAIT		-103
#define RETURN_NO_SUIT			-201
#define RETURN_TOO_MANY_TABLES		-202
#define RETURN_CHUNK_SIZE		-301

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
  int suit;
  int rank;
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
  int ubound;
  int lbound;
  int bestMoveSuit;
  int bestMoveRank;
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

struct absRanksType {
  int 			aggrRanks[4];
  int 			winMask[4];
  struct absRankType 	absRank[15][4];
};


struct relRanksType {
  char relRank[8192][15];
};

struct adaptWinRanksType {
  unsigned short int winRanks[14];
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



struct ddTableDeal {
  unsigned int cards[4][4];
};

struct ddTableDeals {
  int noOfTables;
  struct ddTableDeal deals[MAXNOOFBOARDS>>2];
};

struct ddTableDealPBN {
  char cards[80];
};

struct ddTableDealsPBN {
  int noOfTables;
  struct ddTableDealPBN deals[MAXNOOFBOARDS>>2];
};

struct ddTableResults {
  int resTable[5][4];
};

struct ddTablesRes {
  int noOfBoards;
  struct ddTableResults results[MAXNOOFBOARDS>>2];
};

struct parResults {
  char parScore[2][16];	/* index = 0 is NS view and index = 1 is EW view. */
  char parContractsString[2][128]; /* index = 0 is NS view and index = 1 
				      is EW view. By “view” is here meant 
				      which side that starts the bidding. */
};


struct allParResults {
  struct parResults presults[MAXNOOFBOARDS / 20];
};


struct parResultsDealer {
  int  number;
  int  score;
  char contracts[10][10];
};


struct allparResultsDealer {
  struct parResultsDealer presults[MAXNOOFBOARDS / 20];
};

struct parContr2Type {
  char contracts[10];
  int denom;
};

struct playTraceBin {
  int	number;
  int	suit[52];
  int	rank[52];
};

struct playTracePBN {
  int	number;
  char	cards[106];
};

struct solvedPlay {
  int	number;
  int	tricks[53];
};

struct playTracesBin {
  int	noOfBoards;
  struct playTraceBin  plays[MAXNOOFBOARDS / 10];
};

struct playTracesPBN {
  int	noOfBoards;
  struct playTracePBN  plays[MAXNOOFBOARDS / 10];
};

struct solvedPlays {
  int	noOfBoards;
  struct solvedPlay  solved[MAXNOOFBOARDS / 10];
};



extern int noOfThreads;
extern int noOfCores;
extern struct localVarType * localVar;
extern int * highestRank;
extern int * counttable;
extern struct adaptWinRanksType * adaptWins;
extern unsigned short int bitMapRank[16];
extern unsigned short int relRankInSuit[4][4];
/*extern char relRank[8192][15];*/
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

extern FILE *fp, *fpx;
  /* Pointers to logs */

#ifdef STAT
extern FILE *fp2;
#endif

/* Externally accessible functions. */

EXTERN_C DLLEXPORT int STDCALL SolveBoard(struct deal dl, 
  int target, int solutions, int mode, struct futureTricks *futp, int threadIndex);
EXTERN_C DLLEXPORT int STDCALL SolveBoardPBN(struct dealPBN dlPBN, int target, 
    int solutions, int mode, struct futureTricks *futp, int threadIndex);

EXTERN_C DLLEXPORT int STDCALL CalcDDtable(struct ddTableDeal tableDeal, 
  struct ddTableResults * tablep);
EXTERN_C DLLEXPORT int STDCALL CalcDDtablePBN(struct ddTableDealPBN tableDealPBN, 
  struct ddTableResults * tablep);

EXTERN_C DLLEXPORT int STDCALL CalcAllTables(struct ddTableDeals *dealsp, int mode, 
	int trumpFilter[5], struct ddTablesRes *resp, struct allParResults *presp);
EXTERN_C DLLEXPORT int STDCALL CalcAllTablesPBN(struct ddTableDealsPBN *dealsp, int mode, 
	int trumpFilter[5], struct ddTablesRes *resp, struct allParResults *presp);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunksPBN(struct boardsPBN *bop, struct solvedBoards *solvedp, int chunkSize);
EXTERN_C DLLEXPORT int STDCALL SolveAllChunksBin(struct boards *bop, struct solvedBoards *solvedp, int chunkSize);

EXTERN_C DLLEXPORT int STDCALL Par(struct ddTableResults * tablep, struct parResults *presp,
	int vulnerable);
EXTERN_C DLLEXPORT int STDCALL DealerPar(struct ddTableResults * tablep, struct parResultsDealer * presp,
  int dealer, int vulnerable);
EXTERN_C DLLEXPORT int STDCALL SidesPar(struct ddTableResults * tablep, struct parResultsDealer sidesRes[2],
	int vulnerable);

EXTERN_C DLLEXPORT int STDCALL AnalysePlayBin(struct deal dl, struct playTraceBin play, struct solvedPlay* solvedp,
  int thrId);
EXTERN_C DLLEXPORT int STDCALL AnalysePlayPBN(struct dealPBN dlPBN, struct playTracePBN	playPBN, 
  struct solvedPlay * solvedp, int thrId);
EXTERN_C DLLEXPORT int STDCALL AnalyseAllPlaysBin(
  struct boards	* bop, struct playTracesBin * plp, struct solvedPlays * solvedp, int chunkSize);
EXTERN_C DLLEXPORT int STDCALL AnalyseAllPlaysPBN(struct boardsPBN * bopPBN, struct playTracesPBN * plpPBN,
  struct solvedPlays * solvedp,int chunkSize);

/* Remain externally accessible only for backwards compatibility. */

EXTERN_C DLLEXPORT int STDCALL SolveAllBoards(struct boardsPBN *bop, struct solvedBoards *solvedp);
EXTERN_C DLLEXPORT int STDCALL CalcPar(struct ddTableDeal tableDeal, int vulnerable,
    struct ddTableResults * tablep, struct parResults *presp);
EXTERN_C DLLEXPORT int STDCALL CalcParPBN(struct ddTableDealPBN tableDealPBN, 
  	struct ddTableResults * tablep, int vulnerable, struct parResults *presp);
EXTERN_C DLLEXPORT int STDCALL SolveAllChunks(struct boardsPBN *bop, struct solvedBoards *solvedp, int chunkSize);

/* End of externally accessible functions. */

 
int ABsearch(struct pos * posPoint, int target, int depth, struct localVarType * thrp);
void Make(struct pos * posPoint, unsigned short int trickCards[4], 
  int depth, int trump, struct movePlyType *mply, struct localVarType * thrp);
int MoveGen(struct pos * posPoint, int depth, int trump, struct movePlyType *mply, 
  struct localVarType * thrp);
void MergeSort(int n, struct moveType *a);
inline int WinningMove(struct moveType * mvp1, struct moveType * mvp2, int trump);
inline int WinningMoveNT(struct moveType * mvp1, struct moveType * mvp2);
int AdjustMoveList(struct localVarType * thrp);
int QuickTricks(struct pos * posPoint, int hand, 
	int depth, int target, int trump, int *result, struct localVarType * thrp);
int LaterTricksMIN(struct pos *posPoint, int hand, int depth, int target, int trump, 
  struct localVarType * thrp); 
int LaterTricksMAX(struct pos *posPoint, int hand, int depth, int target, int trump, 
  struct localVarType * thrp);
struct nodeCardsType * CheckSOP(struct pos * posPoint, struct nodeCardsType
  * nodep, int target, int tricks, int * result, int *value, struct localVarType * thrp);
struct nodeCardsType * UpdateSOP(struct pos * posPoint, struct nodeCardsType
  * nodep);  
struct nodeCardsType * FindSOP(struct pos * posPoint,
  struct winCardType * nodeP, int firstHand, 
	int target, int tricks, int * valp, struct localVarType * thrp);  
struct nodeCardsType * BuildPath(struct pos * posPoint, 
  struct posSearchType *nodep, int * result, struct localVarType * thrp);
void BuildSOP(struct pos * posPoint, long long suitLengths, int tricks, int firstHand, int target,
  int depth, int scoreFlag, int score, struct localVarType * thrp);
struct posSearchType * SearchLenAndInsert(struct posSearchType
	* rootp, long long key, int insertNode, int *result, struct localVarType * thrp);  
void Undo(struct pos * posPoint, int depth, struct movePlyType *mply, struct localVarType * thrp);
int CheckDeal(struct moveType * cardp, int thrId);
int InvBitMapRank(unsigned short bitMap);
int InvWinMask(int mask);
int NextMove(struct pos *posPoint, int depth, struct movePlyType *mply, 
  struct localVarType * thrp); 
int DumpInput(int errCode, struct deal dl, int target, int solutions, int mode); 
void Wipe(struct localVarType * thrp);
void AddNodeSet(struct localVarType * thrp);
void AddLenSet(struct localVarType * thrp);
void AddWinSet(struct localVarType * thrp);

void PrintDeal(FILE *fp, unsigned short ranks[4][4]);

int SolveAllBoardsN(struct boards *bop, struct solvedBoards *solvedp, int chunkSize);

int IsCard(char cardChar);
void UpdateDealInfo(int card);
int IsVal(char cardChar);
int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);


