/**
 * @file test_utils.hpp
 * @brief Test utility functions for heuristic sorting tests
 *
 * Provides helper functions for normalizing move orderings and initializing
 * test data structures for heuristic sorting regression and unit tests.
 */

#pragma once

#include <string>
#include "heuristic_sorting/heuristic_sorting.hpp"
#include <api/dds.h>

/**
 * @brief Normalizes move ordering for deterministic comparison in tests
 *
 * Creates a stable textual representation of moves sorted by weight (descending),
 * then by suit, rank, and sequence. Useful for comparing move orders across tests.
 *
 * @param moves Array of moves to normalize
 * @param num_moves Number of moves in the array
 * @param include_scores If true, include weight values in output
 * @return JSON-like string representation of sorted moves
 */
std::string normalize_ordering(const MoveType* moves, int num_moves, bool include_scores = false);

/**
 * @brief Initialize relRanks table and TrackType for testing
 *
 * Helper function for fuzz tests and unit tests that sets up the relRanks lookup
 * table and track structure based on a given position. Can optionally simulate
 * a mid-trick state by providing cards already played.
 *
 * @param tpos The position to initialize from
 * @param rel_table Pointer to RelRanksType array (size 8192 assumed)
 * @param track_p Pointer to TrackType structure to initialize
 * @param cards_played Number of cards already played (0..4), default 0
 * @param played_moves Array of played moves in play order (first to last), optional
 * @param lead_hand Absolute hand that led the trick (0..3), default 0
 * @param trump Trump suit or DDS_NOTRUMP, default DDS_NOTRUMP
 */
void init_rel_and_track(
    const Pos& tpos,
    RelRanksType* rel_table /* size 8192 assumed */,
    TrackType* track_p,
    int cards_played = 0,
    const MoveType* played_moves = nullptr,
    int lead_hand = 0,
    int trump = DDS_NOTRUMP);
