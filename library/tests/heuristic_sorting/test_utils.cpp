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

// Initialize relRanks table and trackType based on a given pos (used by fuzz tests)
void init_rel_and_track(const pos& tpos, relRanksType* relTable /* size 8192 assumed */, trackType* trackp) {
  // zero track
  if (trackp) {
    std::memset(trackp, 0, sizeof(*trackp));
    for (int p = 0; p < DDS_HANDS; ++p) trackp->high[p] = 0;
    for (int s = 0; s < DDS_SUITS; ++s) trackp->removedRanks[s] = 0;
    for (int p = 0; p < DDS_HANDS; ++p) {
      trackp->move[p].suit = 0;
      trackp->move[p].rank = 0;
      trackp->move[p].sequence = 0;
    }
  }

  if (!relTable) return;

  // Initialize relTable[0]
  for (int s = 0; s < DDS_SUITS; s++) {
    for (int ord = 1; ord <= 13; ord++) {
      relTable[0].absRank[ord][s].hand = -1;
      relTable[0].absRank[ord][s].rank = 0;
    }
  }

  // Build handLookup from current deal
  int handLookup[DDS_SUITS][15];
  for (int s = 0; s < DDS_SUITS; s++) {
    for (int r = 14; r >= 2; r--) {
      handLookup[s][r] = 0;
      for (int h = 0; h < DDS_HANDS; h++) {
        if (tpos.rankInSuit[h][s] & bitMapRank[r]) {
          handLookup[s][r] = h;
          break;
        }
      }
    }
  }

  unsigned int topBitRank = 1;
  unsigned int topBitNo = 2;
  for (unsigned int aggr = 1; aggr < 8192; aggr++) {
    if (aggr >= (topBitRank << 1)) {
      topBitRank <<= 1;
      topBitNo++;
    }

    relTable[aggr] = relTable[aggr ^ topBitRank];
    relRanksType * relp = &relTable[aggr];

    int weight = counttable[aggr];
    for (int c = weight; c >= 2; c--) {
      for (int s = 0; s < DDS_SUITS; s++) {
        relp->absRank[c][s].hand = relp->absRank[c - 1][s].hand;
        relp->absRank[c][s].rank = relp->absRank[c - 1][s].rank;
      }
    }
    for (int s = 0; s < DDS_SUITS; s++) {
      relp->absRank[1][s].hand = static_cast<signed char>(handLookup[s][topBitNo]);
      relp->absRank[1][s].rank = static_cast<char>(topBitNo);
    }
  }
}
