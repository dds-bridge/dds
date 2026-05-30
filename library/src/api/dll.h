/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#pragma once

#include <utility/constants.h>

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

/* Version 3.0.0. Allowing for 2 digit minor versions */
#define DDS_VERSION 30000

#define MAXNOOFBOARDS 200

#define MAXNOOFTABLES 40


// Error codes. See interface document for more detail.
// Call ErrorMessage(code, line[]) to get the text form in line[].

// Success.
#define RETURN_NO_FAULT 1
#define TEXT_NO_FAULT "Success"

// Currently happens when fopen() fails or when AnalyseAllPlaysBin()
// get a different number of Boards in its first two arguments.
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
#define TEXT_TOO_MANY_BOARDS "Too many Boards requested"

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



/**
 * @brief Stores the result of a double dummy analysis for a single position.
 *
 * Contains the number of nodes searched, the number of cards in the result,
 * and arrays for each card's suit, rank, equality group, and score.
 */
struct FutureTricks
{
  int nodes;
  int cards;
  int suit[13];
  int rank[13];
  int equals[13];
  int score[13];
};

/**
 * @brief Represents a bridge Deal for double dummy analysis.
 *
 * @param trump The trump suit (0 = NT, 1 = Spades, ...)
 * @param first The hand to play first (0 = N, 1 = E, ...)
 * @param currentTrickSuit Suits of cards played in the current trick
 * @param currentTrickRank Ranks of cards played in the current trick
 * @param remainCards Remaining cards in each hand and suit
 */
struct Deal
{
  int trump;
  int first;
  int currentTrickSuit[3];
  int currentTrickRank[3];
  unsigned int remainCards[DDS_HANDS][DDS_SUITS];
};


/**
 * @brief Represents a bridge Deal in PBN (Portable Bridge Notation) format.
 *
 * @param trump The trump suit
 * @param first The hand to play first
 * @param currentTrickSuit Suits of cards played in the current trick
 * @param currentTrickRank Ranks of cards played in the current trick
 * @param remainCards PBN string describing remaining cards
 */
struct DealPBN
{
  int trump;
  int first;
  int currentTrickSuit[3];
  int currentTrickRank[3];
  char remainCards[80];
};


/**
 * @brief Represents multiple bridge deals for batch analysis.
 *
 * @param noOfBoards Number of deals
 * @param deals Array of deals
 * @param target Array of targets for each Deal
 * @param solutions Array of solution modes for each Deal
 * @param mode Array of modes for each Deal
 */
struct Boards
{
  int no_of_boards;
  struct Deal deals[MAXNOOFBOARDS];
  int target[MAXNOOFBOARDS];
  int solutions[MAXNOOFBOARDS];
  int mode[MAXNOOFBOARDS];
};

/**
 * @brief Multiple boards in PBN format for batch solving.
 *
 * Similar to Boards but uses PBN (Portable Bridge Notation) format
 * for deal representation. Used for solving multiple boards efficiently.
 *
 * @see Boards
 */
struct BoardsPBN
{
  int no_of_boards;                            ///< Number of boards to solve
  struct DealPBN deals[MAXNOOFBOARDS];       ///< Array of deals in PBN format
  int target[MAXNOOFBOARDS];                 ///< Target tricks for each board
  int solutions[MAXNOOFBOARDS];              ///< Solution mode for each board
  int mode[MAXNOOFBOARDS];                   ///< Solve mode for each board
};

/**
 * @brief Solutions for multiple boards.
 *
 * Container for results from batch board solving operations.
 * Each entry contains the complete future tricks analysis for one board.
 *
 * @see FutureTricks
 */
struct SolvedBoards
{
  int no_of_boards;                                    ///< Number of solved boards
  struct FutureTricks solved_board[MAXNOOFBOARDS];    ///< Array of solutions
};

struct DdTableDeal
{
  unsigned int cards[DDS_HANDS][DDS_SUITS];
};

struct DdTableDeals
{
  int no_of_tables;
  struct DdTableDeal deals[MAXNOOFTABLES * DDS_STRAINS];
};

struct DdTableDealPBN
{
  char cards[80];
};

struct DdTableDealsPBN
{
  int no_of_tables;
  struct DdTableDealPBN deals[MAXNOOFTABLES * DDS_STRAINS];
};

struct DdTableResults
{
  int res_table[DDS_STRAINS][DDS_HANDS];
};

struct DdTablesRes
{
  int no_of_boards;
  struct DdTableResults results[MAXNOOFTABLES * DDS_STRAINS];
};

