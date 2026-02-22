/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <string>

using std::string;

/// @file cst.hpp
/// @brief Test configuration constants and types.
/// 
/// Defines enumerations for solver types, threading models,
/// and the global options structure for test drivers.

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

/// Threading model selection.
enum class Threading
{
  DTEST_THREADING_NONE = 0,         ///< No multi-threading
  DTEST_THREADING_WINAPI = 1,       ///< Windows API threads
  DTEST_THREADING_OPENMP = 2,       ///< OpenMP parallelization
  DTEST_THREADING_GCD = 3,          ///< Grand Central Dispatch (macOS)
  DTEST_THREADING_BOOST = 4,        ///< Boost threads
  DTEST_THREADING_STL = 5,          ///< C++ STL threads
  DTEST_THREADING_TBB = 6,          ///< Intel TBB
  DTEST_THREADING_STLIMPL = 7,      ///< STL implementation
  DTEST_THREADING_PPLIMPL = 8,      ///< Windows PPL
  DTEST_THREADING_DEFAULT = 9,      ///< Default for platform
  DTEST_THREADING_SIZE = 10         ///< Number of threading modes
};

/// Global test options structure.
struct OptionsType
{
  std::string fname_;                       ///< Input file path
  Solver solver_;                           ///< Solver mode
  Threading threading_;                     ///< Threading model
  int num_threads_;                         ///< Number of threads to use
  int memory_mb_;                           ///< Memory allocation in MB
  bool report_slow_boards_;                 ///< Report slow-executing hands
};

