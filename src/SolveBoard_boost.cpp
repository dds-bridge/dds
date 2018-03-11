/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Disable some boost header warnings.
#ifdef DDS_THREADS_BOOST
  #if (defined(_WIN32) && !defined(__CYGWIN__))
    #pragma warning(push)
    #pragma warning(disable: 4061 4191 4365 4571 4619 4625 4626 5026 5027 5031)
  #endif

  #include <boost/thread.hpp>
  using boost::thread;

  #include <vector>
  using namespace std;
  vector<thread *> threads;

  #ifdef _WIN32
    #pragma warning(pop)
  #endif
#endif



#include "dds.h"
#include "SolveBoard.h"
#include "SolveBoard_boost.h"


extern int noOfThreads;


int SolveInitThreadsBoost()
{
#ifdef DDS_THREADS_BOOST
  threads.resize(static_cast<unsigned>(noOfThreads));
#endif
  return RETURN_NO_FAULT;
}


int SolveRunThreadsBoost(
  const int chunkSize)
{
#ifdef DDS_THREADS_BOOST
  const unsigned noth = static_cast<unsigned>(noOfThreads);
  START_BLOCK_TIMER;
  if (chunkSize == 1)
  {
    for (unsigned k = 0; k < noth; k++)
      threads[k] = new thread(SolveChunkCommon, k);
  }
  else
  {
    for (unsigned k = 0; k < noth; k++)
      threads[k] = new thread(SolveChunkDDtableCommon, k);
  }
  END_BLOCK_TIMER;

  for (unsigned k = 0; k < noth; k++)
  {
   threads[k]->join();
   delete threads[k];
  }
  return RETURN_NO_FAULT;

#else
  UNUSED(chunkSize);
  return RETURN_NO_FAULT;

#endif
}
