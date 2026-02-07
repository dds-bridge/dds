/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// @file constants.h
/// @brief Utility constants and lookup tables for card representation.
/// @defgroup utility_constants Utility Constants
/// @{

/// Global bridge game dimensions
constexpr int DDS_STRAINS = 5;  ///< Number of strains (4 suits + no trump)
constexpr int DDS_HANDS = 4;    ///< Number of hands (N/E/S/W)
constexpr int DDS_SUITS = 4;    ///< Number of suits (S/H/D/C)
constexpr int DDS_NOTRUMP = 4;  ///< No trump strain index

/// @name Hand Relationship Arrays
/// Precomputed lookup tables for hand relationships. Each array maps
/// an absolute hand (0-3) to the corresponding related hand.
/// @{

/// @brief Left-hand opponent for each hand.
/// Maps: North(0)->West(3), East(1)->North(0), South(2)->East(1), West(3)->South(2)
extern const int lho[DDS_HANDS];

/// @brief Right-hand opponent for each hand.
/// Maps: North(0)->East(1), East(1)->South(2), South(2)->West(3), West(3)->North(0)
extern const int rho[DDS_HANDS];

/// @brief Partner for each hand.
/// Maps: North(0)->South(2), East(1)->West(3), South(2)->North(0), West(3)->East(1)
extern const int partner[DDS_HANDS];

/// @}

/// @name Card Representation Lookup Tables
/// These tables provide efficient O(1) conversions between different
/// card representations (bitmask, rank, suit, hand).
/// @{

/// @brief Bitmask representation for card ranks.
/// Maps absolute rank (0-15) to bitmask (0x0000-0x2000).
/// Rank 2->0x0001, Rank 3->0x0002, ..., Rank Ace(14)->0x1000.
/// Indices 0, 1, 15 are sentinel values.
extern const unsigned short bit_map_rank[16];

/// @brief Character representation for card ranks.
/// Maps absolute rank (0-15) to printable character.
/// Valid ranks: 2-A map to '2'-'A', Ace is 'A', sentinel values are 'x', '-'.
extern const unsigned char card_rank[16];

/// @brief Character representation for suits.
/// Maps suit index (0-4) to character: S/H/D/C/N (north trump).
extern const unsigned char card_suit[DDS_STRAINS];

/// @brief Character representation for hands.
/// Maps hand (0-3) to character: N/E/S/W (North/East/South/West).
extern const unsigned char card_hand[DDS_HANDS];

/// @}

/// @}
