#ifndef DDS_DLLH
#define DDS_DLLH

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


/* Version 2.7.0. Allowing for 2 digit minor versions */
#define DDS_VERSION	       20700	


#define DDS_HANDS	  	   4
#define DDS_SUITS		   4
#define DDS_STRAINS		   5


#define MAXNOOFBOARDS		 200


/* Error codes */
#define RETURN_NO_FAULT		   1
#define	RETURN_UNKNOWN_FAULT	  -1
#define RETURN_ZERO_CARDS	  -2
#define RETURN_TARGET_TOO_HIGH	  -3
#define RETURN_DUPLICATE_CARDS	  -4
#define RETURN_TARGET_WRONG_LO 	  -5
#define RETURN_TARGET_WRONG_HI	  -7
#define RETURN_SOLNS_WRONG_LO	  -8
#define RETURN_SOLNS_WRONG_HI	  -9
#define RETURN_TOO_MANY_CARDS	 -10
#define RETURN_SUIT_OR_RANK	 -12
#define RETURN_PLAYED_CARD	 -13
#define RETURN_CARD_COUNT	 -14
#define RETURN_THREAD_INDEX	 -15
#define RETURN_PLAY_FAULT	 -98
#define RETURN_PBN_FAULT	 -99
#define RETURN_TOO_MANY_BOARDS	-101
#define RETURN_THREAD_CREATE	-102
#define RETURN_THREAD_WAIT	-103
#define RETURN_NO_SUIT		-201
#define RETURN_CHUNK_SIZE	-201
#define RETURN_TOO_MANY_TABLES	-202


struct futureTricks {
  int 			nodes;
  int 			cards;
  int 			suit[13];
  int 			rank[13];
  int 			equals[13];
  int 			score[13];
};

struct deal {
  int 			trump;
  int 			first;
  int 			currentTrickSuit[3];
  int 			currentTrickRank[3];
  unsigned int  	remainCards[DDS_HANDS][DDS_SUITS];
};


struct dealPBN {
  int 			trump;
  int 			first;
  int 			currentTrickSuit[3];
  int 			currentTrickRank[3];
  char 			remainCards[80];
};


struct boards {
  int 			noOfBoards;
  struct deal 		deals[MAXNOOFBOARDS];
  int 			target[MAXNOOFBOARDS];
  int 			solutions[MAXNOOFBOARDS];
  int 			mode[MAXNOOFBOARDS];
};

struct boardsPBN {
  int 			noOfBoards;
  struct dealPBN 	deals[MAXNOOFBOARDS];
  int 			target[MAXNOOFBOARDS];
  int 			solutions[MAXNOOFBOARDS];
  int 			mode[MAXNOOFBOARDS];
};

struct solvedBoards {
  int 			noOfBoards;
  struct futureTricks 	solvedBoard[MAXNOOFBOARDS];
};

struct ddTableDeal {
  unsigned int 		cards[DDS_HANDS][DDS_SUITS];
};

struct ddTableDeals {
  int 			noOfTables;
  struct ddTableDeal 	deals[MAXNOOFBOARDS >> 2];
};

struct ddTableDealPBN {
  char 			cards[80];
};

struct ddTableDealsPBN {
  int 			noOfTables;
  struct ddTableDealPBN deals[MAXNOOFBOARDS>>2];
};

struct ddTableResults {
  int 			resTable[DDS_STRAINS][DDS_HANDS];
};

struct ddTablesRes {
  int 		noOfBoards;
  struct ddTableResults results[MAXNOOFBOARDS>>2];
};

struct parResults {
  /* index = 0 is NS view and index = 1 
     is EW view. By 'view' is here meant 
     which side that starts the bidding. */
  char 			parScore[2][16];
  char 			parContractsString[2][128]; 
};


struct allParResults {
  struct parResults 	presults[MAXNOOFBOARDS / 20];
};

struct parResultsDealer {
  int			number;
  int			score;
  char			contracts[10][10];
};


struct contractType {
	int underTricks;  /* 0 = make  1-13 = sacrifice */
	int overTricks;  /* 0-3,  e.g. 1 for 4S + 1. */
	int level;  /* 1-7 */
	int denom;   /* 0 = No Trumps, 1 = trump Spades, 2 = trump Hearts, 
				  3 = trump Diamonds, 4 = trump Clubs  */
	int seats;  /* One of the cases N, E, W, S, NS, EW;
				   0 = N  1 = E, 2 = S, 3 = W, 4 = NS, 5 = EW */
};

struct parResultsMaster {
	int score;  /* Sign according to the NS view */
	int number;   /* Number of contracts giving the par score */
	struct contractType contracts[10];  /* Par contracts */
};

struct parTextResults {
	char parText[2][128];  /* Short text for par information, e.g.
				Par -110: EW 2S  EW 2D+1 */
	int equal;  /* TRUE in the normal case when it does not matter who
			starts the bidding. Otherwise, FALSE. */
};


struct playTraceBin {
  int			number;
  int			suit[52];
  int			rank[52];
};

struct playTracePBN {
  int			number;
  char			cards[106];
};

struct solvedPlay {
  int			number;
  int			tricks[53];
};

