/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// @file dds_constants.hpp
/// @brief Compile-time constants and macros shared across the whole solver.
///
/// This header carries the values that other headers use to size arrays and
/// loop bounds: the fixed bridge dimensions, the frozen legacy-API version and
/// capacity limits, the solver status codes, and the export/calling-convention
/// macros. It defines no types and pulls in no other project header, so both
/// internal solver code and external API headers can include it without
/// dragging in the public function-declaration surface (`api/dll.h`).

// ---------------------------------------------------------------------------
// Export / calling-convention macros
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Fixed bridge dimensions
// ---------------------------------------------------------------------------

/// @defgroup dds_dimensions Bridge dimensions
/// @{
/// These are `constexpr` and are treated as immutable across the codebase —
/// array sizes and loop bounds everywhere assume them.
constexpr int DDS_STRAINS = 5;  ///< Number of strains (4 suits + no trump)
constexpr int DDS_HANDS = 4;    ///< Number of hands (N/E/S/W)
constexpr int DDS_SUITS = 4;    ///< Number of suits (S/H/D/C)
constexpr int DDS_NOTRUMP = 4;  ///< No trump strain index
/// @}

// ---------------------------------------------------------------------------
// Legacy C API version and capacity limits
// ---------------------------------------------------------------------------

/* Version 3.1.0. Allowing for 2 digit minor versions */
// These three stay object-like macros: this is the frozen legacy C API surface
// and external consumers conventionally test the version / limits in the
// preprocessor (e.g. #if DDS_VERSION >= 30100, #ifdef MAXNOOFBOARDS).
#define DDS_VERSION 30100

#define MAXNOOFBOARDS 200

#define MAXNOOFTABLES 40

// ---------------------------------------------------------------------------
// Solver status codes
// ---------------------------------------------------------------------------

// Error codes. See interface document for more detail.
// Call ErrorMessage(code, line[]) to get the text form in line[].

// Success.
constexpr int RETURN_NO_FAULT = 1;
constexpr const char TEXT_NO_FAULT[] = "Success";

// Currently happens when fopen() fails or when AnalyseAllPlaysBin()
// get a different number of Boards in its first two arguments.
constexpr int RETURN_UNKNOWN_FAULT = -1;
constexpr const char TEXT_UNKNOWN_FAULT[] = "General error";

// SolveBoard()
constexpr int RETURN_ZERO_CARDS = -2;
constexpr const char TEXT_ZERO_CARDS[] = "Zero cards";

// SolveBoard()
constexpr int RETURN_TARGET_TOO_HIGH = -3;
constexpr const char TEXT_TARGET_TOO_HIGH[] =
  "Target exceeds number of tricks";

// SolveBoard()
constexpr int RETURN_DUPLICATE_CARDS = -4;
constexpr const char TEXT_DUPLICATE_CARDS[] = "Cards duplicated";

// SolveBoard()
constexpr int RETURN_TARGET_WRONG_LO = -5;
constexpr const char TEXT_TARGET_WRONG_LO[] =
  "Target is less than -1";

// SolveBoard()
constexpr int RETURN_TARGET_WRONG_HI = -7;
constexpr const char TEXT_TARGET_WRONG_HI[] =
  "Target is higher than 13";

// SolveBoard()
constexpr int RETURN_SOLNS_WRONG_LO = -8;
constexpr const char TEXT_SOLNS_WRONG_LO[] =
  "Solutions parameter is less than 1";

// SolveBoard()
constexpr int RETURN_SOLNS_WRONG_HI = -9;
constexpr const char TEXT_SOLNS_WRONG_HI[] =
  "Solutions parameter is higher than 3";

// SolveBoard(), self-explanatory.
constexpr int RETURN_TOO_MANY_CARDS = -10;
constexpr const char TEXT_TOO_MANY_CARDS[] = "Too many cards";

// SolveBoard()
constexpr int RETURN_SUIT_OR_RANK = -12;
constexpr const char TEXT_SUIT_OR_RANK[] =
  "currentTrickSuit or currentTrickRank has wrong data";

// SolveBoard
constexpr int RETURN_PLAYED_CARD = -13;
constexpr const char TEXT_PLAYED_CARD[] =
  "Played card also remains in a hand";

// SolveBoard()
constexpr int RETURN_CARD_COUNT = -14;
constexpr const char TEXT_CARD_COUNT[] =
  "Wrong number of remaining cards in a hand";

