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
  deal dl,
  futureTricks * futp,
  int hint,
  int thrId);

int AnalyseLaterBoard(
  int leadHand,
  moveType * move,
  int hint,
  int hintDir,
  futureTricks * futp,
  int thrp);

#endif
