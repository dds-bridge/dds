/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVERIF_H
#define DDS_SOLVERIF_H


int SolveBoardInternal(
  struct localVarType * thrp,
  deal& dl,
  int target,
  int solutions,
  int mode,
  futureTricks * futp);

int SolveSameBoard(
  struct localVarType * thrp,
  deal dl,
  futureTricks * futp,
  int hint);

int AnalyseLaterBoard(
  struct localVarType * thrp,
  int leadHand,
  moveType * move,
  int hint,
  int hintDir,
  futureTricks * futp);

#endif
