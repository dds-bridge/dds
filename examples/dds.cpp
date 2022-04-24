/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

/*

S "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3" xxxxxx
U "E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63" xxxxxx
S "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93" xxxxxx

*/
// Test program for the SolveBoard function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "../src/PBN.h"
#include "hands.h"

/*
unsigned char dcardRank[16] =
{ 
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'
};

unsigned char dcardSuit[5] = { 'S', 'H', 'D', 'C', 'N' };
unsigned char dcardHand[4] = { 'N', 'E', 'S', 'W' };
*/
int convert_trump_or_seat(char tmpChar)
{
  int rank = tmpChar - '0';
  if (rank < 15 && rank > 1)
    return rank;
  if (tmpChar == 'N') // North
    return 0;
  else if (tmpChar == 'E')
    return 1;
  else if (tmpChar == 'S')
    return 2;
  else if (tmpChar == 'W')
    return 3;
  else if (tmpChar == 'C')
    return 3;
  else if (tmpChar == 'D')
    return 2;
  else if (tmpChar == 'H')
    return 1;
  else if (tmpChar == 'S')
    return 0;
  else if (tmpChar == 'U') // Trump
    return 4;
  else if (tmpChar == 'x') //
    return 0;
  else if (tmpChar == 'A') //
    return 14;
  else if (tmpChar == 'K') //
    return 13;
  else if (tmpChar == 'Q') //
    return 12;
  else if (tmpChar == 'J') //
    return 11;
  else if (tmpChar == 'T') //
    return 10;
  else
    return -1;
}

/*
  DDS, a bridge double dummy solver solveBoardPBN cli interface.
  trump: 0-4 Clubs, Diamonds, Hearts, Spades, No Trump
  currentTrickSuit: CDHS, AKQJT98765432
*/
int main(int argc, char *argv[])
{
  if (argc < 4) {
    dealPBN dlPBN;
    printf("Usage: %s <board> argc=%d trump first[NESW]:dot_space_pbn currentTricks\n", argv[0], argc);
    return 1;
  }

  deal dl;
  futureTricks fut2, // solutions == 2
                fut3; // solutions == 3

  int target;
  int solutions;
  int mode;
  int threadIndex = 0;
  int res;
  char line[80];
  bool match2;
  bool match3;

#if defined(__linux) || defined(__APPLE__)  || defined(__WASM__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 1; handno++)
  {
    dl.trump = convert_trump_or_seat(argv[1][0]);
    dl.first = convert_trump_or_seat(argv[2][0]);
    // xxxxxx means 0,0,0,0,0,0
    dl.currentTrickSuit[0] = convert_trump_or_seat(argv[3][0]);
    dl.currentTrickSuit[1] = convert_trump_or_seat(argv[3][2]);
    dl.currentTrickSuit[2] = convert_trump_or_seat(argv[3][4]);

    dl.currentTrickRank[0] = convert_trump_or_seat(argv[3][1]);
    dl.currentTrickRank[1] = convert_trump_or_seat(argv[3][3]);
    dl.currentTrickRank[2] = convert_trump_or_seat(argv[3][5]);

    if (ConvertFromPBN(argv[2], dl.remainCards) != RETURN_NO_FAULT) {
      return RETURN_PBN_FAULT;
    }


    target = -1;
    mode = 0;
    solutions = 2;
    res = SolveBoard(dl, target, solutions, mode, &fut2, threadIndex);
    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }
    /*
    sprintf(line,
            "SolveBoard, hand %d: solutions 3 %s, solutions 2 %s\n",
            handno + 1,
            (match3 ? "OK" : "ERROR"),
            (match2 ? "OK" : "ERROR"));

    PrintHand(line, dl.remainCards);

    sprintf(line, "solutions == 3\n");
    PrintFut(line, &fut3);
    sprintf(line, "solutions == 2\n");
    */
    PrintFut(line, &fut2);
  }
}
