/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "constants.h"

/// Left-hand opponent array: lho[hand] gives the hand sitting to hand's left.
/// Bridge hand positions: 0=North, 1=East, 2=South, 3=West (compass orientation)
/// From each player's perspective (compass direction):
/// North's left=East(1), East's left=South(2), South's left=West(3), West's left=North(0)
const int lho[DDS_HANDS] = {1, 2, 3, 0};

/// Right-hand opponent array: rho[hand] gives the hand sitting to hand's right.
/// Bridge hand positions: 0=North, 1=East, 2=South, 3=West (compass orientation)
/// From each player's perspective (compass direction):
/// North's right=West(3), East's right=North(0), South's right=East(1), West's right=South(2)
const int rho[DDS_HANDS] = {3, 0, 1, 2};
const int partner[DDS_HANDS] = {2, 3, 0, 1};

/// Bitmask representation of card ranks.
/// Maps absolute rank (2-14, plus sentinels) to bitmask for efficient
/// rank membership testing and operations. Index r holds 1 << (r-2).
const unsigned short bit_map_rank[16] = {
    0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
    0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000};

/// Text representation of card ranks for human-readable output.
/// Maps rank (0-15) to character: '2' through 'A', with sentinels 'x' and '-'.
const unsigned char card_rank[16] = {
    'x', 'x', '2', '3', '4', '5', '6', '7',
    '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'};

/// Text representation of suits for human-readable output.
/// Maps suit (0-4) to character: S/H/D/C/N (Spades/Hearts/Diamonds/Clubs/NoTrump).
const unsigned char card_suit[DDS_STRAINS] = {'S', 'H', 'D', 'C', 'N'};

/// Text representation of hands for human-readable output.
/// Maps hand (0-3) to character: N/E/S/W (North/East/South/West).
const unsigned char card_hand[DDS_HANDS] = {'N', 'E', 'S', 'W'};