struct playTracesBin {
  int			noOfBoards;
  struct playTraceBin	plays[MAXNOOFBOARDS / 10];
};

struct playTracesPBN {
  int			noOfBoards;
  struct playTracePBN	plays[MAXNOOFBOARDS / 10];
};

struct solvedPlays {
  int			noOfBoards;
  struct solvedPlay	solved[MAXNOOFBOARDS / 10];
};



EXTERN_C DLLEXPORT void STDCALL SetMaxThreads(
  int 			userThreads);

EXTERN_C DLLEXPORT void STDCALL FreeMemory();

EXTERN_C DLLEXPORT int STDCALL SolveBoard(
  struct deal 		dl, 
  int 			target, 
  int 			solutions, 
  int 			mode, 
  struct futureTricks	* futp, 
  int 			threadIndex);

EXTERN_C DLLEXPORT int STDCALL SolveBoardPBN(
  struct dealPBN 	dlpbn, 
  int 			target, 
  int 			solutions, 
  int 			mode, 
  struct futureTricks 	* futp, 
  int 			thrId);

EXTERN_C DLLEXPORT int STDCALL CalcDDtable(
  struct ddTableDeal 	tableDeal, 
  struct ddTableResults * tablep);

EXTERN_C DLLEXPORT int STDCALL CalcDDtablePBN(
  struct ddTableDealPBN tableDealPBN, 
  struct ddTableResults * tablep);

EXTERN_C DLLEXPORT int STDCALL CalcAllTables(
  struct ddTableDeals 	* dealsp, 
  int 			mode, 
  int 			trumpFilter[DDS_STRAINS], 
  struct ddTablesRes 	* resp, 
  struct allParResults 	* presp);

EXTERN_C DLLEXPORT int STDCALL CalcAllTablesPBN(
  struct ddTableDealsPBN * dealsp, 
  int 			mode, 
  int 			trumpFilter[DDS_STRAINS], 
  struct ddTablesRes 	* resp, 
  struct allParResults 	* presp);

EXTERN_C DLLEXPORT int STDCALL SolveAllBoards(
  struct boardsPBN 	* bop, 
  struct solvedBoards 	* solvedp);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunks(
  struct boardsPBN 	* bop, 
  struct solvedBoards 	* solvedp, 
  int 			chunkSize);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunksBin(
  struct boards 	* bop, 
  struct solvedBoards 	* solvedp, 
  int 			chunkSize);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunksPBN(
  struct boardsPBN 	* bop, 
  struct solvedBoards 	* solvedp, 
  int 			chunkSize);

EXTERN_C DLLEXPORT int STDCALL Par(
  struct ddTableResults	* tablep,
  struct parResults	* presp,
  int			vulnerable);

EXTERN_C DLLEXPORT int STDCALL CalcPar(
  struct ddTableDeal 	tableDeal, 
  int 			vulnerable, 
  struct ddTableResults	* tablep, 
  struct parResults 	* presp);

EXTERN_C DLLEXPORT int STDCALL CalcParPBN(
  struct ddTableDealPBN	tableDealPBN, 
  struct ddTableResults	* tablep, 
  int 			vulnerable, 
  struct parResults	* presp);

EXTERN_C DLLEXPORT int STDCALL SidesPar(
  struct ddTableResults * tablep,
  struct parResultsDealer sidesRes[2],
  int			vulnerable);

EXTERN_C DLLEXPORT int STDCALL DealerPar(
  struct ddTableResults	* tablep,
  struct parResultsDealer * presp,
  int			dealer,
  int			vulnerable);

EXTERN_C DLLEXPORT int STDCALL DealerParBin(
  struct ddTableResults * tablep, 
  struct parResultsMaster * presp,
  int dealer, int vulnerable);

EXTERN_C DLLEXPORT int STDCALL SidesParBin(
  struct ddTableResults * tablep, 
  struct parResultsMaster sidesRes[2],
  int vulnerable);

EXTERN_C DLLEXPORT int STDCALL ConvertToDealerTextFormat(
  struct parResultsMaster *pres, 
  char *resp);

EXTERN_C DLLEXPORT int STDCALL ConvertToSidesTextFormat(
  struct parResultsMaster *pres, 
  struct parTextResults *resp);

EXTERN_C DLLEXPORT int STDCALL AnalysePlayBin(
  struct deal		dl,
  struct playTraceBin	play,
  struct solvedPlay	* solved,
  int			thrId);

EXTERN_C DLLEXPORT int STDCALL AnalysePlayPBN(
  struct dealPBN	dlPBN,
  struct playTracePBN	playPBN,
  struct solvedPlay	* solvedp,
  int			thrId);

EXTERN_C DLLEXPORT int STDCALL AnalyseAllPlaysBin(
  struct boards		* bop,
  struct playTracesBin	* plp,
  struct solvedPlays	* solvedp,
  int			chunkSize);

EXTERN_C DLLEXPORT int STDCALL AnalyseAllPlaysPBN(
  struct boardsPBN	* bopPBN,
  struct playTracesPBN	* plpPBN,
  struct solvedPlays	* solvedp,
  int			chunkSize);

#endif
