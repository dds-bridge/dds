#ifndef DDS_TEST_UTILS_H
#define DDS_TEST_UTILS_H

#include <string>

// Forward declare moveType from project headers
struct moveType;

std::string serialize_move_list(const moveType* moves, int numMoves, bool include_scores=false);
std::string normalize_ordering(const moveType* moves, int numMoves, bool include_scores=false);
bool write_json_log(const std::string& path, const std::string& json_text);

#endif // DDS_TEST_UTILS_H
