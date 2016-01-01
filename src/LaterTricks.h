/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_LATERTRICKS_H
#define DDS_LATERTRICKS_H


bool LaterTricksMIN(
  struct pos * posPoint,
  int hand,
  int depth,
  int target,
  int trump,
  struct localVarType * thrp);

bool LaterTricksMAX(
  struct pos * posPoint,
  int hand,
  int depth,
  int target,
  int trump,
  struct localVarType * thrp);

#endif
