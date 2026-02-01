/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_PARSE_H
#define DTEST_PARSE_H

#include <api/dll.h>
#include <string>

using namespace std;


bool read_file(
  const string& fname,
  int& number,
  bool& GIBmode,
  int ** dealer_list,
  int ** vul_list,
  DealPBN ** deal_list,
  FutureTricks ** fut_list,
  DdTableResults ** table_list,
  ParResults ** par_list,
  ParResultsDealer ** dealerpar_list,
  PlayTracePBN ** play_list,
  SolvedPlay ** trace_list);

#endif

