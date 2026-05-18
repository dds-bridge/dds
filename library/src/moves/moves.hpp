/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <fstream>
#include <string>

#include <api/dds.h>
#include <heuristic_sorting/heuristic_sorting.hpp>

/**
 * @brief Move generation category used for heuristic tracking.
 *
 * Encodes the contract and void/not-void situation for which a move list
 * is being generated. Values are used as indices into statistics tables.
 */
enum class MgType {
  /** Notrump at trick 0. */
  NT0 = 0,
  /** Trump contract at trick 0. */
  TRUMP0 = 1,
  /** Notrump, void in one hand. */
  NT_VOID1 = 2,
  /** Trump, void in one hand. */
  TRUMP_VOID1 = 3,
  /** Notrump, no void in one hand. */
  NT_NOTVOID1 = 4,
  /** Trump, no void in one hand. */
  TRUMP_NOTVOID1 = 5,
  /** Notrump, void in two hands. */
  NT_VOID2 = 6,
  /** Trump, void in two hands. */
  TRUMP_VOID2 = 7,
  /** Notrump, no void in two hands. */
  NT_NOTVOID2 = 8,
  /** Trump, no void in two hands. */
  TRUMP_NOTVOID2 = 9,
  /** Notrump, void in three hands. */
  NT_VOID3 = 10,
  /** Trump, void in three hands. */
  TRUMP_VOID3 = 11,
  /** Combined void/not-void tracking for three hands. */
  COMB_NOTVOID3 = 12,
  /** Number of categories. */
  SIZE = 13
};

/**
 * @brief Move generator and tracker for bridge double dummy solver.
 *
 * The Moves class generates, tracks, and manages possible card plays (moves)
 * during double dummy analysis. It provides interfaces for move generation,
 * selection, statistics, and printing, supporting both notrump and suit
 * contracts. Moves is an internal component and not part of the public API.
 *
 * @section error_handling Error Handling
 * This class uses assertions for internal invariant checking. All public methods
 * assume valid input and proper initialization. Violations trigger assertions in
 * debug builds. Methods that may fail to find a move (MakeNext, MakeNextSimple)
 * return nullptr to indicate no valid move exists.
 *
 * @section memory_safety Memory Safety
 * This class uses stack-allocated arrays with RAII semantics. No dynamic memory
 * allocation is performed. The trackp and mply pointers are non-owning references
 * to elements within the stack-allocated arrays (track and moveList respectively).
 * These pointers are valid only during the lifetime of the Moves object and must
 * not outlive it.
 *
 * @section usage Typical Usage
 * 1. Create Moves object (constructor initializes statistics)
 * 2. Call Init() to set up initial state for a deal
 * 3. For opening lead: call MoveGen0() to generate moves
 * 4. For subsequent plays: call MoveGen123() for each hand
 * 5. Use MakeNext() or MakeNextSimple() to iterate through legal moves
 * 6. Call MakeSpecific() to record a move choice
 * 7. Use RegisterHit() to record statistics about chosen moves
 */
class Moves {
public:
  /** @brief Lead hand index for the current trick. */
  int leadHand;
  /** @brief Lead suit for the current trick. */
  int leadSuit;
  /** @brief Current hand index being processed. */
  int currHand;
  /** @brief Current trick number. */
  int currTrick;
  /** @brief Trump suit or DDS_NOTRUMP. */
  int trump;
  /** @brief Suit currently being generated. */
  int suit;
  /** @brief Number of moves currently generated. */
  int numMoves;
  /** @brief Previous move count used by heuristic. */
  int lastNumMoves;

  /** @brief Per-trick tracking state. */
  TrackType track[13];
  /** @brief Pointer to active track entry (non-owning, points into track array). */
  TrackType *trackp;

  /** @brief Move lists indexed by trick and relative hand. */
  MovePlyType moveList[13][DDS_HANDS];

  /** @brief Pointer to current move list storage (non-owning, points into moveList). */
  MoveType *mply;

  /** @brief Last heuristic category per trick and hand. */
  MgType lastCall[13][DDS_HANDS];

  /** @brief Human-readable names for MgType categories. */
  std::string funcName[static_cast<int>(MgType::SIZE)];

  /** @brief Aggregate statistics for a single function category. */
  struct moveStatType {
    int count;
    int findex;
    int sumHits;
    int sumLengths;
  };

