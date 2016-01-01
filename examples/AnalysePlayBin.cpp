/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the AnalysePlayBin function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"


extern unsigned char dcardSuit[5], dcardRank[16];

int main()
{
  deal dl;
  playTraceBin DDplay;
  solvedPlay solved;

  int threadIndex = 0, res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 3; handno++)
  {
    dl.trump = trump[handno];
    dl.first = first[handno];

    dl.currentTrickSuit[0] = 0;
    dl.currentTrickSuit[1] = 0;
    dl.currentTrickSuit[2] = 0;

    dl.currentTrickRank[0] = 0;
    dl.currentTrickRank[1] = 0;
    dl.currentTrickRank[2] = 0;

    for (int h = 0; h < DDS_HANDS; h++)
      for (int s = 0; s < DDS_SUITS; s++)
        dl.remainCards[h][s] = holdings[handno][s][h];

    DDplay.number = playNo[handno];
    for (int i = 0; i < playNo[handno]; i++)
    {
      DDplay.suit[i] = playSuit[handno][i];
      DDplay.rank[i] = playRank[handno][i];
    }

    res = AnalysePlayBin(dl, DDplay, &solved, threadIndex);

    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match = ComparePlay(&solved, handno);

    sprintf(line, "AnalysePlayBin, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    PrintHand(line, dl.remainCards);

    PrintBinPlay(&DDplay, &solved);
  }
}

