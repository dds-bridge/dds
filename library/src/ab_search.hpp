/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds_data_types.hpp>
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

auto undo_0(
    Pos* pos_point,
    int depth,
    const MoveType& mply,
    SolverContext& ctx) -> void;

auto undo_1(
    Pos* pos_point,
    int depth,
    const MoveType& mply) -> void;

auto undo_2(
    Pos* pos_point,
    int depth,
    const MoveType& mply) -> void;

auto undo_3(
    Pos* pos_point,
    int depth,
    const MoveType& mply) -> void;

/// Attempt the ab_search_0 transposition-table lookup.
/// On a hit, writes win_ranks / best_move_tt, sets score_flag, and returns true.
auto apply_ab_tt_lookup(
    Pos* pos_point,
    int target,
    int depth,
    int tricks,
    int hand,
    SolverContext& ctx,
    bool& score_flag) -> bool;

// Evaluate terminal position using the provided context.
auto evaluate_with_context(
    const Pos* pos_point,
    int trump,
    SolverContext& ctx) -> EvalType;