// SolveBoard()
constexpr int RETURN_THREAD_INDEX = -15;
constexpr const char TEXT_THREAD_INDEX[] =
  "Thread index is not 0 .. maximum";

// SolveBoard()
constexpr int RETURN_MODE_WRONG_LO = -16;
constexpr const char TEXT_MODE_WRONG_LO[] =
  "Mode parameter is less than 0";

// SolveBoard()
constexpr int RETURN_MODE_WRONG_HI = -17;
constexpr const char TEXT_MODE_WRONG_HI[] =
  "Mode parameter is higher than 2";

// SolveBoard()
constexpr int RETURN_TRUMP_WRONG = -18;
constexpr const char TEXT_TRUMP_WRONG[] = "Trump is not in 0 .. 4";

// SolveBoard()
constexpr int RETURN_FIRST_WRONG = -19;
constexpr const char TEXT_FIRST_WRONG[] = "First is not in 0 .. 2";

// AnalysePlay*() family of functions.
// (a) Less than 0 or more than 52 cards supplied.
// (b) Invalid suit or rank supplied.
// (c) A played card is not held by the right player.
constexpr int RETURN_PLAY_FAULT = -98;
constexpr const char TEXT_PLAY_FAULT[] = "AnalysePlay input error";

// Returned from a number of places if a PBN string is faulty.
constexpr int RETURN_PBN_FAULT = -99;
constexpr const char TEXT_PBN_FAULT[] = "PBN string error";

// SolveBoard() and AnalysePlay*()
constexpr int RETURN_TOO_MANY_BOARDS = -101;
constexpr const char TEXT_TOO_MANY_BOARDS[] =
  "Too many Boards requested";

// Returned from multi-threading functions.
constexpr int RETURN_THREAD_CREATE = -102;
constexpr const char TEXT_THREAD_CREATE[] =
  "Could not create threads";

// Returned from multi-threading functions when something went
// wrong while waiting for all threads to complete.
constexpr int RETURN_THREAD_WAIT = -103;
constexpr const char TEXT_THREAD_WAIT[] =
  "Something failed waiting for thread to end";

// Tried to set a multi-threading system that is not present in DLL.
constexpr int RETURN_THREAD_MISSING = -104;
constexpr const char TEXT_THREAD_MISSING[] =
  "Multi-threading system not present";

// CalcAllTables*()
constexpr int RETURN_NO_SUIT = -201;
constexpr const char TEXT_NO_SUIT[] =
  "Denomination filter vector has no entries";

// CalcAllTables*()
constexpr int RETURN_TOO_MANY_TABLES = -202;
constexpr const char TEXT_TOO_MANY_TABLES[] =
  "Too many DD tables requested";

// SolveAllChunks*()
constexpr int RETURN_CHUNK_SIZE = -301;
constexpr const char TEXT_CHUNK_SIZE[] = "Chunk size is less than 1";

// Par(), SidesPar(), SidesParBin(), DealerPar(), DealerParBin()
constexpr int RETURN_PAR_TABLE_FAULT = -401;
constexpr const char TEXT_PAR_TABLE_FAULT[] =
  "Missing double dummy table, or an entry outside the range 0 to 13";

// ---------------------------------------------------------------------------
// Solver tuning constants
// ---------------------------------------------------------------------------

constexpr int THREADMEM_SMALL_MAX_MB = 30;
constexpr int THREADMEM_SMALL_DEF_MB = 20;
constexpr int THREADMEM_LARGE_MAX_MB = 160;
constexpr int THREADMEM_LARGE_DEF_MB = 95;

constexpr int MAXNODE = 1;
constexpr int MINNODE = 0;

constexpr int SIMILARDEALLIMIT = 5;
constexpr int SIMILARMAXWINNODES = 700000;

/* "hand" is leading hand, "relative" is hand relative leading
hand.
The handId macro implementation follows a solution
by Thomas Andrews.
All hand identities are given as
0=NORTH, 1=EAST, 2=SOUTH, 3=WEST. */

/**
 * @brief Calculate relative hand position.
 * @param hand Base hand position (0=NORTH, 1=EAST, 2=SOUTH, 3=WEST)
 * @param relative Relative offset (0-3)
 * @return Resulting hand position (0-3)
 */
#define HAND_ID(hand, relative) ((hand + relative) & 3)
