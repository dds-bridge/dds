/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// Main test driver function.
///
/// Entry point for test execution. Configures test parameters from command-line
/// arguments and delegates to specific test loop implementations (solve, calc,
/// par, dealerpar, play).
///
/// @param argc Number of command-line arguments
/// @param argv Array of command-line argument strings
/// @return Exit status code (0 for success, non-zero for failure)
int real_main([[maybe_unused]] int argc, [[maybe_unused]] char * argv[]);

