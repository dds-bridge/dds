/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Test program for the DealerPar function.
// Uses the hands pre-set in hands.cpp.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"


auto main() -> int
{
  DdTableResults DDtable;
  ParResultsDealer pres;

  int res;
  char line[80];
  bool match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 3; handno++)
  {
    set_table(&DDtable, handno);

    res = DealerPar(&DDtable, &pres, dealer_hand_[handno], vulnerability_[handno]);

    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match = compare_dealer_par(&pres, handno);

    printf("DealerPar, hand %d: %s\n\n",
           handno + 1, (match ? "OK" : "ERROR"));

    print_table(&DDtable);

    print_dealer_par(&pres);
  }
}
