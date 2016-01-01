/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the AnalysePlayPBN function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"


int main()
{
  dealPBN dlPBN;
  playTracePBN DDplayPBN;
  solvedPlay solved;

  int threadIndex = 0, res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 3; handno++)
  {
    dlPBN.trump = trump[handno];
    dlPBN.first = first[handno];

    dlPBN.currentTrickSuit[0] = 0;
    dlPBN.currentTrickSuit[1] = 0;
    dlPBN.currentTrickSuit[2] = 0;

    dlPBN.currentTrickRank[0] = 0;
    dlPBN.currentTrickRank[1] = 0;
    dlPBN.currentTrickRank[2] = 0;

    strcpy(dlPBN.remainCards, PBN[handno]);

    DDplayPBN.number = playNo[handno];
    strcpy(DDplayPBN.cards, play[handno]);

    res = AnalysePlayPBN(dlPBN, DDplayPBN, &solved, threadIndex);

    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match = ComparePlay(&solved, handno);

    sprintf(line, "AnalysePlayPBNBin, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    PrintPBNHand(line, dlPBN.remainCards);

    PrintPBNPlay(&DDplayPBN, &solved);
  }
}

