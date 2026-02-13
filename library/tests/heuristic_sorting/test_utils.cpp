/**
 * @file test_utils.cpp
 * @brief Implementation of test utility functions for heuristic sorting tests
 */

#include "test_utils.hpp"
#include <sstream>
#include <cstring>
#include <vector>
#include <algorithm>

#include "heuristic_sorting/heuristic_sorting.hpp"
#include "heuristic_sorting/internal.hpp"
#include <lookup_tables/lookup_tables.hpp>

// Normalization: stable textual representation (same as serialize for now)
std::string normalize_ordering(const MoveType* moves, int numMoves, bool include_scores) {
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

// Initialize relRanks table and TrackType based on a given Pos (used by fuzz tests)
void init_rel_and_track(const Pos& tpos, RelRanksType* relTable /* size 8192 assumed */, TrackType* trackp,
    int cardsPlayed, const MoveType* playedMoves, int lead_hand, int trump) {
  // zero track and set sane defaults
  if (trackp) {
    std::memset(trackp, 0, sizeof(*trackp));
    trackp->lead_hand = lead_hand;
    trackp->lead_suit = 0;
    for (int p = 0; p < DDS_HANDS; ++p) {
      trackp->high[p] = 0;
      trackp->play_suits[p] = 0;
      trackp->play_ranks[p] = 0;
      trackp->move[p].suit = 0;
      trackp->move[p].rank = 0;
      trackp->move[p].sequence = 0;
    }
    for (int s = 0; s < DDS_SUITS; ++s) {
      trackp->removed_ranks[s] = 0; // will OR in present cards below
      for (int h = 0; h < DDS_HANDS; ++h)
        trackp->lowest_win[h][s] = 0;
    }
    // default trickData
    for (int s = 0; s < DDS_SUITS; ++s) trackp->trick_data.play_count[s] = 0;
    trackp->trick_data.best_rank = 0;
    trackp->trick_data.best_suit = 0;
    trackp->trick_data.best_sequence = 0;
    trackp->trick_data.rel_winner = 0;
    trackp->trick_data.next_lead_hand = lead_hand;
  }

  if (!relTable) return;

  // Work on a mutable copy of Pos so we can simulate cards removed by plays
  Pos localPos = tpos;
  if (cardsPlayed > 0 && playedMoves) {
    // playedMoves are in play order starting from lead_hand (absolute)
    for (int i = 0; i < cardsPlayed; ++i) {
      const MoveType &m = playedMoves[i];
      int absHand = (lead_hand + i) % 4;
      if (m.rank > 0 && m.rank < 16) {
        unsigned short mask = bit_map_rank[m.rank];
        // remove the card from localPos
        localPos.rank_in_suit[absHand][m.suit] &= static_cast<unsigned short>(~mask);
        // update aggregate and lengths
        localPos.aggr[m.suit] &= static_cast<unsigned short>(~mask);
        if (localPos.length[absHand][m.suit] > 0) localPos.length[absHand][m.suit]--;
        if (localPos.hand_dist[absHand] > 0) localPos.hand_dist[absHand]--;
      }
    }

    // Recompute winner/second_best conservatively for each suit based on remaining cards
    for (int s = 0; s < DDS_SUITS; ++s) {
      localPos.winner[s].rank = 0; localPos.winner[s].hand = 0;
      localPos.second_best[s].rank = 0; localPos.second_best[s].hand = 0;
      for (int h = 0; h < DDS_HANDS; ++h) {
        unsigned short ris = localPos.rank_in_suit[h][s];
        if (!ris) continue;
        for (int r = 13; r >= 1; --r) {
          if (ris & (1u << r)) {
            if (r > localPos.winner[s].rank) {
              localPos.second_best[s] = localPos.winner[s];
              localPos.winner[s].rank = r;
              localPos.winner[s].hand = h;
            } else if (r > localPos.second_best[s].rank) {
              localPos.second_best[s].rank = r;
              localPos.second_best[s].hand = h;
            }
            break;
          }
        }
      }
    }
  }

  // Initialize relTable[0]
  for (int s = 0; s < DDS_SUITS; s++) {
    for (int ord = 1; ord <= 13; ord++) {
      relTable[0].abs_rank[ord][s].hand = -1;
      relTable[0].abs_rank[ord][s].rank = 0;
    }
  }

  // Build handLookup from current Deal (use localPos which may have had cards removed)
  int handLookup[DDS_SUITS][15];
  for (int s = 0; s < DDS_SUITS; s++) {
    for (int r = 14; r >= 2; r--) {
      handLookup[s][r] = 0;
      for (int h = 0; h < DDS_HANDS; h++) {
        if (localPos.rank_in_suit[h][s] & bit_map_rank[r]) {
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
    RelRanksType * relp = &relTable[aggr];

  int weight = count_table[aggr];
    for (int c = weight; c >= 2; c--) {
      for (int s = 0; s < DDS_SUITS; s++) {
        relp->abs_rank[c][s].hand = relp->abs_rank[c - 1][s].hand;
        relp->abs_rank[c][s].rank = relp->abs_rank[c - 1][s].rank;
      }
    }
    for (int s = 0; s < DDS_SUITS; s++) {
      relp->abs_rank[1][s].hand = static_cast<signed char>(handLookup[s][topBitNo]);
      relp->abs_rank[1][s].rank = static_cast<char>(topBitNo);
    }
  }

  // If requested, simulate cards already played in the current trick.
  // playedMoves is expected to be an array of length >= cardsPlayed with
  // moves in play order (first played -> last played). We will set
  // trackp->play_suits/play_ranks/move/high and update removed_ranks and
  // trickData accordingly; also set lead_suit from the first played card.
  if (trackp && cardsPlayed > 0 && playedMoves) {
    if (cardsPlayed > DDS_HANDS) cardsPlayed = DDS_HANDS;
    int relIndex = 0;
    for (int i = 0; i < cardsPlayed; ++i) {
      const MoveType& m = playedMoves[i];
      // relative index in trick: 0..cardsPlayed-1
      relIndex = i;
      trackp->play_suits[relIndex] = m.suit;
      trackp->play_ranks[relIndex] = m.rank;
      trackp->move[relIndex].suit = m.suit;
      trackp->move[relIndex].rank = m.rank;
      trackp->move[relIndex].sequence = m.sequence;

      // maintain removedRanks: mark that the card has been played
      if (m.rank > 0 && m.rank < 16)
        trackp->removed_ranks[m.suit] |= bit_map_rank[m.rank];

      // update high[]: who currently wins among the played cards
      if (relIndex == 0) {
        trackp->high[0] = 0;
        // lead_suit is the first card's suit
        trackp->lead_suit = m.suit;
      } else {
        // compare with previous winning card
  ExtCard prev = trackp->move[trackp->high[relIndex - 1]];
        bool newIsWinning = false;
        if (m.suit == prev.suit) {
          if (m.rank > prev.rank) newIsWinning = true;
        } else if (m.suit == trump) {
          // trump beats non-trump
          if (trump != DDS_NOTRUMP) newIsWinning = true;
        }
        if (newIsWinning) trackp->high[relIndex] = relIndex;
        else trackp->high[relIndex] = trackp->high[relIndex - 1];
      }
    }

    // Fill remaining high[] entries (for unplayed positions) with last known
    for (int p = cardsPlayed; p < DDS_HANDS; ++p) trackp->high[p] = trackp->high[cardsPlayed - 1];

    // Update trickData play counts
    for (int p = 0; p < cardsPlayed; ++p)
      trackp->trick_data.play_count[ trackp->play_suits[p] ]++;

    // Update trickData best values from the last play
    trackp->trick_data.best_rank = trackp->move[cardsPlayed - 1].rank;
    trackp->trick_data.best_suit = trackp->move[cardsPlayed - 1].suit;
    trackp->trick_data.best_sequence = trackp->move[cardsPlayed - 1].sequence;
    trackp->trick_data.rel_winner = trackp->high[cardsPlayed - 1];
    // next_lead_hand if trick completes would be based on high[cardsPlayed-1]
    trackp->trick_data.next_lead_hand = (trackp->lead_hand + trackp->trick_data.rel_winner) % 4;
  }

  // Populate lowest_win: compute a more precise minimal winning rank for
  // each relative hand (relh) and suit (s) given the current trick state.
  if (trackp) {
    // helpers to find ranks in a bitmask
    auto find_smallest_rank = [](unsigned short ris) -> int {
      for (int r = 1; r <= 13; ++r) if (ris & (1u << r)) return r;
      return 0;
    };
    auto find_smallest_rank_greater = [](unsigned short ris, int thr) -> int {
      for (int r = thr + 1; r <= 13; ++r) if (ris & (1u << r)) return r;
      return 0;
    };

    // current best on trick (if any)
    bool hasCurrentBest = (cardsPlayed > 0);
    int curBestSuit = -1;
    int curBestRank = 0;
    if (hasCurrentBest) {
      int lastRel = cardsPlayed - 1;
      int rel_winner = trackp->high[lastRel];
      ExtCard best = trackp->move[rel_winner];
      curBestSuit = best.suit;
      curBestRank = best.rank;
    }

    for (int relh = 0; relh < DDS_HANDS; ++relh) {
      for (int s = 0; s < DDS_SUITS; ++s) {
        trackp->lowest_win[relh][s] = 0;
        // If this relative hand already played in this trick, skip
        if (cardsPlayed > 0 && relh < cardsPlayed) {
          trackp->lowest_win[relh][s] = 0;
          continue;
        }

        int absHand = (trackp->lead_hand + relh) % 4;
        unsigned short ris = localPos.rank_in_suit[absHand][s];
        if (!ris) { trackp->lowest_win[relh][s] = 0; continue; }

        // If there is no current best (lead not played), use smallest rank
        if (!hasCurrentBest) {
          trackp->lowest_win[relh][s] = find_smallest_rank(ris);
          continue;
        }

        // If candidate suit equals current best suit
        if (s == curBestSuit) {
          // need a higher rank than current best
          trackp->lowest_win[relh][s] = find_smallest_rank_greater(ris, curBestRank);
          continue;
        }

        // If candidate is trump
        if (s == trump) {
          if (curBestSuit != trump) {
            // any trump will beat non-trump; choose smallest trump in hand
            trackp->lowest_win[relh][s] = find_smallest_rank(ris);
          } else {
            // best is also trump: need higher trump
            trackp->lowest_win[relh][s] = find_smallest_rank_greater(ris, curBestRank);
          }
          continue;
        }

        // Candidate is non-trump and not equal to current best suit.
        // If current best is trump, non-trump cannot win.
        if (curBestSuit == trump) {
          trackp->lowest_win[relh][s] = 0;
          continue;
        }

        // If current best is of a different suit (not trump), then only a card
        // in the lead suit can beat it; if candidate suit equals lead_suit,
        // we can try to beat that; otherwise cannot win.
        if (s == trackp->lead_suit) {
          // If current best is also lead_suit this case is handled earlier;
          // here current best is different suit => it must be that someone
          // trumped already, which we handled above. As a fallback, require
          // higher rank than any current best of this suit.
          trackp->lowest_win[relh][s] = find_smallest_rank_greater(ris, curBestRank);
        } else {
          trackp->lowest_win[relh][s] = 0;
        }
      }
    }
  }
}
