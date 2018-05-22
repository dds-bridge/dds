/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_INIT_H
#define DDS_INIT_H

#include "dds.h"
#include "Memory.h"


void SetDeal(ThreadData * thrp);

void SetDealTables(ThreadData * thrp);

void InitWinners(
  const deal& dl,
  pos& posPoint,
  ThreadData const * thrp);

void ResetBestMoves(ThreadData * thrp);

double ThreadMemoryUsed();

void CloseDebugFiles();

#endif
