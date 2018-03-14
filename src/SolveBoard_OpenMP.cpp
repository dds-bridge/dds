/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "SolveBoard.h"
#include "SolveBoard_OpenMP.h"


int SolveInitThreadsOpenMP()
{
  /* Added after suggestion by Dirk Willecke. */
#ifdef _OPENMP
  if (omp_get_dynamic())
    omp_set_dynamic(0);

  omp_set_num_threads(noOfThreads);
#endif

  return RETURN_NO_FAULT;
}


int SolveRunThreadsOpenMP(
  const int chunkSize)
{
#ifdef _OPENMP
  int thid;

  START_BLOCK_TIMER;
  if (chunkSize == 1)
  {
    #pragma omp parallel default(none) private(thid)
    {
      #pragma omp for schedule(dynamic)
      // TODO
      for (int k = 0; k < 999; k++)
      {
        thid = omp_get_thread_num();
        SolveChunkCommon(thid);
      }
    }
  }
  else
  {
    #pragma omp parallel default(none) private(thid)
    {
      #pragma omp for schedule(dynamic)
      for (int k = 0; k < 999; k++)
      {
      // TODO
        thid = omp_get_thread_num();
        SolveChunkDDtableCommon(thid);
      }
    }
  }
  END_BLOCK_TIMER;

  return RETURN_NO_FAULT;

#else
  UNUSED(chunkSize);
  return RETURN_NO_FAULT;
#endif
}
