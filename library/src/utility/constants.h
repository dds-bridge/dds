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

/// @brief Left-hand opponent for each hand (index -> hand value).
/// Precomputed lookup: lho[i] gives the hand number of the player sitting to hand i's left.
/// Values: lho[0]=1 (North's LHO is East), lho[1]=2 (East's LHO is South),
///         lho[2]=3 (South's LHO is West), lho[3]=0 (West's LHO is North)
extern const int lho[DDS_HANDS];

/// @brief Right-hand opponent for each hand (index -> hand value).
/// Precomputed lookup: rho[i] gives the hand number of the player sitting to hand i's right.
/// Values: rho[0]=3 (North's RHO is West), rho[1]=0 (East's RHO is North),
///         rho[2]=1 (South's RHO is East), rho[3]=2 (West's RHO is South)
extern const int rho[DDS_HANDS];

/// @brief Partner for each hand (index -> hand value).
/// Precomputed lookup: partner[i] gives the hand number of the player partnered with hand i.
/// Values: partner[0]=2 (North's partner is South), partner[1]=3 (East's partner is West),
///         partner[2]=0 (South's partner is North), partner[3]=1 (West's partner is East)
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
/// Maps suit index (0-4) to character: S/H/D/C/N (no trump).
extern const unsigned char card_suit[DDS_STRAINS];

/// @brief Character representation for hands.
/// Maps hand (0-3) to character: N/E/S/W (North/East/South/West).
extern const unsigned char card_hand[DDS_HANDS];

/// @}

/// @}
