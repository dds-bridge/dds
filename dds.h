
#if defined(_WIN32)
  #if defined(_MSC_VER)
  #include <intrin.h>
  #endif
  #define USES_DLLMAIN
      /* DLL uses DllMain() for initialization */

#elif defined (__CYGWIN__)
  #include <windows.h>
  #include <process.h>
  #define USES_DLLMAIN
  

#elif defined (__linux)
  #include <unistd.h>
  #define USES_CONSTRUCTOR
      /* DLL uses a constructor function for initialization */
  typedef long long __int64;

#elif defined (__APPLE__)
  #include <unistd.h>
  #define USES_CONSTRUCTOR
  typedef long long __int64;

#endif

#if defined(INFINITY)
#    undef INFINITY
#endif
#define INFINITY    32000

#define TT_MAXMEM  150000000

#define MAXNODE     1
#define MINNODE     0

#ifndef TRUE
#define TRUE        1
#define FALSE       0
#endif

#define MOVESVALID  1
#define MOVESLOCKED 2

#define NSIZE	100000
#define WSIZE   100000
#define LSIZE   20000
#define NINIT	250000
#define WINIT	700000
#define LINIT   50000

#define SIMILARDEALLIMIT	5
#define SIMILARMAXWINNODES      700000


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


struct localVarType {
  int nodeTypeStore[4];
  int trump;
  unsigned short int lowestWin[50][4];
  #ifdef STAT
  int nodes;
  #endif
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
  struct absRanksType 	* abs;
  struct adaptWinRanksType * adaptWins;
  struct posSearchType *rootnp[14][4];
  struct winCardType **pw;
  struct nodeCardsType **pn;
  struct posSearchType **pl;
  struct nodeCardsType * nodeCards;
  struct winCardType * winCards;
  struct posSearchType * posSearch;
  unsigned short int iniRemovedRanks[4];

  int nodeSetSize; /* Index with range 0 to nodeSetSizeLimit */
  int winSetSize;  /* Index with range 0 to winSetSizeLimit */
  int lenSetSize;  /* Index with range 0 to lenSetSizeLimit */
};

struct paramType {
  int noOfBoards;
  struct boards *bop;
  struct solvedBoards *solvedp;
  int error;
};

struct playparamType {
  int			noOfBoards;
  struct boards		* bop;
  struct playTracesBin	* plp;
  struct solvedPlays	* solvedp;
  int			error;
};






