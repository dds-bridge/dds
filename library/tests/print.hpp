/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef DTEST_PRINT_H
#define DTEST_PRINT_H

#include <string>

#include <api/dll.h>

using namespace std;


void set_constants();

void print_PBN(const DealPBN& dl);

void print_FUT(const FutureTricks& fut);

void print_TABLE(const DdTableResults& table);

void print_PAR(const ParResults& par);

void print_DEALERPAR(const ParResultsDealer& par);

void print_PLAY(const PlayTracePBN& play);

void print_TRACE(const SolvedPlay& solved);

void print_double_TRACE(
  const SolvedPlay& solved,
  const SolvedPlay& ref);

#endif

