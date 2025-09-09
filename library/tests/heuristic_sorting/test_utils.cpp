#include "test_utils.h"
#include <fstream>
#include <sstream>
#include <cstring>

#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"

// Minimal serializer: produce a simple JSON array of moves
std::string serialize_move_list(const moveType* moves, int numMoves, bool include_scores) {
  std::ostringstream out;
  out << "[";
  for (int i = 0; i < numMoves; ++i) {
    if (i) out << ", ";
    out << "{\"suit\":" << moves[i].suit << ",\"rank\":" << moves[i].rank;
    if (include_scores) out << ",\"weight\":" << moves[i].weight;
    out << "}";
  }
  out << "]";
  return out.str();
}

// Normalization: stable textual representation (same as serialize for now)
std::string normalize_ordering(const moveType* moves, int numMoves, bool include_scores) {
  return serialize_move_list(moves, numMoves, include_scores);
}

bool write_json_log(const std::string& path, const std::string& json_text) {
  std::ofstream ofs(path);
  if (!ofs) return false;
  ofs << json_text;
  return ofs.good();
}
