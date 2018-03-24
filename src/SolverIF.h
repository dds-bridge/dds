/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVERIF_H
#define DDS_SOLVERIF_H

#include "Memory.h"


int SolveBoardInternal(
  ThreadData * thrp,
  deal& dl,
  int target,
  int solutions,
  int mode,
  futureTricks * futp);

int SolveSameBoard(
  ThreadData * thrp,
  deal dl,
  futureTricks * futp,
  int hint);

int AnalyseLaterBoard(
  ThreadData * thrp,
  int leadHand,
  moveType * move,
  int hint,
  int hintDir,
  futureTricks * futp);

#endif
