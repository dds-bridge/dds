// Small microbenchmark to compare legacy WeightAlloc vs new CallHeuristic
#include <chrono>
#include <iostream>
#include <vector>

#include "dds/dds.h"
#include "moves/Moves.h"
#include "fixtures.h"

using namespace std::chrono;

int main(int argc, char** argv) {
  const int iters = 2000; // fewer iterations but heavier per-call work

  // Create dummy inputs: a Moves instance and a pos selected by fixture
  Moves moves;
  pos tpos{};

  // parse simple --fixture=basic|trump|dense and --mode=alloc|movegen123
  std::string fixture = "basic";
  std::string mode = "alloc"; // default: allocation microbench
  for (int i = 1; i < argc; ++i) {
    std::string s(argv[i]);
    if (s.rfind("--fixture=", 0) == 0) fixture = s.substr(10);
    if (s.rfind("--mode=", 0) == 0) mode = s.substr(7);
  }

  if (fixture == "basic") tpos = fixture_basic();
  else if (fixture == "trump") tpos = fixture_trump_rich();
  else if (fixture == "dense") tpos = fixture_dense();
  else {
    std::cerr << "unknown fixture '" << fixture << "', defaulting to basic\n";
    tpos = fixture_basic();
  }

  // Minimal initialization: set some fields that weight allocators read.
  // Rely on defaults for others.
  // Set lengths and winners to small deterministic values.
  for (int h = 0; h < DDS_HANDS; ++h) {
    for (int s = 0; s < DDS_SUITS; ++s) {
      tpos.length[h][s] = (h + s) % 4; // small varied lengths
      tpos.rankInSuit[h][s] = 0; // zero by default
    }
  }

  // Initialize winners/secondBest to sensible defaults
  for (int s = 0; s < DDS_SUITS; ++s) {
    tpos.winner[s].hand = 0;
    tpos.winner[s].rank = 0;
    tpos.secondBest[s].hand = 1;
    tpos.secondBest[s].rank = 0;
  tpos.aggr[s] = 0;
  for (int d = 0; d < 50; ++d) tpos.winRanks[d][s] = 0;
  }

  // Initialize moves.track to sensible defaults
  for (int i = 0; i < 13; ++i) {
    moves.track[i].leadHand = 0;
    moves.track[i].leadSuit = 0;
    for (int h = 0; h < DDS_HANDS; ++h) {
      moves.track[i].playSuits[h] = 0;
      moves.track[i].playRanks[h] = 0;
      moves.track[i].high[h] = 0;
    }
    for (int s = 0; s < DDS_SUITS; ++s)
      moves.track[i].removedRanks[s] = 0xffff;
  }

  // Prepare a move list in moves similar to MoveGen0 inner state.
  // We'll directly populate moveList[0][0].move with a handful of moves.
  // ...existing code...

  // Create a few sample moves
  movePlyType& list = moves.moveList[0][0];
  moveType* mply = list.move;
  int numMoves = 0;
  for (int suit = 0; suit < DDS_SUITS; ++suit) {
    for (int r = 1; r <= 4; ++r) {
      mply[numMoves].suit = suit;
      mply[numMoves].rank = r;
      mply[numMoves].sequence = 0;
      ++numMoves;
      if (numMoves >= 12) break;
    }
    if (numMoves >= 12) break;
  }
  list.current = 0;
  list.last = numMoves - 1;

  // Initialize Moves internal state so weight allocators can run safely.
  moves.currTrick = 0;
  moves.trackp = &moves.track[0];
  moves.leadHand = 0;
  moves.currHand = 0;
  moves.suit = mply[0].suit;
  moves.numMoves = numMoves;
  moves.lastNumMoves = 0;
  moves.mply = mply;

  // Dummy relRanksType array used by WeightAlloc* functions.
  relRanksType thrp_rel[16];
  for (int i = 0; i < 16; ++i) {
    for (int r = 0; r < 15; ++r)
      for (int s = 0; s < DDS_SUITS; ++s) {
        thrp_rel[i].absRank[r][s].hand = 0;
        thrp_rel[i].absRank[r][s].rank = 0;
      }
  }

  // Timer for legacy: call WeightAllocNT0 directly (as representative)
  if (mode == "alloc") {
    // Timer for legacy: call WeightAllocNT0 directly (as representative)
    auto t0 = steady_clock::now();
    for (int i = 0; i < iters; ++i) {
      moves.WeightAllocNT0(tpos, moveType{}, moveType{}, thrp_rel);
    }
    auto t1 = steady_clock::now();

    // Timer for new: call CallHeuristic
    auto t2 = steady_clock::now();
    for (int i = 0; i < iters; ++i) {
      CallHeuristic(tpos, moveType{}, moveType{}, thrp_rel, mply, moves.numMoves,
                    moves.lastNumMoves, moves.trump, moves.suit, moves.trackp,
                    moves.currTrick, moves.currHand, moves.leadHand, moves.leadSuit);
    }
    auto t3 = steady_clock::now();

    auto dur_legacy_us = duration_cast<microseconds>(t1 - t0).count();
    auto dur_new_us = duration_cast<microseconds>(t3 - t2).count();

    std::cout << "weight_alloc_microbench mode=alloc iterations=" << iters << " moves=" << numMoves << "\n";
    std::cout << "legacy WeightAllocNT0 total_us=" << dur_legacy_us << " per_call_ns=" << (dur_legacy_us * 1000.0 / iters) << "\n";
    std::cout << "new CallHeuristic  total_us=" << dur_new_us << " per_call_ns=" << (dur_new_us * 1000.0 / iters) << "\n";
  } else if (mode == "movegen123") {
    // Ensure moves.track[tricks].leadSuit is set appropriately.
    const int tricks = 0; // use trick 0 for this microbench
    // initialize removedRanks and leadSuit similar to Moves::Init
    moves.track[tricks].leadHand = 0;
    moves.track[tricks].leadSuit = 0;
    for (int s = 0; s < DDS_SUITS; ++s)
      moves.track[tricks].removedRanks[s] = 0xffff;

    // apply rankInSuit removal like Init would
    for (int h = 0; h < DDS_HANDS; ++h)
      for (int s = 0; s < DDS_SUITS; ++s)
        moves.track[tricks].removedRanks[s] ^= tpos.rankInSuit[h][s];

    // set leadHand etc.
    moves.trackp = &moves.track[tricks];
    moves.track[tricks].leadHand = 0;
    moves.leadHand = 0;

    auto t0 = steady_clock::now();
    for (int i = 0; i < iters; ++i) {
      // Heavier per-iteration initialization to more closely mimic real flow.
      // Reset track removedRanks from the pos each iteration.
      for (int s = 0; s < DDS_SUITS; ++s)
        moves.track[tricks].removedRanks[s] = 0xffff;
      for (int h = 0; h < DDS_HANDS; ++h)
        for (int s = 0; s < DDS_SUITS; ++s)
          moves.track[tricks].removedRanks[s] ^= tpos.rankInSuit[h][s];

      // Call Init to mirror real initialization (small, safe args)
      int initialRanks[3] = {0, 0, 0};
      int initialSuits[3] = {0, 0, 0};
      moves.Init(tricks, 0, initialRanks, initialSuits, tpos.rankInSuit, moves.trump, moves.track[tricks].leadHand);

      // call MoveGen123 for relative hand = 1 (following hand)
      moves.MoveGen123(tricks, 1, tpos);
    }
    auto t1 = steady_clock::now();
    auto dur_us = duration_cast<microseconds>(t1 - t0).count();
    std::cout << "weight_alloc_microbench mode=movegen123 iterations=" << iters << " moves=" << numMoves << "\n";
    std::cout << "MoveGen123 total_us=" << dur_us << " per_call_ns=" << (dur_us * 1000.0 / iters) << "\n";
  } else {
    std::cerr << "unknown mode '" << mode << "'\n";
  }

  if (mode == "compare") {
    // Compare legacy vs new by toggling runtime switch (requires build with DDS_USE_NEW_HEURISTIC)
    const int tricks = 0;
    auto run_with_flag = [&](bool use_new) {
#ifdef DDS_USE_NEW_HEURISTIC
      set_use_new_heuristic(use_new);
#else
      (void)use_new; // no-op when build flag not set
#endif
      // heavier per-iteration init and run MoveGen123
      for (int s = 0; s < DDS_SUITS; ++s)
        moves.track[tricks].removedRanks[s] = 0xffff;
      for (int h = 0; h < DDS_HANDS; ++h)
        for (int s = 0; s < DDS_SUITS; ++s)
          moves.track[tricks].removedRanks[s] ^= tpos.rankInSuit[h][s];

      auto t0 = steady_clock::now();
      for (int i = 0; i < iters; ++i) {
        for (int s = 0; s < DDS_SUITS; ++s)
          moves.track[tricks].removedRanks[s] = 0xffff;
        for (int h = 0; h < DDS_HANDS; ++h)
          for (int s = 0; s < DDS_SUITS; ++s)
            moves.track[tricks].removedRanks[s] ^= tpos.rankInSuit[h][s];

        int initialRanks[3] = {0, 0, 0};
        int initialSuits[3] = {0, 0, 0};
        moves.Init(tricks, 0, initialRanks, initialSuits, tpos.rankInSuit, moves.trump, moves.track[tricks].leadHand);
        moves.MoveGen123(tricks, 1, tpos);
      }
      auto t1 = steady_clock::now();
      return duration_cast<microseconds>(t1 - t0).count();
    };

    long legacy_us = run_with_flag(false);
    long new_us = run_with_flag(true);
    std::cout << "weight_alloc_microbench mode=compare iterations=" << iters << " moves=" << numMoves << "\n";
    std::cout << "legacy MoveGen123 total_us=" << legacy_us << " per_call_ns=" << (legacy_us * 1000.0 / iters) << "\n";
    std::cout << "new    MoveGen123 total_us=" << new_us << " per_call_ns=" << (new_us * 1000.0 / iters) << "\n";
  }

  return 0;
}
