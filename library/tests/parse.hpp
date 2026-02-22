/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>
#include <string>

/// @file parse.hpp
/// @brief Input file parsing for test data in PBN and GIB formats.
/// 
/// Reads test hand data from files, supporting both PBN (Portable
/// Bridge Notation) and dealer (.gib) formats.

/// Read test data from file.
/// Parses deals, expected results, and play traces from input.
/// @param fname Path to input file
/// @param number Number of deals read (output)
/// @param GIBmode true if file is .gib format, false for PBN
/// @param dealer_list First player (output)
/// @param vul_list Vulnerability (output)
/// @param deal_list Board deals in PBN (output)
/// @param fut_list Expected future tricks (output)
/// @param table_list Expected DD tables (output)
/// @param par_list Expected PAR scores (output)
/// @param dealerpar_list Expected dealer PAR (output)
/// @param play_list Expected play traces (output)
/// @param trace_list Expected trace results (output)
/// @return true if parsing succeeded
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

