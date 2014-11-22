/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


int QuickTricks(
  struct pos 		* posPoint, 
  int 			hand, 
  int 			depth, 
  int 			target, 
  int 			trump, 
  bool 			* result, 
  struct localVarType 	* thrp);

bool QuickTricksSecondHand(
  struct pos		* posPoint,
  int			hand,
  int			depth,
  int			target,
  int			trump,
  struct localVarType	* thrp);

