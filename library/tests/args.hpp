/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// @file args.hpp
/// @brief Command-line argument parsing for test utilities.
/// 
/// Provides functions to parse and validate command-line options
/// for test driver programs (dtest, itest). Options include:
/// - Input file specification
/// - Solver type selection (solve, calc, play, par, dealer_par)
/// - Number of threads
/// - Memory allocation
/// - Slow board reporting

/// Print usage information.
/// @param base Command name for usage message
void usage(
    const char base[]);

/// Print current option values.
void print_options();

/// Parse command-line arguments into global options.
/// @param argc Argument count from main()
/// @param argv Argument vector from main()
void read_args(
    int argc,
    char * argv[]);

