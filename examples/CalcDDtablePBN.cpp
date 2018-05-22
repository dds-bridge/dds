/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the CalcDDtablePBN function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"


int main()
{
  ddTableDealPBN tableDealPBN;
  ddTableResults table;

  int res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 3; handno++)
  {
    strcpy(tableDealPBN.cards, PBN[handno]);

    res = CalcDDtablePBN(tableDealPBN, &table);

    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match = CompareTable(&table, handno);

    sprintf(line,
            "CalcDDtable, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    PrintPBNHand(line, tableDealPBN.cards);

    PrintTable(&table);
  }
}
