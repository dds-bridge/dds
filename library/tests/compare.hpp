/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>

/// @file compare.hpp
/// @brief Result comparison utilities for test validation.
/// 
/// Provides functions to compare expected and actual solver results
/// across all result types: DD tables, future tricks, PAR scores,
/// dealer PAR, and play traces.

/// Compare two PBN deal representations.
/// @param dl1 First deal
/// @param dl2 Second deal
/// @return true if deals are identical
bool compare_PBN(
    const DealPBN& dl1,
    const DealPBN& dl2);

/// Compare two future tricks results.
bool compare_FUT(
    const FutureTricks& fut1,
    const FutureTricks& fut2);

/// Compare two DD table results.
bool compare_TABLE(
    const DdTableResults& table1,
    const DdTableResults& table2);

/// Compare two PAR score results.
bool compare_PAR(
    const ParResults& par1,
    const ParResults& par2);

/// Compare two dealer PAR results.
bool compare_DEALERPAR(
    const ParResultsDealer& par1,
    const ParResultsDealer& par2);

/// Compare two play traces for equivalence.
bool compare_TRACE(
    const SolvedPlay& trace1,
    const SolvedPlay& trace2);
