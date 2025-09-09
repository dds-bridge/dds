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
  // Create an index array and sort it deterministically by:
  // 1) weight (descending)
  // 2) suit (ascending)
  // 3) rank (descending)
  // 4) sequence (ascending)
  std::vector<int> idx(numMoves);
  for (int i = 0; i < numMoves; ++i) idx[i] = i;

  auto cmp = [&](int a, int b) {
    // higher weight first
    if (moves[a].weight != moves[b].weight) return moves[a].weight > moves[b].weight;
    if (moves[a].suit != moves[b].suit) return moves[a].suit < moves[b].suit;
    if (moves[a].rank != moves[b].rank) return moves[a].rank > moves[b].rank;
    return moves[a].sequence < moves[b].sequence;
  };

  // stable sort to preserve original relative order when comparator reports equal
  std::stable_sort(idx.begin(), idx.end(), cmp);

  std::ostringstream out;
  out << "[";
  for (int k = 0; k < numMoves; ++k) {
    int i = idx[k];
    if (k) out << ", ";
    out << "{";
    out << "\"suit\":" << moves[i].suit << ",";
    out << "\"rank\":" << moves[i].rank;
    if (include_scores) out << ",\"weight\":" << moves[i].weight;
    out << "}";
  }
  out << "]";
  return out.str();
}

bool write_json_log(const std::string& path, const std::string& json_text) {
  std::ofstream ofs(path);
  if (!ofs) return false;
  ofs << json_text;
  return ofs.good();
}
