/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iostream>
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
  #include <unistd.h>
#endif

#include <api/dll.h>
#include "testcommon.hpp"
#include "args.hpp"
#include "cst.hpp"

using std::cout;
using std::endl;

OptionsType options;


int main(int argc, char * argv[])
{
  read_args(argc, argv);

  SetResources(options.memory_mb_, 0);

  DDSInfo info;
  GetDDSInfo(&info);
  cout << info.systemString << endl;
  if (options.num_threads_ == 0)
    cout << "dtest worker threads: auto\n";
  else
    cout << "dtest worker threads: " << options.num_threads_ << "\n";

  real_main(argc, argv);

  // Restore normal termination so destructors / atexit handlers run.
  exit(0);
}
