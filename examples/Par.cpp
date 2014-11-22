// Test program for the Par function.
// Uses the hands pre-set in hands.cpp.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"


int main()
{
  ddTableResults DDtable;
  parResults	pres;

  int 		res;
  char		line[80];
  bool		match;

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  for (int handno = 0; handno < 3; handno++)
  {
    SetTable(&DDtable, handno);

    res = Par(&DDtable, &pres, vul[handno]);

    if (res != RETURN_NO_FAULT)
    {
      ErrorMessage(res, line);
      printf("DDS error: %s\n", line);
    }

    match = ComparePar(&pres, handno);

    printf("Par, hand %d: %s\n\n",
      handno+1, (match ? "OK" : "ERROR"));

    PrintTable(&DDtable);

    PrintPar(&pres);
  }
}
