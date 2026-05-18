/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <string>

/// @file cst.hpp
/// @brief Test configuration constants and types.
/// 
/// Defines enumerations for solver types and the global options
/// structure for test drivers.

/// Solver operation mode enumeration.
enum class Solver
{
  DTEST_SOLVER_SOLVE = 0,       ///< Solve single board
  DTEST_SOLVER_CALC = 1,        ///< Calculate DD table
  DTEST_SOLVER_PLAY = 2,        ///< Play out deal
  DTEST_SOLVER_PAR = 3,         ///< Calculate PAR score
  DTEST_SOLVER_DEALERPAR = 4,   ///< Calculate dealer PAR
  DTEST_SOLVER_SIZE = 5         ///< Number of solver modes
};

/// Global test options structure.
struct OptionsType
{
  std::string fname_;                       ///< Input file path
  Solver solver_;                           ///< Solver mode
  int num_threads_;                         ///< Number of threads to use
  int memory_mb_;                           ///< Memory allocation in MB
  bool report_slow_boards_;                 ///< Report slow-executing hands
};

