/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_INIT_H
#define DDS_INIT_H


void SetDeal(
  struct ThreadData * thrp);

void SetDealTables(
  struct ThreadData * thrp);

void InitWinners(
  deal * dl,
  struct pos * posPoint,
  struct ThreadData * thrp);

void ResetBestMoves(
  struct ThreadData * thrp);

double ThreadMemoryUsed();

void CloseDebugFiles();

// Used by SH for stand-alone mode.
void DDSidentify(char * s);

#endif
