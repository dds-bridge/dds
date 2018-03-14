/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <stdio.h>
#include <stdlib.h>
#include "../include/dll.h"
#include "testcommon.h"

int main(int argc, char * argv[])
{
  int ncores = 0;
  if (argc >= 4)
    ncores = atoi(argv[3]);
  SetMaxThreads(ncores);

  if (argc >= 5)
    SetThreading(threadingCode(argv[4]));

  DDSInfo info;
  GetDDSInfo(&info);
  printf("%s", info.systemString);
  printf("%-12s %20s\n\n", "Version", info.versionString);
  fflush(stdout);

// TODO
#if defined(_WIN32)
  printf("_WIN32\n");
  fflush(stdout);
#elif defined(_WIN64)
  printf("_WIN64\n");
  fflush(stdout);
#else
  printf("No WIN\n");
  fflush(stdout);
#endif

  realMain(argc, argv);

  exit(0);
}
