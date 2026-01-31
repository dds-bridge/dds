/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_COMPARE_H
#define DTEST_COMPARE_H

#include <api/dll.h>


bool compare_PBN(
  const DealPBN& dl1,
  const DealPBN& dl2);

bool compare_FUT(
  const FutureTricks& fut1,
  const FutureTricks& fut2);

bool compare_TABLE(
  const DdTableResults& table1,
  const DdTableResults& table2);

bool compare_PAR(
  const ParResults& par1,
  const ParResults& par2);

bool compare_DEALERPAR(
  const ParResultsDealer& par1,
  const ParResultsDealer& par2);

bool compare_TRACE(
  const SolvedPlay& trace1,
  const SolvedPlay& trace2);

#endif