  /** @brief Collection of statistics for all function categories. */
  struct moveStatsType {
    int nfuncs;
    moveStatType list[static_cast<int>(MgType::SIZE)];
  };

  /** @brief Trick-level statistics for all hands. */
  moveStatType trickTable[13][DDS_HANDS];

  /** @brief Trick-level statistics for winning suit only. */
  moveStatType trickSuitTable[13][DDS_HANDS];

  /** @brief Detailed per-function stats by trick and hand. */
  moveStatsType trickDetailTable[13][DDS_HANDS];

  /** @brief Detailed per-function stats by trick/hand for winning suit. */
  moveStatsType trickDetailSuitTable[13][DDS_HANDS];

  /** @brief Aggregated function stats across all tricks. */
  moveStatsType trickFuncTable;

  /** @brief Aggregated function stats for winning suit. */
  moveStatsType trickFuncSuitTable;

  /**
   * @brief Compute top number of winning moves for a given rank.
   *
   * @param ris Rank-in-suit bitmask
   * @param prank Partner rank
   * @param topNumber Output: top move number
   * @param mno Output: move index
   */
  auto GetTopNumber(const int ris, const int prank, int &topNumber,
                    int &mno) const -> void;

  /**
   * @brief Determine whether one move wins over another.
   *
   * @param mvp1 Candidate move
   * @param mvp2 Current winning card
   * @param trump Trump suit
   * @return True if mvp1 wins against mvp2
   */
  inline auto WinningMove(const MoveType &mvp1, const ExtCard &mvp2,
                          const int trump) const -> bool;

  /**
   * @brief Render a move list as a printable string.
   *
   * @param mply Move list
   * @return Formatted string for debugging/logging
   */
  auto PrintMove(const MovePlyType &mply) const -> std::string;

  /** @brief Sort current move list by weight. */
  auto MergeSort() -> void;

  /**
   * @brief Invoke heuristic sorting for current move list.
   *
   * @param tpos Current position
   * @param best_move Best move from search
   * @param best_move_tt Best move from transposition table
   * @param thrp_rel Relative ranks per hand
   */
  auto call_heuristic(const Pos &tpos, const MoveType &best_move,
                     const MoveType &best_move_tt, const RelRanksType thrp_rel[])
      -> void;

  // (logging accessors removed)

    /**
     * @brief Update statistics for a single function category.
     *
     * @param stat Statistics table to update
     * @param findex Function index
     * @param hit Hit position
     * @param len List length
     */
    auto UpdateStatsEntry(moveStatsType &stat, const int findex, const int hit,
              const int len) const -> void;

    /** @brief Format average stats for a single category. */
    auto AverageString(const moveStatType &statp) const -> std::string;

    /** @brief Format detailed average stats for a single category. */
    auto FullAverageString(const moveStatType &statp) const -> std::string;

    /**
     * @brief Format trick-level statistics as a table.
     *
     * @param tablep Table of statistics
     * @return Formatted text table
     */
    auto PrintTrickTable(const moveStatType tablep[][DDS_HANDS]) const
      -> std::string;

    /**
     * @brief Format function-level statistics as a table.
     *
     * @param tablep Statistics collection
     * @return Formatted text table
     */
    auto PrintFunctionTable(const moveStatsType &tablep) const -> std::string;

  /**
   * @brief Construct a new Moves object.
   *
   * Initializes move tracking structures and prepares for move generation.
   */
  Moves();

  /**
   * @brief Destroy the Moves object and clean up resources.
   *
   * Releases all memory and performs cleanup of move tracking state.
   */
  ~Moves();

