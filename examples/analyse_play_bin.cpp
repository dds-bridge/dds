/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the AnalysePlayBin function.
// Uses the hands pre-set in hands.cpp.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"


extern unsigned char card_suit_chars_[5], card_rank_chars_[16];

auto main() -> int
{
  Deal dl;
  PlayTraceBin DDplay;
  SolvedPlay solved;

  int threadIndex = 0, res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__) || defined(__WASM__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 3; handno++)
  {
    dl.trump = trump_suit_[handno];
    dl.first = first_hand_[handno];

    dl.currentTrickSuit[0] = 0;
    dl.currentTrickSuit[1] = 0;
    dl.currentTrickSuit[2] = 0;

    dl.currentTrickRank[0] = 0;
    dl.currentTrickRank[1] = 0;
    dl.currentTrickRank[2] = 0;

    for (int h = 0; h < DDS_HANDS; h++)
      for (int s = 0; s < DDS_SUITS; s++)
        dl.remainCards[h][s] = holdings_[handno][s][h];

    DDplay.number = play_count_[handno];
    for (int i = 0; i < play_count_[handno]; i++)
    {
      DDplay.suit[i] = play_suit_[handno][i];
      DDplay.rank[i] = play_rank_[handno][i];
    }

    res = AnalysePlayBin(dl, DDplay, &solved, threadIndex);

    if (res != RETURN_NO_FAULT)
    {
 #ifdef __WASM__
      // For WASM, we can't use ErrorMessage, so we'll just print the error code
      snprintf(line, sizeof(line), "error code %d", res);
 #else
      ErrorMessage(res, line);
 #endif
      printf("DDS error: %s\n", line);
    }

    match = compare_play(&solved, handno);

    sprintf(line, "AnalysePlayBin, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    print_hand(line, dl.remainCards);

    print_bin_play(&DDplay, &solved);
  }
}

