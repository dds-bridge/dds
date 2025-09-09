#ifndef DDS_TEST_UTILS_H
#define DDS_TEST_UTILS_H

#include <string>
#include "heuristic_sorting/heuristic_sorting.h"
#include "dds/dds.h"

std::string serialize_move_list(const moveType* moves, int numMoves, bool include_scores=false);
std::string normalize_ordering(const moveType* moves, int numMoves, bool include_scores=false);
bool write_json_log(const std::string& path, const std::string& json_text);

// Initialize relRanks table and trackType based on a given pos (used by fuzz tests)
void init_rel_and_track(const pos& tpos, relRanksType* relTable /* size 8192 assumed */, trackType* trackp);

#endif // DDS_TEST_UTILS_H
