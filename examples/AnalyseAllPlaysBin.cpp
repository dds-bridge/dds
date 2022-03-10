/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the AnalyseAllPlaysBin function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"


extern unsigned char dcardSuit[5], dcardRank[16];

int main()
{
  boards bo;
  playTracesBin DDplays;
  solvedPlays solved;

  int chunkSize = 1, res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  bo.noOfBoards = 3;
  DDplays.noOfBoards = 3;

  for (int handno = 0; handno < 3; handno++)
  {
    bo.deals[handno].trump = trump[handno];
    bo.deals[handno].first = first[handno];

    bo.deals[handno].currentTrickSuit[0] = 0;
    bo.deals[handno].currentTrickSuit[1] = 0;
    bo.deals[handno].currentTrickSuit[2] = 0;

    bo.deals[handno].currentTrickRank[0] = 0;
    bo.deals[handno].currentTrickRank[1] = 0;
    bo.deals[handno].currentTrickRank[2] = 0;

    for (int h = 0; h < DDS_HANDS; h++)
      for (int s = 0; s < DDS_SUITS; s++)
        bo.deals[handno].remainCards[h][s] = holdings[handno][s][h];

    DDplays.plays[handno].number = playNo[handno];
    for (int i = 0; i < playNo[handno]; i++)
    {
      DDplays.plays[handno].suit[i] = playSuit[handno][i];
      DDplays.plays[handno].rank[i] = playRank[handno][i];
    }
  }

  res = AnalyseAllPlaysBin(&bo, &DDplays, &solved, chunkSize);

  if (res != RETURN_NO_FAULT)
  {
    ErrorMessage(res, line);
    printf("DDS error: %s\n", line);
  }

  for (int handno = 0; handno < 3; handno++)
  {
    match = ComparePlay(&solved.solved[handno], handno);

    sprintf(line, "AnalyseAllPlaysBin, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    PrintHand(line, bo.deals[handno].remainCards);

    PrintBinPlay(&DDplays.plays[handno], &solved.solved[handno]);
  }
}

