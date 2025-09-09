#include <gtest/gtest.h>
#include <random>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iostream>

#include "test_utils.h"
#include "heuristic_sorting/heuristic_sorting.h"
#include "heuristic_sorting/internal.h"
#include "moves/Moves.h"

static std::string run_and_serialize_once(const pos& tpos, const relRanksType* thrp_rel, moveType* moves, int numMoves, int trump, trackType* trackp) {
  moveType bestMove = {};
  moveType bestMoveTT = {};

  CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel, moves, numMoves,
                0, trump, 0, trackp, 1, 0, 0, 0);
  return normalize_ordering(moves, numMoves, true);
}

TEST(FuzzDriver, RandomizedBatch) {
#ifdef DDS_USE_NEW_HEURISTIC
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
    for (int h = 0; h < DDS_HANDS; ++h) {
      int suitCounts[DDS_SUITS] = {0,0,0,0};
      for (int c = 0; c < 13; ++c) {
        int card = deck[idx++];
        int suit = card / 13;
        int rank = (card % 13) + 1; // 1..13

        // set bit for rank in rankInSuit (we use 1<<rank, safe within 16 bits)
        tpos.rankInSuit[h][suit] |= static_cast<unsigned short>(1u << rank);
        suitCounts[suit]++;
      }
      // set lengths for this hand
      for (int s = 0; s < DDS_SUITS; ++s) {
        tpos.length[h][s] = static_cast<unsigned char>(suitCounts[s]);
      }
      // handDist: total cards in hand
      tpos.handDist[h] = 0;
      for (int s = 0; s < DDS_SUITS; ++s) tpos.handDist[h] += suitCounts[s];
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

  // Initialize rel table and track using helper
  trackType track = {};
  static relRanksType relTable[8192];
  init_rel_and_track(tpos, relTable, &track);

    std::string legacy;
    std::string neu;
    bool okLegacy = true;
    bool okNew = true;
    try {
      set_use_new_heuristic(false);
      legacy = run_and_serialize_once(tpos, relTable, moves, numMoves, 1, &track);
    } catch (...) {
      okLegacy = false;
    }

    for (int m = 0; m < numMoves; ++m) moves[m].weight = 0;

    try {
      set_use_new_heuristic(true);
      neu = run_and_serialize_once(tpos, relTable, moves, numMoves, 1, &track);
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
  EXPECT_EQ(0, mismatched) << "Found " << mismatched << " mismatches out of " << count << " cases";
#else
  GTEST_SKIP() << "Runtime toggling not available: build with --define=new_heuristic=true";
#endif
}
