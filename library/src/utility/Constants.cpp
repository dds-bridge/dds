/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "Constants.h"

// Hand relationship arrays
int lho[DDS_HANDS] = { 1, 2, 3, 0 };
int rho[DDS_HANDS] = { 3, 0, 1, 2 };
int partner[DDS_HANDS] = { 2, 3, 0, 1 };

// bitMapRank[absolute rank] is the absolute suit corresponding
// to that rank. The absolute rank is 2 .. 14, but it is useful
// for some reason that I have forgotten to have number 15
// set as well :-).
unsigned short int bitMapRank[16] =
{
  0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000
};

unsigned char cardRank[16] =
{
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'
};

unsigned char cardSuit[DDS_STRAINS] =
{
  'S', 'H', 'D', 'C', 'N'
};

unsigned char cardHand[DDS_HANDS] =
{
  'N', 'E', 'S', 'W'
};
