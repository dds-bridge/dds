/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// @file dds_c_data_types.h
/// @brief The plain-old-data structures that cross the pure-C ABI shim.
///
/// This is the subset of the legacy data types that appear in
/// `dds_c_api.h` signatures (`struct Deal`, `struct FutureTricks`,
/// `struct DdTableDeal`, `struct DdTableDealPBN`, `struct DdTableResults`,
/// `struct ParResults`). It is split out from `dds_data_types.hpp` so the
/// C-ABI shim can pull in exactly what it passes by pointer and nothing
/// else. Every remaining legacy and internal type lives in
/// `dds_data_types.hpp`, which includes this header.

#include <api/dds_constants.hpp>

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

struct DdTableDeal
{
  unsigned int cards[DDS_HANDS][DDS_SUITS];
};

struct DdTableDealPBN
{
  char cards[80];
};

struct DdTableResults
{
  int res_table[DDS_STRAINS][DDS_HANDS];
};

struct ParResults
{
  /* index = 0 is NS view and index = 1
     is EW view. By 'view' is here meant
     which side that starts the bidding. */
  char par_score[2][16];
  char par_contracts_string[2][128];
};
