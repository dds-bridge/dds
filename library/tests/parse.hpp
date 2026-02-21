/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>
#include <string>

bool read_file(
    const std::string& fname,
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

