/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef DDS_DLL_H
#define DDS_DLL_H

#if (defined(_WIN32) || defined(__CYGWIN__)) && ! defined(__clang__)
  #define DLLEXPORT __declspec(dllexport)
  #define STDCALL __stdcall
#else
  #define DLLEXPORT
  #define STDCALL
#endif

#ifdef __cplusplus
  #define EXTERN_C extern "C"
#else
  #define EXTERN_C
  #include <stdbool.h> // make "bool" available
#endif

/* Version 2.9.0. Allowing for 2 digit minor versions */
#define DDS_VERSION 20900


#define DDS_HANDS 4
#define DDS_SUITS 4
#define DDS_STRAINS 5


#define MAXNOOFBOARDS 200

#define MAXNOOFTABLES 40


// Error codes. See interface document for more detail.
// Call ErrorMessage(code, line[]) to get the text form in line[].

// Success.
#define RETURN_NO_FAULT 1
#define TEXT_NO_FAULT "Success"

// Currently happens when fopen() fails or when AnalyseAllPlaysBin()
// get a different number of boards in its first two arguments.
#define RETURN_UNKNOWN_FAULT -1
#define TEXT_UNKNOWN_FAULT "General error"

// SolveBoard()
#define RETURN_ZERO_CARDS -2
#define TEXT_ZERO_CARDS "Zero cards"

// SolveBoard()
#define RETURN_TARGET_TOO_HIGH -3
#define TEXT_TARGET_TOO_HIGH "Target exceeds number of tricks"

// SolveBoard()
#define RETURN_DUPLICATE_CARDS -4
#define TEXT_DUPLICATE_CARDS "Cards duplicated"

// SolveBoard()
#define RETURN_TARGET_WRONG_LO -5
#define TEXT_TARGET_WRONG_LO "Target is less than -1"

// SolveBoard()
#define RETURN_TARGET_WRONG_HI -7
#define TEXT_TARGET_WRONG_HI "Target is higher than 13"

// SolveBoard()
#define RETURN_SOLNS_WRONG_LO -8
#define TEXT_SOLNS_WRONG_LO "Solutions parameter is less than 1"

// SolveBoard()
#define RETURN_SOLNS_WRONG_HI -9
#define TEXT_SOLNS_WRONG_HI "Solutions parameter is higher than 3"

// SolveBoard(), self-explanatory.
#define RETURN_TOO_MANY_CARDS -10
#define TEXT_TOO_MANY_CARDS "Too many cards"

// SolveBoard()
#define RETURN_SUIT_OR_RANK -12
#define TEXT_SUIT_OR_RANK \
  "currentTrickSuit or currentTrickRank has wrong data"

// SolveBoard
#define RETURN_PLAYED_CARD -13
#define TEXT_PLAYED_CARD "Played card also remains in a hand"

// SolveBoard()
#define RETURN_CARD_COUNT -14
#define TEXT_CARD_COUNT "Wrong number of remaining cards in a hand"

// SolveBoard()
#define RETURN_THREAD_INDEX -15
#define TEXT_THREAD_INDEX "Thread index is not 0 .. maximum"

// SolveBoard()
#define RETURN_MODE_WRONG_LO -16
#define TEXT_MODE_WRONG_LO "Mode parameter is less than 0"

// SolveBoard()
#define RETURN_MODE_WRONG_HI -17
#define TEXT_MODE_WRONG_HI "Mode parameter is higher than 2"

// SolveBoard()
#define RETURN_TRUMP_WRONG -18
#define TEXT_TRUMP_WRONG "Trump is not in 0 .. 4"

// SolveBoard()
#define RETURN_FIRST_WRONG -19
#define TEXT_FIRST_WRONG "First is not in 0 .. 2"

// AnalysePlay*() family of functions.
// (a) Less than 0 or more than 52 cards supplied.
// (b) Invalid suit or rank supplied.
// (c) A played card is not held by the right player.
#define RETURN_PLAY_FAULT -98
#define TEXT_PLAY_FAULT "AnalysePlay input error"

