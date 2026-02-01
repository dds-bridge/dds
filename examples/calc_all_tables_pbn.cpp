/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the CalcAllTablesPBN function.
// Uses the hands pre-set in hands.cpp.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"


auto main() -> int
{
  DdTableDealsPBN DDdealsPBN;
  DdTablesRes tableRes;
  AllParResults pres;

  int mode = 0; // No par calculation
  int trumpFilter[DDS_STRAINS] = {0, 0, 0, 0, 0}; // All
  int res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  DDdealsPBN.no_of_tables = 3;

  for (int handno = 0; handno < 3; handno++)
  {
    strcpy(DDdealsPBN.deals[handno].cards, pbn_hands_[handno]);
  }

  res = CalcAllTablesPBN(&DDdealsPBN, mode, trumpFilter,
                         &tableRes, &pres);

  if (res != RETURN_NO_FAULT)
  {
    ErrorMessage(res, line);
    printf("DDS error: %s\n", line);
  }

  for (int handno = 0; handno < 3; handno++)
  {
    match = compare_table(&tableRes.results[handno], handno);

    sprintf(line,
            "CalcDDtable, hand %d: %s\n",
            handno + 1, (match ? "OK" : "ERROR"));

    print_pbn_hand(line, DDdealsPBN.deals[handno].cards);

    print_table(&tableRes.results[handno]);
  }
}

