/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_LATERTRICKS_H
#define DDS_LATERTRICKS_H

#include "dds.h"
#include "Memory.h"


bool LaterTricksMIN(
  pos * posPoint,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  ThreadData const * thrp);

bool LaterTricksMAX(
  pos * posPoint,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  ThreadData const * thrp);

#endif
