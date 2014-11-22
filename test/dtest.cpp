#include <stdio.h>
#include <stdlib.h>
#include "../include/dll.h"
#include "testcommon.h"

int main(int argc, char * argv[])
{
  int ncores = 0;
  if (argc == 4)
    ncores = atoi(argv[3]);

  SetMaxThreads(ncores);

  realMain(argc, argv);

  exit(0);
}
