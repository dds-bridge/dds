/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

// System headers
#if defined(DDS_MEMORY_LEAKS) && defined(_MSC_VER)
  #define DDS_MEMORY_LEAKS_WIN32
  #define _CRTDBG_MAP_ALLOC
  #include <crtdbg.h>
#endif

// Project headers
#include <api/dll.h>


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

#include <utility/constants.h>

/**
 * @brief Calculate relative hand position.
 * @param hand Base hand position (0=NORTH, 1=EAST, 2=SOUTH, 3=WEST)
 * @param relative Relative offset (0-3)
 * @return Resulting hand position (0-3)
 */
#define HAND_ID(hand, relative) ((hand + relative) & 3)

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
