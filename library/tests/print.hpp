/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>

/// @file print.hpp
/// @brief Result formatting and display utilities.
/// 
/// Provides functions to format and print test results in human-readable
/// form, supporting all DDS result types.

/// Initialize print constants (column widths, etc).
void set_constants();

/// Print a board deal in PBN format.
void print_PBN(const DealPBN& dl);

/// Print future tricks results.
void print_FUT(const FutureTricks& fut);

/// Print DD table results.
void print_TABLE(const DdTableResults& table);

/// Print PAR score results.
void print_PAR(const ParResults& par);

/// Print dealer PAR results.
void print_DEALERPAR(const ParResultsDealer& par);

/// Print play trace.
void print_PLAY(const PlayTracePBN& play);

/// Print solved play trace with results.
void print_TRACE(const SolvedPlay& solved);

/// Print comparison of two play traces side-by-side.
void print_double_TRACE(
    const SolvedPlay& solved,
    const SolvedPlay& ref);

