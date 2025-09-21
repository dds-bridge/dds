#ifndef DDS_TEST_UTILS_H
#define DDS_TEST_UTILS_H

#include <string>
#include "heuristic_sorting/heuristic_sorting.h"
#include "dds/dds.h"

std::string normalize_ordering(const moveType* moves, int numMoves, bool include_scores=false);

// Initialize relRanks table and trackType based on a given pos (used by fuzz tests)
// Optional: simulate a mid-trick state by providing the number of cards already
// played (0..4) and the array of played moves in play order (first to last).
// leadHand is the absolute hand that led the trick (0..3). trump may be
// provided to let the helper decide current winning card when trumps exist.
void init_rel_and_track(
	const pos& tpos,
	relRanksType* relTable /* size 8192 assumed */,
	trackType* trackp,
	int cardsPlayed = 0,
	const moveType* playedMoves = nullptr,
	int leadHand = 0,
	int trump = DDS_NOTRUMP);

#endif // DDS_TEST_UTILS_H
