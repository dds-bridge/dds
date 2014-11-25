/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


int SolveSameBoard(
  deal          dl,
  futureTricks  * futp,
  int           hint,
  int           thrId);

int AnalyseLaterBoard(
  int           leadHand,
  moveType      * move,
  int           hint,
  int           hintDir,
  futureTricks  * futp,
  int           thrp);