    /**
     * @brief Initialize move generation for a new deal state.
     *
     * @param tricks Current trick index
     * @param relStartHand Relative starting hand
     * @param initialRanks Initial ranks played
     * @param initialSuits Initial suits played
     * @param rank_in_suit Rank bitmaps by hand/suit
     * @param our_trump Trump suit
     * @param our_lead_hand Absolute lead hand
     */
    auto Init(const int tricks, const int relStartHand, const int initialRanks[],
        const int initialSuits[],
        const unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS],
        const int our_trump, const int our_lead_hand) -> void;

    /**
     * @brief Reset tracking state for a new lead hand.
     *
     * @param tricks Current trick index
     * @param leadHand Absolute lead hand
     */
    auto Reinit(const int tricks, const int leadHand) -> void;

    /**
     * @brief Generate moves for first hand of the trick.
     *
     * @param tricks Current trick index
     * @param tpos Current position
     * @param bestMove Best move from search
     * @param bestMoveTT Best move from transposition table
     * @param thrp_rel Relative ranks per hand
     * @return Number of generated moves
     */
    auto MoveGen0(const int tricks, const Pos &tpos, const MoveType &bestMove,
          const MoveType &bestMoveTT, const RelRanksType thrp_rel[])
      -> int;

    /**
     * @brief Generate moves for second/third/fourth hand of the trick.
     *
     * @param tricks Current trick index
     * @param relHand Relative hand index
     * @param tpos Current position
     * @return Number of generated moves
     */
    auto MoveGen123(const int tricks, const int relHand, const Pos &tpos) -> int;

    /**
     * @brief Get number of moves available for trick/hand.
     *
     * @param trick Trick index
     * @param relHand Relative hand index
     * @return Move count
     */
    auto GetLength(const int trick, const int relHand) const -> int;

    /**
     * @brief Apply a specific move to tracking state.
     *
     * @param mply Move to apply
     * @param trick Trick index
     * @param relHand Relative hand index
     */
    auto MakeSpecific(const MoveType &mply, const int trick, const int relHand)
      -> void;

    /**
     * @brief Choose next move according to win constraints.
     *
     * @param trick Trick index
     * @param relHand Relative hand index
     * @param win_ranks Minimum winning rank per suit
     * @return Pointer to chosen move, or nullptr if no valid move found
     */
    auto MakeNext(const int trick, const int relHand,
          const unsigned short win_ranks[DDS_SUITS]) -> MoveType const *;

    /**
     * @brief Choose next move without win constraints.
     *
     * @param trick Trick index
     * @param relHand Relative hand index
     * @return Pointer to chosen move, or nullptr if list exhausted
     */
    auto MakeNextSimple(const int trick, const int relHand) -> MoveType const *;

    /**
     * @brief Advance to next move in list.
     *
     * @param tricks Current trick index
     * @param relHand Relative hand index
     */
    auto Step(const int tricks, const int relHand) -> void;

    /**
     * @brief Reset move index to start of list.
     *
     * @param tricks Current trick index
     * @param relHand Relative hand index
     */
    auto Rewind(const int tricks, const int relHand) -> void;

    /**
     * @brief Remove forbidden moves from a list.
     *
     * @param tricks Current trick index
     * @param relHand Relative hand index
     * @param forbiddenMoves Move list to exclude
     */
    auto Purge(const int tricks, const int relHand,
         const MoveType forbiddenMoves[]) -> void;

    /**
     * @brief Reward the last chosen move with extra weight.
     *
     * @param trick Trick index
     * @param relHand Relative hand index
     */
    auto Reward(const int trick, const int relHand) -> void;

    /**
     * @brief Collect summary data for the current trick.
     *
     * @param tricks Current trick index
     * @return Trick data snapshot
     */
    auto GetTrickData(const int tricks) -> const TrickDataType &;

    /**
     * @brief Sort moves by heuristic weight.
     *
     * @param tricks Current trick index
     * @param relHand Relative hand index
     */
    auto Sort(const int tricks, const int relHand) -> void;

    /**
     * @brief Render moves for a trick/hand as a printable string.
     *
     * @param trick Trick index
     * @param relHand Relative hand index
     * @return Formatted string
     */
    auto PrintMoves(const int trick, const int relHand) const -> std::string;

    /**
     * @brief Register the chosen move in statistics tables.
     *
     * @param tricks Current trick index
     * @param relHand Relative hand index
     */
    auto RegisterHit(const int tricks, const int relHand) -> void;

    /**
     * @brief Render the last trick as a string.
     *
     * @param trick Trick index
     * @return Formatted string
     */
    auto TrickToText(const int trick) const -> std::string;

    /**
     * @brief Print summary trick statistics to stream.
     *
     * @param fout Output stream
     */
    auto PrintTrickStats(std::ofstream &fout) const -> void;

    /**
     * @brief Print detailed trick statistics to stream.
     *
     * @param fout Output stream
     */
    auto PrintTrickDetails(std::ofstream &fout) const -> void;

    /**
     * @brief Print aggregated function statistics to stream.
     *
     * @param fout Output stream
     */
    auto PrintFunctionStats(std::ofstream &fout) const -> void;
};
