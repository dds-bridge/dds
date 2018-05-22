/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <cstdlib>

#include "../include/dll.h"
#include "testcommon.h"
#include "args.h"
#include "cst.h"

using namespace std;

OptionsType options;


int main(int argc, char * argv[])
{
  ReadArgs(argc, argv);

  if (options.threading != DTEST_THREADING_DEFAULT)
    SetThreading(static_cast<int>(options.threading));

  SetResources(options.memoryMB, options.numThreads);

  DDSInfo info;
  GetDDSInfo(&info);
  cout << info.systemString << endl;

  realMain(argc, argv);

#ifdef DDS_SCHEDULER
  scheduler.PrintTiming();
#endif

  FreeMemory();

  exit(0);
}

