#include <stdio.h>
#include <stdlib.h>
#include "../include/dll.h"
#include "testcommon.h"


void DDSidentify(char * s);


int main(int argc, char * argv[])
{
  int ncores = 0;
  if (argc == 4)
    ncores = atoi(argv[3]);

  SetMaxThreads(ncores);

  char DDSid[400];
  DDSidentify(DDSid);
  printf("%s", DDSid);
  fflush(stdout);
  
  realMain(argc, argv);

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  FreeMemory();

  exit(0);
}

