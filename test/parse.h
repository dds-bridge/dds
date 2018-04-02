/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DTEST_PARSE_H
#define DTEST_PARSE_H

bool read_file(
  char const * fname,
  int * number,
  int ** dealer_list,
  int ** vul_list,
  struct dealPBN ** deal_list,
  struct futureTricks ** fut_list,
  struct ddTableResults ** table_list,
  struct parResults ** par_list,
  struct parResultsDealer ** dealerpar_list,
  struct playTracePBN ** play_list,
  struct solvedPlay ** trace_list,
  bool& GIBmode);

#endif

