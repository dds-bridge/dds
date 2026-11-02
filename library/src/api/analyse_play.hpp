/**
 * @file analyse_play.hpp
 * @brief C++ interface for play analysis with explicit solver context.
 *
 * This header provides the C++-only, context-aware entry point for play
 * analysis, allowing clients to manage solver state across multiple
 * analyse_play calls.
 *
 * Naming note: this mirrors the legacy AnalysePlayBin/AnalysePlayPBN API
 * family, so "analyse" (British spelling) is kept deliberately here,
 * matching that existing name rather than switching to "analyze".
 *
 * @copyright (C) 2006-2014 by Bo Haglund / 2014-2018 by Bo Haglund & Soren Hein.
 * @see LICENSE and README.
 */

#pragma once

#include <api/dds_data_types.hpp>
#include <solver_context/solver_context.hpp>

/**
 * @brief Analyze a sequence of played cards (binary format) with explicit solver context.
 *
 * C++-only overload that accepts an explicit SolverContext, allowing clients
 * to manage solver state and resources across multiple analyse_play calls.
 * This is the preferred C++ API following DDS 3 snake_case naming conventions.
 *
 * @param ctx Solver context containing state and resources
 * @param dl Deal to analyze
 * @param play Sequence of played cards (binary format)
 * @param solvedp Output structure for the solved play
 * @return Error code (RETURN_NO_FAULT on success)
 * @see AnalysePlayBin() for the backward-compatible PascalCase legacy version
 */
auto analyse_play(
    SolverContext& ctx,
    const Deal& dl,
    const PlayTraceBin& play,
    SolvedPlay* solvedp) -> int;
