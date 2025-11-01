/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_LATERTRICKS_H
#define DDS_LATERTRICKS_H

#include "dds/dds.h"
#include "system/Memory.h"
#include <memory>


bool LaterTricksMIN(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  const std::shared_ptr<ThreadData>& thrp);

bool LaterTricksMAX(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  const std::shared_ptr<ThreadData>& thrp);

#endif
