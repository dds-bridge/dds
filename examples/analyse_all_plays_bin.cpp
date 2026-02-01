/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the AnalyseAllPlaysBin function.
// Uses the hands pre-set in hands.cpp.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"


extern unsigned char card_suit_chars_[5], card_rank_chars_[16];

auto main() -> int
{
  Boards bo;
  PlayTracesBin DDplays;
  SolvedPlays solved;

  int chunkSize = 1, res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  bo.no_of_boards = 3;
  DDplays.no_of_boards = 3;

  for (int handno = 0; handno < 3; handno++)
  {
    bo.deals[handno].trump = trump_suit_[handno];
    bo.deals[handno].first = first_hand_[handno];

    bo.deals[handno].currentTrickSuit[0] = 0;
    bo.deals[handno].currentTrickSuit[1] = 0;
    bo.deals[handno].currentTrickSuit[2] = 0;

    bo.deals[handno].currentTrickRank[0] = 0;
    bo.deals[handno].currentTrickRank[1] = 0;
    bo.deals[handno].currentTrickRank[2] = 0;

    for (int h = 0; h < DDS_HANDS; h++)
      for (int s = 0; s < DDS_SUITS; s++)
        bo.deals[handno].remainCards[h][s] = holdings_[handno][s][h];

    DDplays.plays[handno].number = play_count_[handno];
    for (int i = 0; i < play_count_[handno]; i++)
    {
      DDplays.plays[handno].suit[i] = play_suit_[handno][i];
      DDplays.plays[handno].rank[i] = play_rank_[handno][i];
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
    match = compare_play(&solved.solved[handno], handno);

    sprintf(line, "AnalyseAllPlaysBin, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    print_hand(line, bo.deals[handno].remainCards);

    print_bin_play(&DDplays.plays[handno], &solved.solved[handno]);
  }
}

