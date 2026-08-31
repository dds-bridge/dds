/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// @file dds_data_types.hpp
/// @brief Every DDS data type that the pure-C ABI shim does not need.
///
/// This is the internal / C++-side view of the solver's data model: the
/// legacy plain-old-data structures that never cross the `dds_c_api.h`
/// boundary (batch containers, PBN variants, the par-result family, play
/// traces, `DDSInfo`) plus the core search structures used throughout the
/// solver (`Pos`, `MoveType`, `EvalType`, ...). The C-ABI subset lives in
/// `dds_c_data_types.h`, which this header includes, so `#include
/// <api/dds_data_types.hpp>` gives internal code the whole data model
/// without pulling in the public function-declaration surface
/// (`api/dll.h`).

#include <api/dds_c_data_types.h>
#include <api/dds_constants.hpp>
#include <utility/constants.h>  // card-representation lookup tables (lho/rho/partner,
                                // bit_map_rank, card_rank/suit/hand)

// ===========================================================================
// Legacy plain-old-data structures (not part of the pure-C ABI shim)
// ===========================================================================

/**
 * @brief Represents a bridge Deal in PBN (Portable Bridge Notation) format.
 *
 * @param trump The trump suit
 * @param first The hand to play first
 * @param currentTrickSuit Suits of cards played in the current trick
 * @param currentTrickRank Ranks of cards played in the current trick
 * @param remainCards PBN string describing remaining cards. Only the first hand may have a compass letter (N/E/S/W); later hands follow clockwise with no extra directions.
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

struct DdTableDeals
{
  int no_of_tables;
  struct DdTableDeal deals[MAXNOOFTABLES * DDS_STRAINS];
};

struct DdTableDealsPBN
{
  int no_of_tables;
  struct DdTableDealPBN deals[MAXNOOFTABLES * DDS_STRAINS];
};

struct DdTablesRes
{
  int no_of_boards;
  struct DdTableResults results[MAXNOOFTABLES * DDS_STRAINS];
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
       The detailed text format is given in the DLL interface
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


// ===========================================================================
// Core search structures used throughout the solver
// ===========================================================================

/**
 * @brief Represents a single card move in the game.
 *
 * Contains information about a card that can be played, including
 * its suit, rank, sequence status, and sorting weight.
 */
struct MoveType
{
  int suit;      ///< Suit of the card (0-3: spades, hearts, diamonds, clubs)
  int rank;      ///< Rank of the card (2-14: 2 through Ace)
  int sequence;  ///< Whether this move is the first in a sequence
  int weight;    ///< Weight used for sorting during move generation
};

/**
 * @brief Collection of moves available at a single ply.
 *
 * Stores all possible moves at a given point in the game,
 * along with tracking of current and last move indices.
 */
struct MovePlyType
{
  MoveType move[14];  ///< Array of possible moves (max 13 cards + sentinel)
  int current;        ///< Index of current move being considered
  int last;           ///< Index of last valid move in array
};

/**
 * @brief Identifies a high card by rank and holding hand.
 *
 * Used to track high cards in each suit during analysis.
 */
struct HighCardType
{
  int rank;  ///< Rank of the high card (2-14)
  int hand;  ///< Hand holding the card (0-3: N, E, S, W)
};

/**
 * @brief Complete position state during game analysis.
 *
 * Represents the full state of a bridge position including card distribution,
 * trump information, and current play state. This is the core data structure
 * used throughout the solver.
 */
struct Pos
{
  unsigned short int rank_in_suit[DDS_HANDS][DDS_SUITS];  ///< Bitmask of ranks held by each hand in each suit
  unsigned short int aggr[DDS_SUITS];                      ///< Aggregate bitmask of all cards in each suit
  unsigned char length[DDS_HANDS][DDS_SUITS];              ///< Number of cards each hand holds in each suit
  int hand_dist[DDS_HANDS];                                ///< Total number of cards held by each hand

  unsigned short int win_ranks[50][DDS_SUITS];  ///< Cards that win by rank at each depth
  int first[50];                                ///< Hand that leads the trick for each ply
  MoveType move[50];                            ///< Presently winning move at each ply
  int hand_rel_first;                           ///< Current hand, relative to first hand
  int tricks_max;                               ///< Aggregated tricks won by maximizing side
  HighCardType winner[DDS_SUITS];               ///< Winning rank of trick in each suit
  HighCardType second_best[DDS_SUITS];          ///< Second best rank in each suit
};

/**
 * @brief Trick-level data for current play state.
 *
 * Tracks information about the current trick being played,
 * including play counts, best cards, and lead information.
 */
struct TrickDataType
{
  int play_count[DDS_SUITS];  ///< Number of cards played in each suit
  int best_rank;              ///< Rank of best card played so far
  int best_suit;              ///< Suit of best card played so far
  int best_sequence;          ///< Sequence of best card
  int rel_winner;             ///< Relative position of current trick winner
  int next_lead_hand;         ///< Hand that will lead next trick
};

/**
 * @brief Evaluation result for a position.
 *
 * Contains the number of tricks that can be won and which specific
 * card ranks can win in each suit.
 */
struct EvalType
{
  int tricks;                             ///< Number of tricks that can be won from this position
  unsigned short int win_ranks[DDS_SUITS];  ///< Bitmask of winning ranks in each suit
};

/**
 * @brief Simple card representation.
 *
 * Basic structure identifying a card by suit and rank.
 */
struct Card
{
  int suit;  ///< Suit of the card (0-3: spades, hearts, diamonds, clubs)
  int rank;  ///< Rank of the card (2-14: 2 through Ace)
};

/**
 * @brief Extended card representation with sequence information.
 *
 * Like Card but includes sequence information for tracking
 * equivalent cards during move generation.
 */
struct ExtCard
{
  int suit;      ///< Suit of the card (0-3: spades, hearts, diamonds, clubs)
  int rank;      ///< Rank of the card (2-14: 2 through Ace)
  int sequence;  ///< Sequence identifier for equivalent cards
};

/**
 * @brief Absolute rank with holding hand.
 *
 * Compact representation (2 bytes) identifying a card rank
 * and which hand holds it.
 */
struct AbsRankType // 2 bytes
{
  char rank;         ///< Rank of the card (2-14)
  signed char hand;  ///< Hand holding the card (0-3: N, E, S, W)
};

/**
 * @brief Relative rank table for all suits.
 *
 * Contains absolute rank information for all possible card positions
 * across all suits. Used for quick lookup during position analysis.
 */
struct RelRanksType // 120 bytes
{
  AbsRankType abs_rank[15][DDS_SUITS];  ///< Rank information indexed by position and suit
};

/**
 * @brief Parameters for batch board solving.
 *
 * Contains input/output structures for solving multiple boards
 * in a single operation.
 */
struct ParamType
{
  int no_of_boards;            ///< Number of boards to solve
  Boards const * bop;          ///< Pointer to input boards
  SolvedBoards * solvedp;      ///< Pointer to output solutions
  int error;                   ///< Error code from operation
};

/**
 * @brief Execution mode for solver operations.
 *
 * Determines how the solver processes a position - solving for best play,
 * calculating all possible outcomes, or tracing a specific line of play.
 */
enum class RunMode
{
  DDS_RUN_SOLVE = 0,  ///< Solve mode: find optimal play
  DDS_RUN_CALC = 1,   ///< Calculate mode: compute all outcomes
  DDS_RUN_TRACE = 2,  ///< Trace mode: analyze specific play sequence
  DDS_RUN_SIZE = 3    ///< Size sentinel (not a valid mode)
};
