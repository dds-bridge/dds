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

enum class MgType {
  NT0 = 0,
  TRUMP0 = 1,
  NT_VOID1 = 2,
  TRUMP_VOID1 = 3,
  NT_NOTVOID1 = 4,
  TRUMP_NOTVOID1 = 5,
  NT_VOID2 = 6,
  TRUMP_VOID2 = 7,
  NT_NOTVOID2 = 8,
  TRUMP_NOTVOID2 = 9,
  NT_VOID3 = 10,
  TRUMP_VOID3 = 11,
  COMB_NOTVOID3 = 12,
  SIZE = 13
};

/**
 * @brief Move generator and tracker for bridge double dummy solver.
 *
 * The Moves class generates, tracks, and manages possible card plays (moves)
 * during double dummy analysis. It provides interfaces for move generation,
 * selection, statistics, and printing, supporting both notrump and suit
 * contracts. Moves is an internal component and not part of the public API.
 */
class Moves {
public:
  int leadHand;
  int leadSuit;
  int currHand;
  int currTrick;
  int trump;
  int suit;
  int numMoves;
  int lastNumMoves;

  trackType track[13];
  trackType *trackp;

  MovePlyType moveList[13][DDS_HANDS];

  MoveType *mply;

  MgType lastCall[13][DDS_HANDS];

  std::string funcName[static_cast<int>(MgType::SIZE)];

  struct moveStatType {
    int count;
    int findex;
    int sumHits;
    int sumLengths;
  };

  struct moveStatsType {
    int nfuncs;
    moveStatType list[static_cast<int>(MgType::SIZE)];
  };

  moveStatType trickTable[13][DDS_HANDS];

  moveStatType trickSuitTable[13][DDS_HANDS];

  moveStatsType trickDetailTable[13][DDS_HANDS];

  moveStatsType trickDetailSuitTable[13][DDS_HANDS];

  moveStatsType trickFuncTable;

  moveStatsType trickFuncSuitTable;

  auto GetTopNumber(const int ris, const int prank, int &topNumber,
                    int &mno) const -> void;

  inline auto WinningMove(const MoveType &mvp1, const ExtCard &mvp2,
                          const int trump) const -> bool;

  auto PrintMove(const MovePlyType &mply) const -> std::string;

  auto MergeSort() -> void;

  auto CallHeuristic(const Pos &tpos, const MoveType &bestMove,
                     const MoveType &bestMoveTT, const RelRanksType thrp_rel[])
      -> void;

  // (logging accessors removed)

  auto UpdateStatsEntry(moveStatsType &stat, const int findex, const int hit,
                        const int len) const -> void;

  auto AverageString(const moveStatType &statp) const -> std::string;

  auto FullAverageString(const moveStatType &statp) const -> std::string;

  auto PrintTrickTable(const moveStatType tablep[][DDS_HANDS]) const
      -> std::string;

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

  auto Init(const int tricks, const int relStartHand, const int initialRanks[],
            const int initialSuits[],
            const unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS],
            const int trump, const int leadHand) -> void;

  auto Reinit(const int tricks, const int leadHand) -> void;

  auto MoveGen0(const int tricks, const Pos &tpos, const MoveType &bestMove,
                const MoveType &bestMoveTT, const RelRanksType thrp_rel[])
      -> int;

  auto MoveGen123(const int tricks, const int relHand, const Pos &tpos) -> int;

  auto GetLength(const int trick, const int relHand) const -> int;

  auto MakeSpecific(const MoveType &mply, const int trick, const int relHand)
      -> void;

  auto MakeNext(const int trick, const int relHand,
                const unsigned short win_ranks[DDS_SUITS]) -> MoveType const *;

  auto MakeNextSimple(const int trick, const int relHand) -> MoveType const *;

  auto Step(const int tricks, const int relHand) -> void;

  auto Rewind(const int tricks, const int relHand) -> void;

  auto Purge(const int tricks, const int relHand,
             const MoveType forbiddenMoves[]) -> void;

  auto Reward(const int trick, const int relHand) -> void;

  auto GetTrickData(const int tricks) -> const TrickDataType &;

  auto Sort(const int tricks, const int relHand) -> void;

  auto PrintMoves(const int trick, const int relHand) const -> std::string;

  auto RegisterHit(const int tricks, const int relHand) -> void;

  auto TrickToText(const int trick) const -> std::string;

  auto PrintTrickStats(std::ofstream &fout) const -> void;

  auto PrintTrickDetails(std::ofstream &fout) const -> void;

  auto PrintFunctionStats(std::ofstream &fout) const -> void;
};