// Returned from a number of places if a PBN string is faulty.
#define RETURN_PBN_FAULT -99
#define TEXT_PBN_FAULT "PBN string error"

// SolveBoard() and AnalysePlay*()
#define RETURN_TOO_MANY_BOARDS -101
#define TEXT_TOO_MANY_BOARDS "Too many boards requested"

// Returned from multi-threading functions.
#define RETURN_THREAD_CREATE -102
#define TEXT_THREAD_CREATE "Could not create threads"

// Returned from multi-threading functions when something went
// wrong while waiting for all threads to complete.
#define RETURN_THREAD_WAIT -103
#define TEXT_THREAD_WAIT "Something failed waiting for thread to end"

// Tried to set a multi-threading system that is not present in DLL.
#define RETURN_THREAD_MISSING -104
#define TEXT_THREAD_MISSING "Multi-threading system not present"

// CalcAllTables*()
#define RETURN_NO_SUIT -201
#define TEXT_NO_SUIT "Denomination filter vector has no entries"

// CalcAllTables*()
#define RETURN_TOO_MANY_TABLES -202
#define TEXT_TOO_MANY_TABLES "Too many DD tables requested"

// SolveAllChunks*()
#define RETURN_CHUNK_SIZE -301
#define TEXT_CHUNK_SIZE "Chunk size is less than 1"



struct futureTricks
{
  int nodes;
  int cards;
  int suit[13];
  int rank[13];
  int equals[13];
  int score[13];
};

struct deal
{
  int trump;
  int first;
  int currentTrickSuit[3];
  int currentTrickRank[3];
  unsigned int remainCards[DDS_HANDS][DDS_SUITS];
};


struct dealPBN
{
  int trump;
  int first;
  int currentTrickSuit[3];
  int currentTrickRank[3];
  char remainCards[80];
};


struct boards
{
  int noOfBoards;
  struct deal deals[MAXNOOFBOARDS];
  int target[MAXNOOFBOARDS];
  int solutions[MAXNOOFBOARDS];
  int mode[MAXNOOFBOARDS];
};

struct boardsPBN
{
  int noOfBoards;
  struct dealPBN deals[MAXNOOFBOARDS];
  int target[MAXNOOFBOARDS];
  int solutions[MAXNOOFBOARDS];
  int mode[MAXNOOFBOARDS];
};

struct solvedBoards
{
  int noOfBoards;
  struct futureTricks solvedBoard[MAXNOOFBOARDS];
};

struct ddTableDeal
{
  unsigned int cards[DDS_HANDS][DDS_SUITS];
};

struct ddTableDeals
{
  int noOfTables;
  struct ddTableDeal deals[MAXNOOFTABLES * DDS_STRAINS];
};

struct ddTableDealPBN
{
  char cards[80];
};

struct ddTableDealsPBN
{
  int noOfTables;
  struct ddTableDealPBN deals[MAXNOOFTABLES * DDS_STRAINS];
};

struct ddTableResults
{
  int resTable[DDS_STRAINS][DDS_HANDS];
};

struct ddTablesRes
{
  int noOfBoards;
  struct ddTableResults results[MAXNOOFTABLES * DDS_STRAINS];
};

struct parResults
{
  /* index = 0 is NS view and index = 1
     is EW view. By 'view' is here meant
     which side that starts the bidding. */
  char parScore[2][16];
  char parContractsString[2][128];
};


struct allParResults
{
  struct parResults presults[MAXNOOFTABLES];
};

struct parResultsDealer
{
  /* number: Number of contracts yielding the par score.
     score: Par score for the specified dealer hand.
     contracts:  Par contract text strings.  The first contract
       is in contracts[0], the last one in contracts[number-1].
       The detailed text format is is given in the DLL interface
       document.
  */
  int number;
  int score;
  char contracts[10][10];
};

