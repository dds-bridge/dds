/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "SolveBoard.h"
#include "SolveBoard_GCD.h"


extern int noOfThreads;


int SolveInitThreadsGCD()
{
  return RETURN_NO_FAULT;
}


int SolveRunThreadsGCD(
  const int chunkSize)
{
  // This code for LLVM multi-threading on the Mac was kindly
  // contributed by Pierre Cossard.

#if (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) || defined(__MAC_OS_X_VERSION_MAX_ALLOWED))
  int thid;

  START_BLOCK_TIMER;
  if (chunkSize == 1)
  {
    dispatch_apply(static_cast<size_t>(noOfThreads),
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
      ^(size_t t)
    {
      thid = omp_get_thread_num();
      SolveChunkCommon(thid);
    });
  }
  else
  {
    dispatch_apply(static_cast<size_t>(noOfThreads),
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0),
      ^(size_t t)
    {
      thid = omp_get_thread_num();
      SolveChunkDDtableCommon(thid);
    });
  }
  END_BLOCK_TIMER;

  return RETURN_NO_FAULT;

#else
  UNUSED(chunkSize);
  return RETURN_NO_FAULT;
#endif
}
