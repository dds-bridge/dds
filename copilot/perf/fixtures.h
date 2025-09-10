// Realistic pos fixtures extracted from existing tests for microbench and experiments.
#pragma once

#include "dds/dds.h"

// Fixture 1: basic empty-ish position (useful as a control)
inline pos fixture_basic() {
  pos tpos = {};
  for (int hand = 0; hand < DDS_HANDS; ++hand)
    for (int suit = 0; suit < DDS_SUITS; ++suit) {
      tpos.rankInSuit[hand][suit] = 0;
      tpos.length[hand][suit] = 0;
    }
  for (int s = 0; s < DDS_SUITS; ++s) {
    tpos.winner[s].hand = 0;
    tpos.winner[s].rank = 0;
    tpos.secondBest[s].hand = 0;
    tpos.secondBest[s].rank = 0;
    tpos.aggr[s] = 0;
  }
  return tpos;
}

// Fixture 2: small trump-rich hand inspired by integration_test::createTestPosition
inline pos fixture_trump_rich(bool hasTrumpWinner = true, int trumpSuit = 1) {
  pos tpos = {};
  // initialize arrays
  for (int hand = 0; hand < DDS_HANDS; ++hand)
    for (int suit = 0; suit < DDS_SUITS; ++suit) {
      tpos.rankInSuit[hand][suit] = 0;
      tpos.length[hand][suit] = 0;
    }

  // Hand 0 (North) has A K Q of spades and a K of hearts
  tpos.rankInSuit[0][0] = 0x7000; // A K Q (bitmask style used in tests)
  tpos.length[0][0] = 3;
  tpos.rankInSuit[0][1] = 0x1000; // K of hearts
  tpos.length[0][1] = 1;

  // Hand 1 (East) has J of spades and A K of diamonds
  tpos.rankInSuit[1][0] = 0x0800; // J
  tpos.length[1][0] = 1;
  tpos.rankInSuit[1][2] = 0x6000; // A K of diamonds
  tpos.length[1][2] = 2;

  if (hasTrumpWinner) {
    tpos.winner[trumpSuit].rank = 14;
    tpos.winner[trumpSuit].hand = 0;
  }

  return tpos;
}

// Fixture 3: dense distribution with varying lengths and winRanks (more stress)
inline pos fixture_dense() {
  pos tpos = {};
  for (int hand = 0; hand < DDS_HANDS; ++hand) {
    for (int suit = 0; suit < DDS_SUITS; ++suit) {
      // give each hand a few cards of varying lengths
      tpos.length[hand][suit] = (hand + suit) % 5; // 0..4
      // set some synthetic rank masks for winRanks used by heuristics
      tpos.rankInSuit[hand][suit] = ((hand + 1) << 12) | ((suit + 1) << 8);
    }
    tpos.handDist[hand] = 0;
  }
  // populate winRanks with small depth patterns
  for (int d = 0; d < 10; ++d) {
    for (int s = 0; s < DDS_SUITS; ++s) {
      tpos.winRanks[d][s] = (unsigned short)((d + 1) << (s + 1));
    }
  }
  for (int s = 0; s < DDS_SUITS; ++s) {
    tpos.winner[s].hand = s % DDS_HANDS;
    tpos.winner[s].rank = 10 + s;
    tpos.secondBest[s].hand = (s + 1) % DDS_HANDS;
    tpos.secondBest[s].rank = 5 + s;
  }
  return tpos;
}