struct contractType
{
  int underTricks; /* 0 = make 1-13 = sacrifice */
  int overTricks; /* 0-3, e.g. 1 for 4S + 1. */
  int level; /* 1-7 */
  int denom; /* 0 = No Trumps, 1 = trump Spades, 2 = trump Hearts,
				  3 = trump Diamonds, 4 = trump Clubs */
  int seats; /* One of the cases N, E, W, S, NS, EW;
				   0 = N 1 = E, 2 = S, 3 = W, 4 = NS, 5 = EW */
};

struct parResultsMaster
{
  int score; /* Sign according to the NS view */
  int number; /* Number of contracts giving the par score */
  struct contractType contracts[10]; /* Par contracts */
};

struct parTextResults
{
  char parText[2][128]; /* Short text for par information, e.g.
				Par -110: EW 2S EW 2D+1 */
  bool equal; /* true in the normal case when it does not matter who
			starts the bidding. Otherwise, false. */
};


struct playTraceBin
{
  int number;
  int suit[52];
  int rank[52];
};

struct playTracePBN
{
  int number;
  char cards[106];
};

struct solvedPlay
{
  int number;
  int tricks[53];
};

struct playTracesBin
{
  int noOfBoards;
  struct playTraceBin plays[MAXNOOFBOARDS];
};

struct playTracesPBN
{
  int noOfBoards;
  struct playTracePBN plays[MAXNOOFBOARDS];
};

struct solvedPlays
{
  int noOfBoards;
  struct solvedPlay solved[MAXNOOFBOARDS];
};

struct DDSInfo
{
  // Version 2.8.0 has 2, 8, 0 and a string of 2.8.0
  int major, minor, patch; 
  char versionString[10];

  // Currently 0 = unknown, 1 = Windows, 2 = Cygwin, 3 = Linux, 4 = Apple
  int system;

  // We know 32 and 64-bit systems.
  int numBits;

  // Currently 0 = unknown, 1 = Microsoft Visual C++, 2 = mingw,
  // 3 = GNU g++, 4 = clang
  int compiler;

  // Currently 0 = none, 1 = DllMain, 2 = Unix-style
  int constructor;

  int numCores;

  // Currently 
  // 0 = none, 
  // 1 = Windows (native), 
  // 2 = OpenMP, 
  // 3 = GCD,
  // 4 = Boost,
  // 5 = STL,
  // 6 = TBB,
  // 7 = STLIMPL (for_each), experimental only
  // 8 = PPLIMPL (for_each), experimental only
  int threading;

  // The actual number of threads configured
  int noOfThreads;

  // This will break if there are > 128 threads...
  // The string is of the form LLLSSS meaning 3 large TT memories
  // and 3 small ones.
  char threadSizes[128];

  char systemString[1024];
};



EXTERN_C DLLEXPORT void STDCALL SetMaxThreads(
  int userThreads);

EXTERN_C DLLEXPORT int STDCALL SetThreading(
  int code);

EXTERN_C DLLEXPORT void STDCALL SetResources(
  int maxMemoryMB,
  int maxThreads);

EXTERN_C DLLEXPORT void STDCALL FreeMemory();

EXTERN_C DLLEXPORT int STDCALL SolveBoard(
  struct deal dl,
  int target,
  int solutions,
  int mode,
  struct futureTricks * futp,
  int threadIndex);

EXTERN_C DLLEXPORT int STDCALL SolveBoardPBN(
  struct dealPBN dlpbn,
  int target,
  int solutions,
  int mode,
  struct futureTricks * futp,
  int thrId);

EXTERN_C DLLEXPORT int STDCALL CalcDDtable(
  struct ddTableDeal tableDeal,
  struct ddTableResults * tablep);

EXTERN_C DLLEXPORT int STDCALL CalcDDtablePBN(
  struct ddTableDealPBN tableDealPBN,
  struct ddTableResults * tablep);

