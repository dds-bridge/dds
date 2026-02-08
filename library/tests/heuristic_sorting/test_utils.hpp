#ifndef LIBRARY_TESTS_HEURISTIC_SORTING_TEST_UTILS_HPP_
#define LIBRARY_TESTS_HEURISTIC_SORTING_TEST_UTILS_HPP_

#include <string>
#include "heuristic_sorting/heuristic_sorting.hpp"
#include <api/dds.h>

std::string normalize_ordering(const MoveType* moves, int numMoves, bool include_scores = false);

// Initialize relRanks table and TrackType based on a given Pos (used by fuzz tests)
// Optional: simulate a mid-trick state by providing the number of cards already
// played (0..4) and the array of played moves in play order (first to last).
// lead_hand is the absolute hand that led the trick (0..3). trump may be
// provided to let the helper decide current winning card when trumps exist.
void init_rel_and_track(
	const Pos& tpos,
	RelRanksType* relTable /* size 8192 assumed */,
	TrackType* trackp,
	int cardsPlayed = 0,
	const MoveType* playedMoves = nullptr,
	int lead_hand = 0,
	int trump = DDS_NOTRUMP);

#endif // LIBRARY_TESTS_HEURISTIC_SORTING_TEST_UTILS_HPP_
