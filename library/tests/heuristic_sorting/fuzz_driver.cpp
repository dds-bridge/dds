#include <gtest/gtest.h>
#include <random>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include "test_utils.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "moves/Moves.h"

static std::string run_and_serialize_once(const pos& tpos, const relRanksType* thrp_rel, moveType* moves, int numMoves, int trump, int suit, trackType* trackp, int currTrick, int currHand, int leadHand, int leadSuit) {
  moveType bestMove = {};
  moveType bestMoveTT = {};
  CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                0, trump, suit, trackp, currTrick, currHand, leadHand, leadSuit);
  return normalize_ordering(moves, numMoves, true);
}

TEST(FuzzDriver, RandomizedBatch) {
  const char* env_count = std::getenv("FUZZ_COUNT");
  const char* env_seed = std::getenv("FUZZ_SEED");
  int count = env_count ? std::atoi(env_count) : 100;
  unsigned seed = env_seed ? static_cast<unsigned>(std::atoi(env_seed)) : 12345u;

  std::mt19937 rng(seed);
  int matched = 0;
  int mismatched = 0;

  for (int i = 0; i < count; ++i) {
    // mutate seed per-case for variety
    unsigned case_seed = seed + i;
    std::mt19937 g(case_seed);

    pos tpos;
    std::memset(&tpos, 0, sizeof(tpos));

    // Realistic random deal: shuffle 52 cards and distribute to 4 hands.
    // Cards encoded as 0..51: suit = card / 13, rank = (card % 13) + 1
    std::vector<int> deck(52);
    for (int i = 0; i < 52; ++i) deck[i] = i;
    std::shuffle(deck.begin(), deck.end(), g);

    int idx = 0;
    // Build per-suit card lists then remap ranks to continuous numbers
    std::vector<std::pair<int,int>> suitCards[DDS_SUITS];
    int suitCountsPerHand[DDS_HANDS][DDS_SUITS] = {};

    for (int h = 0; h < DDS_HANDS; ++h) {
      for (int c = 0; c < 13; ++c) {
        int card = deck[idx++];
        int suit = card / 13;
        int rank = (card % 13) + 1; // 1..13 original rank
        suitCards[suit].push_back({rank, h});
        suitCountsPerHand[h][suit]++;
      }
    }

    // For each suit, sort by original rank desc and assign new ranks
    for (int s = 0; s < DDS_SUITS; ++s) {
      auto &vec = suitCards[s];
      std::sort(vec.begin(), vec.end(), [](const auto &a, const auto &b){ return a.first > b.first; });
      int newRank = 13;
      for (auto &p : vec) {
        int hand = p.second;
        tpos.rankInSuit[hand][s] |= static_cast<unsigned short>(1u << newRank);
        if (newRank > 1) --newRank; else newRank = 1;
      }
    }

    // set lengths/handDist
    for (int h = 0; h < DDS_HANDS; ++h) {
      tpos.handDist[h] = 0;
      for (int s = 0; s < DDS_SUITS; ++s) {
        tpos.length[h][s] = static_cast<unsigned char>(suitCountsPerHand[h][s]);
        tpos.handDist[h] += suitCountsPerHand[h][s];
      }
    }

    // compute aggr for each suit (aggregate ranks across hands)
    for (int s = 0; s < DDS_SUITS; ++s) {
      unsigned short agg = 0;
      for (int h = 0; h < DDS_HANDS; ++h) agg |= tpos.rankInSuit[h][s];
      tpos.aggr[s] = agg;
    }

    // minimal winner information: set winner suit's rank to highest card seen
    for (int s = 0; s < DDS_SUITS; ++s) {
      tpos.winner[s].rank = 0;
      tpos.winner[s].hand = 0;
      for (int h = 0; h < DDS_HANDS; ++h) {
        unsigned short ris = tpos.rankInSuit[h][s];
        if (ris) {
          // find highest rank bit
          for (int r = 13; r >= 1; --r) {
            if (ris & (1u << r)) { tpos.winner[s].rank = r; tpos.winner[s].hand = h; break; }
          }
        }
      }
    }

    const int numMoves = 10;
  moveType moves[numMoves];
    for (int m = 0; m < numMoves; ++m) {
      moves[m].suit = m % 4;
      moves[m].rank = 14 - (m % 13);
      moves[m].weight = 0;
      moves[m].sequence = m + 1;
    }

    // pick a random current trick (number of tricks already played)
    std::uniform_int_distribution<int> trickDist(0, 12);
    int currTrick = trickDist(g);

  // Decide whether to simulate a mid-trick with 1..3 cards played (~30% cases)
  std::uniform_real_distribution<double> prob(0.0, 1.0);
  int cardsPlayed = 0;
  moveType playedMoves[4];
  trackType track = {};
  static relRanksType relTable[8192];
  int leadHand = 0;
  if (prob(g) < 0.30) {
      std::uniform_int_distribution<int> cardsDist(1, 3);
      cardsPlayed = cardsDist(g);

      // Choose a lead hand and then generate played moves in order from lead
  std::uniform_int_distribution<int> leadDist(0, 3);
  leadHand = leadDist(g);
  int nextHand = leadHand;

      for (int p = 0; p < cardsPlayed; ++p) {
        // pick a random suit where that hand has at least one card
        std::vector<int> suitsWithCard;
        for (int s = 0; s < DDS_SUITS; ++s) {
          if (tpos.rankInSuit[nextHand][s]) suitsWithCard.push_back(s);
        }
        if (suitsWithCard.empty()) {
          // fallback: pick any suit
          playedMoves[p].suit = p % DDS_SUITS;
          playedMoves[p].rank = 1;
        } else {
          int sidx = suitsWithCard[g() % suitsWithCard.size()];
          // pick highest available rank in that suit for that hand
          unsigned short ris = tpos.rankInSuit[nextHand][sidx];
          int pickRank = 1;
          for (int r = 13; r >= 1; --r) if (ris & (1u << r)) { pickRank = r; break; }
          playedMoves[p].suit = sidx;
          playedMoves[p].rank = pickRank;
        }
        playedMoves[p].sequence = p + 1;
        nextHand = (nextHand + 1) % 4;
      }

  // Initialize rel table and track using helper with mid-trick (use chosen leadHand)
  init_rel_and_track(tpos, relTable, &track, cardsPlayed, playedMoves, leadHand, /*trump*/1);
    } else {
      // No mid-trick
      cardsPlayed = 0;
  // choose a random leadHand when no mid-trick to vary positions
  std::uniform_int_distribution<int> leadDist2(0,3);
  leadHand = leadDist2(g);
  init_rel_and_track(tpos, relTable, &track, 0, nullptr, leadHand, /*trump*/1);
    }

    // Randomize currHand and suit to exercise following-hand logic as well
    std::uniform_int_distribution<int> handDist(0,3);
    int currHand = handDist(g);
    // pick a leadSuit: prefer first played card suit when mid-trick, else random
    int leadSuit = 0;
    if (cardsPlayed > 0) leadSuit = playedMoves[0].suit;
    else {
      std::uniform_int_distribution<int> suitDist(0,3);
      leadSuit = suitDist(g);
    }

    // choose a suit context for CallHeuristic (can be different from leadSuit)
    std::uniform_int_distribution<int> suitCtx(0,3);
    int suit = suitCtx(g);

    std::string legacy;
    std::string neu;
    bool okLegacy = true;
    bool okNew = true;
    try {
      set_use_new_heuristic(false);
      legacy = run_and_serialize_once(tpos, relTable, moves, numMoves, 1, suit, &track, currTrick, currHand, leadHand, leadSuit);
    } catch (...) {
      okLegacy = false;
    }

    for (int m = 0; m < numMoves; ++m) moves[m].weight = 0;

    try {
      set_use_new_heuristic(true);
      neu = run_and_serialize_once(tpos, relTable, moves, numMoves, 1, suit, &track, currTrick, currHand, leadHand, leadSuit);
    } catch (...) {
      okNew = false;
    }

    if (!okLegacy || !okNew) {
      mismatched++;
      // write a diagnostic for crash/failure cases
      std::ostringstream fname;
      auto ts = std::chrono::system_clock::now();
      auto t = std::chrono::system_clock::to_time_t(ts);
      fname << "build/compare-results/fuzz_" << t << "_case_" << i << "_error.txt";
      std::ofstream ofs(fname.str());
      ofs << "okLegacy=" << okLegacy << " okNew=" << okNew << "\n";
      continue;
    }

    if (legacy == neu) matched++; else mismatched++;

    if (mismatched > 0 && mismatched <= 5) {
      // write per-case diagnostics for first few mismatches
      std::ostringstream fname;
      auto ts = std::chrono::system_clock::now();
      auto t = std::chrono::system_clock::to_time_t(ts);
      fname << "build/compare-results/fuzz_" << t << "_case_" << i << "_legacy.json";
      write_json_log(fname.str(), legacy);
      fname.str(""); fname << "build/compare-results/fuzz_" << t << "_case_" << i << "_new.json";
      write_json_log(fname.str(), neu);
    }
  }

  std::cout << "Fuzz results: total=" << count << " matched=" << matched << " mismatched=" << mismatched << "\n";
  if (mismatched > 0) {
    std::cerr << "ERROR: Found " << mismatched << " mismatches out of " << count << " cases\n";
    FAIL() << "Found " << mismatched << " mismatches out of " << count << " cases";
  } else {
    SUCCEED();
  }
}

