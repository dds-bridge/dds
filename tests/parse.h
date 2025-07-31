/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_PARSE_H
#define DTEST_PARSE_H

#include "../include/dll.h"

using namespace std;


bool read_file(
  const string& fname,
  int& number,
  bool& GIBmode,
  int ** dealer_list,
  int ** vul_list,
  dealPBN ** deal_list,
  futureTricks ** fut_list,
  ddTableResults ** table_list,
  parResults ** par_list,
  parResultsDealer ** dealerpar_list,
  playTracePBN ** play_list,
  solvedPlay ** trace_list);

#endif

