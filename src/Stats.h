/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef DDS_STATSH
#define DDS_STATSH

void InitTimer();

void SetTimerName(char * name);

void StartTimer();

void EndTimer();

void PrintTimer();

void InitTimerList();

void StartTimerNo(int n);

void EndTimerNo(int n);

void EndTimerNoAndComp(int n, int pred);

void PrintTimerList();

void InitCounter();

void PrintCounter();

#endif
