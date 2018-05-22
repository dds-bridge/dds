/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_COMPARE_H
#define DTEST_COMPARE_H

#include "../include/dll.h"


bool compare_PBN(
  const dealPBN& dl1,
  const dealPBN& dl2);

bool compare_FUT(
  const futureTricks& fut1,
  const futureTricks& fut2);

bool compare_TABLE(
  const ddTableResults& table1,
  const ddTableResults& table2);

bool compare_PAR(
  const parResults& par1,
  const parResults& par2);

bool compare_DEALERPAR(
  const parResultsDealer& par1,
  const parResultsDealer& par2);

bool compare_TRACE(
  const solvedPlay& trace1,
  const solvedPlay& trace2);

#endif

