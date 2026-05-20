/**
 * @file solve_board.hpp
 * @brief C++ interface for double dummy solver with explicit solver context.
 *
 * This header provides C++-only overloads that accept an explicit
 * SolverContext, allowing clients to manage solver state across multiple solve operations.
 *
 * @copyright (C) 2006-2014 by Bo Haglund / 2014-2018 by Bo Haglund & Soren Hein.
 * @see LICENSE and README.
 */

#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>

/**
 * @brief Solve a single deal with explicit solver context.
 *
 * C++-only overload that accepts an explicit SolverContext, allowing clients
 * to manage solver state and resources across multiple solve operations.
 * This is the preferred C++ API following DDS 3 snake_case naming conventions.
 *
 * @param ctx Solver context containing state and resources
 * @param dl Deal to solve
 * @param target Target number of tricks (-1 for maximum)
 * @param solutions Solution mode (1=one, 2=all, 3=all with ranks)
 * @param mode Solve mode (0=auto, 1-3=specific modes)
 * @param futp Output structure for future tricks
 * @return Error code (RETURN_NO_FAULT on success)
 * @see SolveBoard() for backward-compatible PascalCase version
 */
auto solve_board(
    SolverContext& ctx,
    const Deal& dl,
    int target,
    int solutions,
    int mode,
    FutureTricks* futp) -> int;

/**
 * @brief Backward-compatible PascalCase wrapper for solve_board.
 *
 * Kept for compatibility with existing C++ call sites. New C++ code should
 * prefer solve_board() to follow DDS 3 snake_case naming conventions.
 *
 * @see solve_board() for preferred snake_case API
 */
auto SolveBoard(
    SolverContext& ctx,
    const Deal& dl,
    int target,
    int solutions,
    int mode,
    FutureTricks* futp) -> int;
