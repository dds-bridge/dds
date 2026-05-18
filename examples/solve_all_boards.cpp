/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the SolveAllBoards function.
// Uses the hands pre-set in hands.cpp.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"


auto main() -> int
{
  BoardsPBN bo;
  SolvedBoards solved;

  int res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  bo.no_of_boards = 3;
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

    strcpy(bo.deals[handno].remainCards, pbn_hands_[handno]);

    bo.target [handno] = -1;
    bo.solutions[handno] = 3;
    bo.mode [handno] = 0;
  }

  res = SolveAllBoards(&bo, &solved);

  if (res != RETURN_NO_FAULT)
  {
    ErrorMessage(res, line);
    printf("DDS error: %s\n", line);
  }

  for (int handno = 0; handno < 3; handno++)
  {
    match = compare_future_tricks(&solved.solved_board[handno], handno, 3);

    sprintf(line,
            "SolveAllBoards, hand %d: solutions 3 %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    print_pbn_hand(line, bo.deals[handno].remainCards);

    print_future_tricks(line, &solved.solved_board[handno]);
  }
}