struct ParResults
{
  /* index = 0 is NS view and index = 1
     is EW view. By 'view' is here meant
     which side that starts the bidding. */
  char par_score[2][16];
  char par_contracts_string[2][128];
};

struct AllParResults
{
  struct ParResults par_results[MAXNOOFTABLES];
};

struct ParResultsDealer
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

struct ContractType
{
  int under_tricks; /* 0 = make 1-13 = sacrifice */
  int over_tricks; /* 0-3, e.g. 1 for 4S + 1. */
  int level; /* 1-7 */
  int denom; /* 0 = No Trumps, 1 = trump Spades, 2 = trump Hearts,
                  3 = trump Diamonds, 4 = trump Clubs */
  int seats; /* One of the cases N, E, W, S, NS, EW;
                   0 = N 1 = E, 2 = S, 3 = W, 4 = NS, 5 = EW */
};

struct ParResultsMaster
{
  int score; /* Sign according to the NS view */
  int number; /* Number of contracts giving the par score */
  struct ContractType contracts[10]; /* Par contracts */
};

struct ParTextResults
{
  char par_text[2][128]; /* Short text for par information, e.g.
            Par -110: EW 2S EW 2D+1 */
  bool equal; /* true in the normal case when it does not matter who
            starts the bidding. Otherwise, false. */
};


struct PlayTraceBin
{
  int number;
  int suit[52];
  int rank[52];
};

struct PlayTracePBN
{
  int number;
  char cards[106];
};

struct SolvedPlay
{
  int number;
  int tricks[53];
};

struct PlayTracesBin
{
  int no_of_boards;
  struct PlayTraceBin plays[MAXNOOFBOARDS];
};

struct PlayTracesPBN
{
  int no_of_boards;
  struct PlayTracePBN plays[MAXNOOFBOARDS];
};

struct SolvedPlays
{
  int no_of_boards;
  struct SolvedPlay solved[MAXNOOFBOARDS];
};

