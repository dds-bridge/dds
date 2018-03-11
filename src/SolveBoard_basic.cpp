/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "SolveBoard.h"
#include "SolveBoard_basic.h"


int SolveInitThreadsNone()
{
  return RETURN_NO_FAULT;
}


int SolveRunThreadsNone(
  const int chunkSize)
{
  START_BLOCK_TIMER;

  if (chunkSize == 1)
    SolveChunkCommon(0);
  else
    SolveChunkDDtableCommon(0);
  END_BLOCK_TIMER;

  return RETURN_NO_FAULT;
}

