/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#include <iomanip>
#include <cstdlib>

#include "../include/dll.h"
#include "testcommon.h"

using namespace std;


int main(int argc, char * argv[])
{
  if (argc >= 5)
    SetThreading(threadingCode(argv[4]));

  int nthreads = 0;
  if (argc >= 4)
    nthreads = atoi(argv[3]);
  SetMaxThreads(nthreads);

  SetResources(1800, nthreads);

  DDSInfo info;
  GetDDSInfo(&info);
  cout << info.systemString << endl;

  realMain(argc, argv);

  exit(0);
}