struct DDSInfo
{
  // Version 2.8.0 has 2, 8, 0 and a string of 2.8.0
  int major, minor, patch; 
  char version_string[10];

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



/**
 * @brief Set the maximum number of threads used by the solver.
 *
 * @deprecated In the modern C++ API, thread count is controlled by the
 *             embedding application (typically one SolverContext per worker
 *             thread). New code should create/destroy SolverContext instances
 *             in the application rather than calling this function.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param userThreads Maximum number of threads to use
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. It has no direct equivalent in the modern API, where both
 * threading and TT memory limits are configured via SolverContext and
 * SolverConfig on a per-instance basis.
 */
EXTERN_C DLLEXPORT auto STDCALL SetMaxThreads(
  int userThreads) -> void;

/**
 * @brief Set the threading backend used by the solver.
 *
 * @deprecated Use SolverContext instead - threading is implicit (one context per thread).
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param code Threading backend code (see documentation)
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. The modern C++ API does not require threading configuration;
 * instead, create one SolverContext instance per thread.
 */
EXTERN_C DLLEXPORT auto STDCALL SetThreading(
  int code) -> int;

/**
 * @brief Set memory and thread resources for the solver.
 *
 * @deprecated Use SolverContext with SolverConfig instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param maxMemoryMB Maximum memory in megabytes
 * @param maxThreads Maximum number of threads
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which provides per-instance configuration through SolverConfig.
 */
EXTERN_C DLLEXPORT auto STDCALL SetResources(
  int maxMemoryMB,
  int maxThreads) -> void;

/**
 * @brief Free memory used by the solver.
 *
 * @deprecated Use SolverContext RAII instead - cleanup is automatic.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. The modern C++ API uses RAII (Resource Acquisition Is
 * Initialization) through SolverContext, which automatically cleans up
 * resources when the context goes out of scope. No explicit cleanup needed.
 */
EXTERN_C DLLEXPORT auto STDCALL FreeMemory() -> void;

/**
 * @brief Solve a single bridge Deal using double dummy analysis.
 *
 * @param dl The Deal to analyze
 * @param target Target number of tricks
 * @param solutions Solution mode (1 = best, 2 = all, etc.)
 * @param mode Analysis mode
 * @param futp Pointer to result structure
 * @param threadIndex Index of thread to use
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL SolveBoard(
  struct Deal dl,
  int target,
  int solutions,
  int mode,
  struct FutureTricks * futp,
  int threadIndex) -> int;

/**
 * @brief Solve a single bridge Deal in PBN format using double dummy analysis.
 *
 * @param dlpbn The PBN Deal to analyze
 * @param target Target number of tricks
 * @param solutions Solution mode
 * @param mode Analysis mode
 * @param futp Pointer to result structure
 * @param thrId Index of thread to use
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL SolveBoardPBN(
  struct DealPBN dlpbn,
  int target,
  int solutions,
  int mode,
  struct FutureTricks * futp,
  int thrId) -> int;

/**
 * @brief Calculate the double dummy table for a given Deal.
 *
 * @param tableDeal Deal for which to calculate the table
 * @param tablep Pointer to result table
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL CalcDDtable(
  struct DdTableDeal tableDeal,
  struct DdTableResults * tablep) -> int;

/**
 * @brief Calculate the double dummy table for a PBN Deal.
 *
 * @param tableDealPBN PBN Deal for which to calculate the table
 * @param tablep Pointer to result table
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL CalcDDtablePBN(
  struct DdTableDealPBN tableDealPBN,
  struct DdTableResults * tablep) -> int;

/**
 * @brief Calculate double dummy tables for multiple deals.
 *
 * @param dealsp Pointer to multiple deals
 * @param mode Analysis mode
 * @param trumpFilter Array of trump suit filters
 * @param resp Pointer to result tables
 * @param presp Pointer to par results
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTables(
  struct DdTableDeals const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTablesRes * resp,
  struct AllParResults * presp) -> int;

/**
 * @brief Calculate double dummy tables for multiple PBN deals.
 *
 * @param dealsp Pointer to multiple PBN deals
 * @param mode Analysis mode
 * @param trumpFilter Array of trump suit filters
 * @param resp Pointer to result tables
 * @param presp Pointer to par results
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTablesPBN(
  struct DdTableDealsPBN const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTablesRes * resp,
  struct AllParResults * presp) -> int;

/**
 * @brief Solve multiple bridge deals in PBN format.
 *
 * @param bop Pointer to multiple PBN deals
 * @param solvedp Pointer to results for solved Boards
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL SolveAllBoards(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsBin(
  struct Boards const * bop,
  struct SolvedBoards * solvedp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsSeq(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsBinSeq(
  struct Boards const * bop,
  struct SolvedBoards * solvedp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllChunks(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllChunksBin(
  struct Boards const * bop,
  struct SolvedBoards * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllChunksPBN(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL Par(
  struct DdTableResults const * tablep,
  struct ParResults * presp,
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL CalcPar(
  struct DdTableDeal tableDeal,
  int vulnerable,
  struct DdTableResults * tablep,
  struct ParResults * presp) -> int;

EXTERN_C DLLEXPORT auto STDCALL CalcParPBN(
  struct DdTableDealPBN tableDealPBN,
  struct DdTableResults * tablep,
  int vulnerable,
  struct ParResults * presp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SidesPar(
  struct DdTableResults const * tablep,
  struct ParResultsDealer sidesRes[2],
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL DealerPar(
  struct DdTableResults const * tablep,
  struct ParResultsDealer * presp,
  int dealer,
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL DealerParBin(
  struct DdTableResults const * tablep,
  struct ParResultsMaster * presp,
  int dealer, 
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL SidesParBin(
  struct DdTableResults const * tablep,
  struct ParResultsMaster sidesRes[2],
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL ConvertToDealerTextFormat(
  struct ParResultsMaster const * pres,
  char * resp) -> int;

EXTERN_C DLLEXPORT auto STDCALL ConvertToSidesTextFormat(
  struct ParResultsMaster const * pres,
  struct ParTextResults * resp) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalysePlayBin(
  struct Deal dl,
  struct PlayTraceBin play,
  struct SolvedPlay * solved,
  int thrId) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalysePlayPBN(
  struct DealPBN dlPBN,
  struct PlayTracePBN playPBN,
  struct SolvedPlay * solvedp,
  int thrId) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalyseAllPlaysBin(
  struct Boards const * bop,
  struct PlayTracesBin const * plp,
  struct SolvedPlays * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalyseAllPlaysPBN(
  struct BoardsPBN const * bopPBN,
  struct PlayTracesPBN const * plpPBN,
  struct SolvedPlays * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL GetDDSInfo(
  struct DDSInfo * info) -> void;

EXTERN_C DLLEXPORT auto STDCALL ErrorMessage(
  int code,
  char line[80]) -> void;
