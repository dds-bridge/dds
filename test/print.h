/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#ifndef DTEST_PRINT_H
#define DTEST_PRINT_H

#include <string>

#include "../include/dll.h"

using namespace std;


void set_constants();

void print_PBN(const dealPBN& dl);

void print_FUT(const futureTricks& fut);

void print_TABLE(const ddTableResults& table);

void print_PAR(const parResults& par);

void print_DEALERPAR(const parResultsDealer& par);

void print_PLAY(const playTracePBN& play);

void print_TRACE(const solvedPlay& solved);

void print_double_TRACE(
  const solvedPlay& solved,
  const solvedPlay& ref);

#endif