EXTERN_C DLLEXPORT int STDCALL CalcAllTables(
  struct ddTableDeals * dealsp,
  int mode,
  int trumpFilter[DDS_STRAINS],
  struct ddTablesRes * resp,
  struct allParResults * presp);

EXTERN_C DLLEXPORT int STDCALL CalcAllTablesPBN(
  struct ddTableDealsPBN * dealsp,
  int mode,
  int trumpFilter[DDS_STRAINS],
  struct ddTablesRes * resp,
  struct allParResults * presp);

EXTERN_C DLLEXPORT int STDCALL SolveAllBoards(
  struct boardsPBN * bop,
  struct solvedBoards * solvedp);

EXTERN_C DLLEXPORT int STDCALL SolveAllBoardsBin(
  struct boards * bop,
  struct solvedBoards * solvedp);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunks(
  struct boardsPBN * bop,
  struct solvedBoards * solvedp,
  int chunkSize);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunksBin(
  struct boards * bop,
  struct solvedBoards * solvedp,
  int chunkSize);

EXTERN_C DLLEXPORT int STDCALL SolveAllChunksPBN(
  struct boardsPBN * bop,
  struct solvedBoards * solvedp,
  int chunkSize);

EXTERN_C DLLEXPORT int STDCALL Par(
  struct ddTableResults * tablep,
  struct parResults * presp,
  int vulnerable);

EXTERN_C DLLEXPORT int STDCALL CalcPar(
  struct ddTableDeal tableDeal,
  int vulnerable,
  struct ddTableResults * tablep,
  struct parResults * presp);

EXTERN_C DLLEXPORT int STDCALL CalcParPBN(
  struct ddTableDealPBN tableDealPBN,
  struct ddTableResults * tablep,
  int vulnerable,
  struct parResults * presp);

EXTERN_C DLLEXPORT int STDCALL SidesPar(
  struct ddTableResults * tablep,
  struct parResultsDealer sidesRes[2],
  int vulnerable);

EXTERN_C DLLEXPORT int STDCALL DealerPar(
  struct ddTableResults * tablep,
  struct parResultsDealer * presp,
  int dealer,
  int vulnerable);

EXTERN_C DLLEXPORT int STDCALL DealerParBin(
  struct ddTableResults * tablep,
  struct parResultsMaster * presp,
  int dealer, 
  int vulnerable);

EXTERN_C DLLEXPORT int STDCALL SidesParBin(
  struct ddTableResults * tablep,
  struct parResultsMaster sidesRes[2],
  int vulnerable);

EXTERN_C DLLEXPORT int STDCALL ConvertToDealerTextFormat(
  struct parResultsMaster * pres,
  char * resp);

EXTERN_C DLLEXPORT int STDCALL ConvertToSidesTextFormat(
  struct parResultsMaster * pres,
  struct parTextResults * resp);

EXTERN_C DLLEXPORT int STDCALL AnalysePlayBin(
  struct deal dl,
  struct playTraceBin play,
  struct solvedPlay * solved,
  int thrId);

EXTERN_C DLLEXPORT int STDCALL AnalysePlayPBN(
  struct dealPBN dlPBN,
  struct playTracePBN playPBN,
  struct solvedPlay * solvedp,
  int thrId);

EXTERN_C DLLEXPORT int STDCALL AnalyseAllPlaysBin(
  struct boards * bop,
  struct playTracesBin * plp,
  struct solvedPlays * solvedp,
  int chunkSize);

EXTERN_C DLLEXPORT int STDCALL AnalyseAllPlaysPBN(
  struct boardsPBN * bopPBN,
  struct playTracesPBN * plpPBN,
  struct solvedPlays * solvedp,
  int chunkSize);

EXTERN_C DLLEXPORT void STDCALL GetDDSInfo(
  struct DDSInfo * info);

EXTERN_C DLLEXPORT void STDCALL ErrorMessage(
  int code,
  char line[80]);

#endif
