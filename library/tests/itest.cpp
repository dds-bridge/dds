/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <cstdlib>

#include <api/dll.h>
#include "testcommon.hpp"
#include "args.hpp"
#include "cst.hpp"

using std::cout;
using std::endl;
using std::setw;

OptionsType options;


int main(int argc, char * argv[])
{
  read_args(argc, argv);

  SetResources(options.memory_mb_, options.num_threads_);

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

