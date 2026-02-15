/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds.h>
#include <solver_context/solver_context.hpp>

auto ab_search(
    Pos* pos_point,
    int target,
    int depth,
    SolverContext& ctx) -> bool;

auto ab_search_0(
    Pos* pos_point,
    int target,
    int depth,
    SolverContext& ctx) -> bool;

auto ab_search_1(
    Pos* pos_point,
    int target,
    int depth,
    SolverContext& ctx) -> bool;

auto ab_search_2(
    Pos* pos_point,
    int target,
    int depth,
    SolverContext& ctx) -> bool;

auto ab_search_3(
    Pos* pos_point,
    int target,
    int depth,
    SolverContext& ctx) -> bool;

auto make_0(
    Pos* pos_point,
    int depth,
    const MoveType* mply) -> void;

auto make_1(
    Pos* pos_point,
    int depth,
    const MoveType* mply) -> void;

auto make_2(
    Pos* pos_point,
    int depth,
    const MoveType* mply) -> void;

auto make_3(
    Pos* pos_point,
    unsigned short trick_cards[DDS_SUITS],
    int depth,
    const MoveType* mply,
    SolverContext& ctx) -> void;

// Evaluate terminal position using the provided context.
auto evaluate_with_context(
    const Pos* pos_point,
    int trump,
    SolverContext& ctx) -> EvalType;
