/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the SolveBoardPBN function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"


int main()
{
  dealPBN dlPBN;
  futureTricks fut2, // solutions == 2
                fut3; // solutions == 3

  int target;
  int solutions;
  int mode;
  int threadIndex = 0;
  int res;
  char line[80];
  bool match2,
                match3;

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

    target = -1;
    solutions = 3;
    mode = 0;
    res = SolveBoardPBN(dlPBN, target, solutions, mode, &fut3, 0);

    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match3 = CompareFut(&fut3, handno, solutions);

    solutions = 2;
    res = SolveBoardPBN(dlPBN, target, solutions, mode, &fut2, 0);
    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match2 = CompareFut(&fut2, handno, solutions);

    sprintf(line,
            "SolveBoardPBN, hand %d: solutions 3 %s, solutions 2 %s\n",
            handno + 1,
            (match3 ? "OK" : "ERROR"),
            (match2 ? "OK" : "ERROR"));

    PrintPBNHand(line, dlPBN.remainCards);

    sprintf(line, "solutions == 3\n");
    PrintFut(line, &fut3);
    sprintf(line, "solutions == 2\n");
    PrintFut(line, &fut2);
  }
}
