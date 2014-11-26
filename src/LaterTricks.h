/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


bool LaterTricksMIN(
  struct pos            * posPoint, 
  int                   hand, 
  int                   depth, 
  int                   target, 
  int                   trump, 
  struct localVarType   * thrp); 

bool LaterTricksMAX(
  struct pos            * posPoint, 
  int                   hand, 
  int                   depth, 
  int                   target, 
  int                   trump, 
  struct localVarType   * thrp);
