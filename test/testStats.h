/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef DDS_STATSH
#define DDS_STATSH

void TestInitTimer();

void TestSetTimerName(const char * name);

void TestStartTimer();

void TestEndTimer();

void TestPrintTimer();

void TestInitTimerList();

void TestStartTimerNo(int n);

void TestEndTimerNo(int n);

void TestEndTimerNoAndComp(int n, int pred);

void TestPrintTimerList();

void TestInitCounter();

void TestPrintCounter();

#endif
