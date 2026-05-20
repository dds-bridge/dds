/**
 * @file calc_dd_table.hpp
 * @brief C++ interface for double dummy table calculation with explicit solver context.
 *
 * This header provides C++ overloads for DD table calculation that accept explicit
 * SolverContext, allowing clients to manage solver state and transposition tables
 * across multiple table calculations.
 *
 * @copyright (C) 2006-2014 by Bo Haglund / 2014-2018 by Bo Haglund & Soren Hein.
 * @see LICENSE and README.
 */

#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>

// Naming note: New C++ APIs in DDS 3 use snake_case (calc_dd_table).
// Legacy C API names remain PascalCase (CalcDDtable).

/**
 * @brief Calculate double dummy table for a deal (creates temporary context).
 *
 * Computes the double dummy table showing maximum tricks for each strain
 * and declarer. This version creates a temporary SolverContext internally.
 *
 * @param table_deal Deal represented as card holdings for each hand
 * @param table_results Output: double dummy table results (5 strains x 4 declarers)
 * @return Error code (RETURN_NO_FAULT on success)
 *
 * @note For repeated calculations, prefer calc_dd_table(ctx, ...) to reuse context
 */
auto calc_dd_table(
    const DdTableDeal& table_deal,
    DdTableResults* table_results) -> int;

/**
 * @brief Calculate double dummy table with explicit solver context.
 *
 * C++ overload that accepts an explicit SolverContext, allowing clients
 * to reuse solver state and allocated resources across multiple table
 * calculations.
 *
 * @param ctx Solver context containing state and allocated solver resources
 * @param table_deal Deal represented as card holdings for each hand
 * @param table_results Output: double dummy table results (5 strains x 4 declarers)
 * @return Error code (RETURN_NO_FAULT on success)
 *
 * @note Reusing the same context avoids per-call setup and allocation costs.
 *       DD-table calculations iterate all strains, so TT entry reuse across
 *       calls may be limited depending on call patterns.
 */
auto calc_dd_table(
    SolverContext& ctx,
    const DdTableDeal& table_deal,
    DdTableResults* table_results) -> int;

/**
 * @brief Calculate double dummy table from PBN deal (creates temporary context).
 *
 * Convenience overload that accepts PBN format deal string and delegates
 * to binary format calculation after conversion.
 *
 * @param table_deal_pbn Deal in PBN format
 * @param table_results Output: double dummy table results
 * @return Error code (RETURN_NO_FAULT on success, RETURN_PBN_FAULT on parse error)
 */
auto calc_dd_table_pbn(
    const DdTableDealPBN& table_deal_pbn,
    DdTableResults* table_results) -> int;

/**
 * @brief Calculate double dummy table from PBN deal with explicit context.
 *
 * Context-aware overload accepting PBN format. Converts and delegates to
 * binary format calculation with context reuse.
 *
 * @param ctx Solver context for resource management
 * @param table_deal_pbn Deal in PBN format
 * @param table_results Output: double dummy table results
 * @return Error code (RETURN_NO_FAULT on success, RETURN_PBN_FAULT on parse error)
 */
auto calc_dd_table_pbn(
    SolverContext& ctx,
    const DdTableDealPBN& table_deal_pbn,
    DdTableResults* table_results) -> int;
