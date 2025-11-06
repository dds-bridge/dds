/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_UTILITY_CONSTANTS_H
#define DDS_UTILITY_CONSTANTS_H

// Constants from dll.hpp
constexpr int DDS_STRAINS = 5;
constexpr int DDS_HANDS = 4;
constexpr int DDS_SUITS = 4;
constexpr int DDS_NOTRUMP = 4;

// Hand relationship arrays
extern int lho[DDS_HANDS];
extern int rho[DDS_HANDS];
extern int partner[DDS_HANDS];

// Card mapping arrays
extern unsigned short int bitMapRank[16];
extern unsigned char cardRank[16];
extern unsigned char cardSuit[DDS_STRAINS];
extern unsigned char cardHand[DDS_HANDS];

#endif
