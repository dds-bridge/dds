/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <heuristic_sorting/heuristic_sorting.hpp>
#include <lookup_tables/lookup_tables.hpp>
#include "moves.hpp"

using std::cout;
using std::endl;
using std::fixed;
using std::hex;
using std::left;
using std::ofstream;
using std::right;
using std::setprecision;
using std::setw;
using std::string;
using std::stringstream;
using std::to_string;

#ifdef DDS_MOVES
#define MG_REGISTER(a, b) lastCall[currTrick][b] = a
const MgType RegisterList[16] = {MgType::NT0,           MgType::TRUMP0,
                                 MgType::SIZE,          MgType::SIZE, // Unused

                                 MgType::NT_NOTVOID1,   MgType::TRUMP_NOTVOID1,
                                 MgType::NT_VOID1,      MgType::TRUMP_VOID1,

                                 MgType::NT_NOTVOID2,   MgType::TRUMP_NOTVOID2,
                                 MgType::NT_VOID2,      MgType::TRUMP_VOID2,

                                 MgType::COMB_NOTVOID3, MgType::COMB_NOTVOID3,
                                 MgType::NT_VOID3,      MgType::TRUMP_VOID3};
#else
#define MG_REGISTER(a, b) 1;
#endif

Moves::Moves() {
  // Initialize non-owning pointers to nullptr for safety
  trackp = nullptr;
  mply = nullptr;

  funcName[static_cast<int>(MgType::NT0)] = "NT0";
  funcName[static_cast<int>(MgType::TRUMP0)] = "Trump0";
  funcName[static_cast<int>(MgType::NT_VOID1)] = "NT_Void1";
  funcName[static_cast<int>(MgType::TRUMP_VOID1)] = "Trump_Void1";
  funcName[static_cast<int>(MgType::NT_NOTVOID1)] = "NT_Notvoid1";
  funcName[static_cast<int>(MgType::TRUMP_NOTVOID1)] = "Trump_Notvoid1";
  funcName[static_cast<int>(MgType::NT_VOID2)] = "NT_Void2";
  funcName[static_cast<int>(MgType::TRUMP_VOID2)] = "Trump_Void2";
  funcName[static_cast<int>(MgType::NT_NOTVOID2)] = "NT_Notvoid2";
  funcName[static_cast<int>(MgType::TRUMP_NOTVOID2)] = "Trump_Notvoid2";
  funcName[static_cast<int>(MgType::NT_VOID3)] = "NT_Void3";
  funcName[static_cast<int>(MgType::TRUMP_VOID3)] = "Trump_Void3";
  funcName[static_cast<int>(MgType::COMB_NOTVOID3)] = "Comb_Notvoid3";

  for (int t = 0; t < 13; t++) {
    for (int h = 0; h < DDS_HANDS; h++) {
      lastCall[t][h] = MgType::SIZE;

      trickTable[t][h].count = 0;
      trickSuitTable[t][h].count = 0;

      trickDetailTable[t][h].nfuncs = 0;
      trickDetailSuitTable[t][h].nfuncs = 0;
      for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
        trickDetailTable[t][h].list[i].count = 0;
        trickDetailSuitTable[t][h].list[i].count = 0;
      }
    }
  }

  trickFuncTable.nfuncs = 0;
  trickFuncSuitTable.nfuncs = 0;
  for (int i = 0; i < static_cast<int>(MgType::SIZE); i++) {
    trickFuncTable.list[i].count = 0;
    trickFuncSuitTable.list[i].count = 0;
  }
}

Moves::~Moves() {}

/**
 * @brief Initialize move generation state for a new deal.
 *
 * Sets up tracking arrays and initializes removed ranks based on the starting
 * position. Handles partial tricks by incorporating already-played cards.
 *
 * The removedRanks array tracks which cards are no longer available. It starts
 * with all cards (0xffff), then XORs with available cards to mark them as
 * available, then XORs back any cards already played in this trick.
 */
auto Moves::Init(const int tricks, const int relStartHand,
                 const int initialRanks[], const int initialSuits[],
                 const unsigned short rank_in_suit[DDS_HANDS][DDS_SUITS],
                 const int our_trump, const int our_lead_hand) -> void {
  currTrick = tricks;
  trump = our_trump;

  if (relStartHand == 0)
    track[tricks].lead_hand = our_lead_hand;

  for (int m = 0; m < 13; m++) {
    for (int h = 0; h < DDS_HANDS; h++) {
      moveList[m][h].current = 0;
      moveList[m][h].last = 0;
    }
  }

  // 0x1ffff would be enough, but this is for compatibility.
  for (int s = 0; s < DDS_SUITS; s++)
    track[tricks].removed_ranks[s] = 0xffff;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      track[tricks].removed_ranks[s] ^= rank_in_suit[h][s];

  for (int n = 0; n < relStartHand; n++) {
    int s = initialSuits[n];
    int r = initialRanks[n];

    track[tricks].removed_ranks[s] ^= bit_map_rank[r];
  }
}

auto Moves::Reinit(const int tricks, const int ourLeadHand) -> void {
  track[tricks].lead_hand = ourLeadHand;
}

/**
 * @brief Generate moves for the opening lead (first hand of trick).
 *
 * Iterates through all suits and generates legal plays from each. Uses
 * precomputed group_data to identify sequences of equivalent cards.
 * The inner loop merges consecutive groups when gaps are filled by
 * removed cards, reducing the move list to distinct strategic options.
 *
 * Calls heuristic sorting after each suit to prioritize likely-good moves.
 * Final sort by weight ensures best moves are tried first in search.
 *
 * @return Number of legal moves generated
 */
auto Moves::MoveGen0(const int tricks, const Pos &tpos,
                     const MoveType &bestMove, const MoveType &bestMoveTT,
                     const RelRanksType thrp_rel[]) -> int {
  trackp = &track[tricks];
  leadHand = trackp->lead_hand;
  currHand = leadHand;
  currTrick = tricks;

  const MoveGroupType *mp;
  int removed, g, rank, seq;

  MovePlyType &list = moveList[tricks][0];
  mply = list.move;
  for (int s = 0; s < DDS_SUITS; s++)
    trackp->lowest_win[0][s] = 0;
  numMoves = 0;

  for (suit = 0; suit < DDS_SUITS; suit++) {
    unsigned short ris = tpos.rank_in_suit[leadHand][suit];
    if (ris == 0)
      continue;

    lastNumMoves = numMoves;
    mp = &group_data[ris];
    g = mp->last_group_;
    removed = trackp->removed_ranks[suit];

    // Generate moves for this suit by iterating through card groups.
    // Merge consecutive groups when gaps are filled by removed cards.
    while (g >= 0) {
      rank = mp->rank_[g];
      seq = mp->sequence_[g];

      // If all cards in the gap above this group have been played,
      // merge this group with the one below it (equivalent plays).
      while (g >= 1 && ((mp->gap_[g] & removed) == mp->gap_[g]))
        seq |= mp->fullseq_[--g];

      mply[numMoves].sequence = seq;
      mply[numMoves].suit = suit;
      mply[numMoves].rank = rank;

      numMoves++;
      g--;
    }

    Moves::call_heuristic(tpos, bestMove, bestMoveTT, thrp_rel);
  }

#ifdef DDS_MOVES
  bool ftest = ((trump != DDS_NOTRUMP) && (tpos.winner[trump].rank != 0));
  if (ftest)
    MG_REGISTER(MgType::TRUMP0, 0);
  else
    MG_REGISTER(MgType::NT0, 0);
#endif

  list.current = 0;
  list.last = numMoves - 1;

  if (numMoves != 1)
    Moves::MergeSort();
  return numMoves;
}

auto Moves::MoveGen123(const int tricks, const int handRel, const Pos &tpos)
    -> int {
  trackp = &track[tricks];
  leadHand = trackp->lead_hand;
  currHand = HAND_ID(leadHand, handRel);
  currTrick = tricks;
  leadSuit = track[tricks].lead_suit;

  const MoveGroupType *mp;
  int removed, g, rank, seq;

  MovePlyType &list = moveList[tricks][handRel];
  mply = list.move;

  for (int s = 0; s < DDS_SUITS; s++)
    trackp->lowest_win[handRel][s] = 0;
  numMoves = 0;

  [[maybe_unused]] int findex;
  int ftest =
      ((trump != DDS_NOTRUMP) && (tpos.winner[trump].rank != 0) ? 1 : 0);

  unsigned short ris = tpos.rank_in_suit[currHand][leadSuit];

  if (ris != 0) {
    mp = &group_data[ris];
    g = mp->last_group_;
    removed = trackp->removed_ranks[leadSuit];

    while (g >= 0) {
      rank = mp->rank_[g];
      seq = mp->sequence_[g];

      while (g >= 1 && ((mp->gap_[g] & removed) == mp->gap_[g]))
        seq |= mp->fullseq_[--g];

      mply[numMoves].sequence = seq;
      mply[numMoves].suit = leadSuit;
      mply[numMoves].rank = rank;

      numMoves++;
      g--;
    }

    findex = 4 * handRel + ftest;
#ifdef DDS_MOVES
    MG_REGISTER(RegisterList[findex], handRel);
#endif

    list.current = 0;
    list.last = numMoves - 1;
    if (numMoves == 1)
      return numMoves;
    Moves::call_heuristic(tpos, MoveType{}, MoveType{}, nullptr);

    Moves::MergeSort();
    return numMoves;
  }

  findex = 4 * handRel + ftest + 2;

#ifdef DDS_MOVES
  MG_REGISTER(RegisterList[findex], handRel);
#endif

  for (suit = 0; suit < DDS_SUITS; suit++) {
    ris = tpos.rank_in_suit[currHand][suit];
    if (ris == 0)
      continue;

    lastNumMoves = numMoves;
    mp = &group_data[ris];
    g = mp->last_group_;
    removed = trackp->removed_ranks[suit];

    while (g >= 0) {
      rank = mp->rank_[g];
      seq = mp->sequence_[g];

      while (g >= 1 && ((mp->gap_[g] & removed) == mp->gap_[g]))
        seq |= mp->fullseq_[--g];

      mply[numMoves].sequence = seq;
      mply[numMoves].suit = suit;
      mply[numMoves].rank = rank;

      numMoves++;
      g--;
    }

    Moves::call_heuristic(tpos, MoveType{}, MoveType{}, nullptr);
  }

  list.current = 0;
  list.last = numMoves - 1;
  if (numMoves != 1)
    Moves::MergeSort();
  return numMoves;
}

auto Moves::GetTopNumber(const int ris, const int prank, int &topNumber,
                         int &mno) const -> void {
  // Determine how many winning moves exist when partner has played prank.
  // topNumber indicates the number of cards that can win, mno is the index
  // in the move list of the lowest winning card.
  topNumber = -10;

  // Find the lowest move that still overtakes partner's card.
  mno = 0;
  while (mno < numMoves - 1 && mply[1 + mno].rank > prank)
    mno++;

  const MoveGroupType &mp = group_data[ris];
  int g = mp.last_group_;

  // Include partner's card as removed to count only moves that beat it.
  const int removed =
      static_cast<int>(trackp->removed_ranks[leadSuit] | bit_map_rank[prank]);

  int fullseq = mp.fullseq_[g];

  // Merge groups as in move generation, accounting for gaps filled
  // by removed cards (including partner's card).
  while (g >= 1 && ((mp.gap_[g] & removed) == mp.gap_[g]))
    fullseq |= mp.fullseq_[--g];

  topNumber = count_table[fullseq] - 1;
}

inline auto Moves::WinningMove(const MoveType &mvp1, const ExtCard &mvp2,
                               const int ourTrump) const -> bool {
  /* Return true if move 1 wins over move 2, with the assumption that
  move 2 is the presently winning card of the trick */

  if (mvp1.suit == mvp2.suit) {
    if (mvp1.rank > mvp2.rank)
      return true;
    else
      return false;
  } else if (mvp1.suit == ourTrump)
    return true;
  else
    return false;
}

auto Moves::GetLength(const int trick, const int relHand) const -> int {
  return moveList[trick][relHand].last + 1;
}

auto Moves::MakeSpecific(const MoveType &ourMply, const int trick,
                         const int relHand) -> void {
  trackp = &track[trick];

  if (relHand == 0) {
    trackp->move[0].suit = ourMply.suit;
    trackp->move[0].rank = ourMply.rank;
    trackp->move[0].sequence = ourMply.sequence;
    trackp->high[0] = 0;

    trackp->lead_suit = ourMply.suit;
  } else if (ourMply.suit == trackp->move[relHand - 1].suit) {
    if (ourMply.rank > trackp->move[relHand - 1].rank) {
      trackp->move[relHand].suit = ourMply.suit;
      trackp->move[relHand].rank = ourMply.rank;
      trackp->move[relHand].sequence = ourMply.sequence;
      trackp->high[relHand] = relHand;
    } else {
      trackp->move[relHand] = trackp->move[relHand - 1];
      trackp->high[relHand] = trackp->high[relHand - 1];
    }
  } else if (ourMply.suit == trump) {
    trackp->move[relHand].suit = ourMply.suit;
    trackp->move[relHand].rank = ourMply.rank;
    trackp->move[relHand].sequence = ourMply.sequence;
    trackp->high[relHand] = relHand;
  } else {
    trackp->move[relHand] = trackp->move[relHand - 1];
    trackp->high[relHand] = trackp->high[relHand - 1];
  }

  trackp->play_suits[relHand] = ourMply.suit;
  trackp->play_ranks[relHand] = ourMply.rank;

  // When trick completes (4th card played), prepare next trick's state.
  if (relHand == 3) {
    TrackType *newp = &track[trick - 1];

    // Winner of this trick leads the next one.
    newp->lead_hand = (trackp->lead_hand + trackp->high[3]) % 4;

    // Update removed ranks to include all cards played in this trick.
    int r, s;
    for (s = 0; s < DDS_SUITS; s++)
      newp->removed_ranks[s] = trackp->removed_ranks[s];

    for (int h = 0; h < DDS_HANDS; h++) {
      r = trackp->play_ranks[h];
      s = trackp->play_suits[h];
      newp->removed_ranks[s] |= bit_map_rank[r];
    }
  }
}

auto Moves::MakeNext(const int trick, const int relHand,
                     const unsigned short ourWinRanks[DDS_SUITS])
    -> MoveType const * {
  // Find moves that are >= ourWinRanks[suit], but allow one
  // "small" move per suit to explore losing options.
  //
  // The lowest_win array tracks the minimum rank to try next for each suit.
  // After trying one card below the winning threshold, subsequent cards
  // must meet the threshold.

  int *lwp = track[trick].lowest_win[relHand];
  MovePlyType &list = moveList[trick][relHand];
  trackp = &track[trick];

  MoveType *currp = nullptr, *prevp;

  bool found = false;
  if (list.last == -1)
    return nullptr;
  else if (list.current == 0) {
    currp = &list.move[0];
    found = true;
  } else {
    prevp = &list.move[list.current - 1];
    if (lwp[prevp->suit] == 0) {
      int low = lowest_rank[ourWinRanks[prevp->suit]];
      if (low == 0)
        low = 15;
      if (prevp->rank < low)
        lwp[prevp->suit] = low;
    }

    while (list.current <= list.last && !found) {
      currp = &list.move[list.current];
      if (currp->rank >= lwp[currp->suit])
        found = true;
      else
        list.current++;
    }

    if (!found)
      return nullptr;
  }

  if (relHand == 0) {
    trackp->move[0].suit = currp->suit;
    trackp->move[0].rank = currp->rank;
    trackp->move[0].sequence = currp->sequence;
    trackp->high[0] = 0;

    trackp->lead_suit = currp->suit;
  } else if (currp->suit == trackp->move[relHand - 1].suit) {
    if (currp->rank > trackp->move[relHand - 1].rank) {
      trackp->move[relHand].suit = currp->suit;
      trackp->move[relHand].rank = currp->rank;
      trackp->move[relHand].sequence = currp->sequence;
      trackp->high[relHand] = relHand;
    } else {
      trackp->move[relHand] = trackp->move[relHand - 1];
      trackp->high[relHand] = trackp->high[relHand - 1];
    }
  } else if (currp->suit == trump) {
    trackp->move[relHand].suit = currp->suit;
    trackp->move[relHand].rank = currp->rank;
    trackp->move[relHand].sequence = currp->sequence;
    trackp->high[relHand] = relHand;
  } else {
    trackp->move[relHand] = trackp->move[relHand - 1];
    trackp->high[relHand] = trackp->high[relHand - 1];
  }

  trackp->play_suits[relHand] = currp->suit;
  trackp->play_ranks[relHand] = currp->rank;

  if (relHand == 3) {
    TrackType &newt = track[trick - 1];

    newt.lead_hand = (trackp->lead_hand + trackp->high[3]) % 4;

    int r, s;
    for (s = 0; s < DDS_SUITS; s++)
      newt.removed_ranks[s] = trackp->removed_ranks[s];

    for (int h = 0; h < DDS_HANDS; h++) {
      r = trackp->play_ranks[h];
      s = trackp->play_suits[h];
      newt.removed_ranks[s] |= bit_map_rank[r];
    }
  }

  list.current++;
  return currp;
}

auto Moves::MakeNextSimple(const int trick, const int relHand)
    -> MoveType const * {
  // Don't worry about small moves. Why not, actually?

  MovePlyType &list = moveList[trick][relHand];
  if (list.current > list.last)
    return nullptr;

  const MoveType &curr = list.move[list.current];

  trackp = &track[trick];

  if (relHand == 0) {
    trackp->move[0].suit = curr.suit;
    trackp->move[0].rank = curr.rank;
    trackp->move[0].sequence = curr.sequence;
    trackp->high[0] = 0;

    trackp->lead_suit = curr.suit;
  } else if (curr.suit == trackp->move[relHand - 1].suit) {
    if (curr.rank > trackp->move[relHand - 1].rank) {
      trackp->move[relHand].suit = curr.suit;
      trackp->move[relHand].rank = curr.rank;
      trackp->move[relHand].sequence = curr.sequence;
      trackp->high[relHand] = relHand;
    } else {
      trackp->move[relHand] = trackp->move[relHand - 1];
      trackp->high[relHand] = trackp->high[relHand - 1];
    }
  } else if (curr.suit == trump) {
    trackp->move[relHand].suit = curr.suit;
    trackp->move[relHand].rank = curr.rank;
    trackp->move[relHand].sequence = curr.sequence;
    trackp->high[relHand] = relHand;
  } else {
    trackp->move[relHand] = trackp->move[relHand - 1];
    trackp->high[relHand] = trackp->high[relHand - 1];
  }

  trackp->play_suits[relHand] = curr.suit;
  trackp->play_ranks[relHand] = curr.rank;

  if (relHand == 3) {
    track[trick - 1].lead_hand = (trackp->lead_hand + trackp->high[3]) % 4;
  }

  list.current++;
  return &curr;
}

auto Moves::Step(const int tricks, const int relHand) -> void {
  moveList[tricks][relHand].current++;
}

auto Moves::Rewind(const int tricks, const int relHand) -> void {
  moveList[tricks][relHand].current = 0;
}

auto Moves::Purge(const int trick, const int ourLeadHand,
                  const MoveType forbiddenMoves[]) -> void {
  MovePlyType &ourMply = moveList[trick][ourLeadHand];

  for (int k = 1; k <= 13; k++) {
    int s = forbiddenMoves[k].suit;
    int rank = forbiddenMoves[k].rank;
    if (rank == 0)
      continue;

    for (int r = 0; r <= ourMply.last; r++) {
      if (s == ourMply.move[r].suit && rank == ourMply.move[r].rank) {
        /* For the forbidden move r: */
        for (int n = r; n <= ourMply.last; n++)
          ourMply.move[n] = ourMply.move[n + 1];
        ourMply.last--;
      }
    }
  }
}

auto Moves::Reward(const int tricks, const int relHand) -> void {
  moveList[tricks][relHand]
      .move[moveList[tricks][relHand].current - 1]
      .weight += 100;
}

auto Moves::GetTrickData(const int tricks) -> const TrickDataType & {
  TrickDataType &data = track[tricks].trick_data;
  for (int s = 0; s < DDS_SUITS; s++)
    data.play_count[s] = 0;
  for (int relh = 0; relh < DDS_HANDS; relh++)
    data.play_count[trackp->play_suits[relh]]++;

#ifndef NDEBUG
  int sum = 0;
  for (int s = 0; s < DDS_SUITS; s++)
    sum += data.play_count[s];

  // Internal invariant: exactly 4 cards must be played per trick
  assert(sum == 4 && "GetTrickData: play_count sum must equal 4");
#endif

  data.best_rank = trackp->move[3].rank;
  data.best_suit = trackp->move[3].suit;
  data.best_sequence = trackp->move[3].sequence;
  data.rel_winner = trackp->high[3];
  return data;
}

auto Moves::Sort(const int tricks, const int relHand) -> void {
  numMoves = moveList[tricks][relHand].last + 1;
  mply = moveList[tricks][relHand].move;
  Moves::MergeSort();
}

#define CMP_SWAP(i, j)                                                         \
  if (mply[i].weight < mply[j].weight) {                                       \
    tmp = mply[i];                                                             \
    mply[i] = mply[j];                                                         \
    mply[j] = tmp;                                                             \
  }

/**
 * @brief Build a heuristic context and invoke heuristic sorting.
 *
 * Centralizes the construction of HeuristicContext to keep the call sites
 * consistent and avoid exposing mutable state to heuristic helpers.
 */
auto Moves::call_heuristic(const Pos &tpos, const MoveType &best_move,
                          const MoveType &best_move_tt,
                          const RelRanksType thrp_rel[]) -> void {
  // Construct context once here and call the context-taking overload.
  HeuristicContext context{
      tpos,  best_move, best_move_tt, thrp_rel,  mply,     numMoves, lastNumMoves,
      trump, suit,     trackp,     currTrick, currHand, leadHand, leadSuit};

  // Snapshot removed_ranks into the context to avoid direct dependence on
  // the mutable Moves::trackp buffer inside heuristic code.
  for (int s = 0; s < DDS_SUITS; ++s) {
    context.removed_ranks[s] = trackp ? trackp->removed_ranks[s] : 0;
  }
  // Snapshot minimal trick state for helper usage.
  context.move1_rank = (trackp ? trackp->move[1].rank : 0);
  context.high1 = (trackp ? trackp->high[1] : 0);
  context.move1_suit = (trackp ? trackp->move[1].suit : 0);
  // Third-hand snapshots
  context.move2_rank = (trackp ? trackp->move[2].rank : 0);
  context.move2_suit = (trackp ? trackp->move[2].suit : 0);
  context.high2 = (trackp ? trackp->high[2] : 0);
  // Leader snapshot
  context.lead0_rank = (trackp ? trackp->move[0].rank : 0);

  ::call_heuristic(context);
}

/**
 * @brief Sort move list by weight using small fixed-size sorting networks.
 *
 * Optimized for short lists typical in move generation. Falls back to
 * insertion sort for other sizes.
 */
auto Moves::MergeSort() -> void {
  const int len = numMoves;
  MoveType tmp;

  switch (len) {
  case 12:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(4, 5);
    CMP_SWAP(6, 7);
    CMP_SWAP(8, 9);
    CMP_SWAP(10, 11);

    CMP_SWAP(1, 3);
    CMP_SWAP(5, 7);
    CMP_SWAP(9, 11);

    CMP_SWAP(0, 2);
    CMP_SWAP(4, 6);
    CMP_SWAP(8, 10);

    CMP_SWAP(1, 2);
    CMP_SWAP(5, 6);
    CMP_SWAP(9, 10);

    CMP_SWAP(1, 5);
    CMP_SWAP(6, 10);
    CMP_SWAP(5, 9);
    CMP_SWAP(2, 6);
    CMP_SWAP(1, 5);
    CMP_SWAP(6, 10);
    CMP_SWAP(0, 4);
    CMP_SWAP(7, 11);
    CMP_SWAP(3, 7);
    CMP_SWAP(4, 8);
    CMP_SWAP(0, 4);
    CMP_SWAP(7, 11);
    CMP_SWAP(1, 4);
    CMP_SWAP(7, 10);
    CMP_SWAP(3, 8);
    CMP_SWAP(2, 3);
    CMP_SWAP(8, 9);
    CMP_SWAP(2, 4);
    CMP_SWAP(7, 9);
    CMP_SWAP(3, 5);
    CMP_SWAP(6, 8);
    CMP_SWAP(3, 4);
    CMP_SWAP(5, 6);
    CMP_SWAP(7, 8);
    break;
  case 11:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(4, 5);
    CMP_SWAP(6, 7);
    CMP_SWAP(8, 9);

    CMP_SWAP(1, 3);
    CMP_SWAP(5, 7);
    CMP_SWAP(0, 2);
    CMP_SWAP(4, 6);
    CMP_SWAP(8, 10);
    CMP_SWAP(1, 2);
    CMP_SWAP(5, 6);
    CMP_SWAP(9, 10);
    CMP_SWAP(1, 5);
    CMP_SWAP(6, 10);
    CMP_SWAP(5, 9);
    CMP_SWAP(2, 6);
    CMP_SWAP(1, 5);
    CMP_SWAP(6, 10);
    CMP_SWAP(0, 4);
    CMP_SWAP(3, 7);
    CMP_SWAP(4, 8);
    CMP_SWAP(0, 4);
    CMP_SWAP(1, 4);
    CMP_SWAP(7, 10);
    CMP_SWAP(3, 8);
    CMP_SWAP(2, 3);
    CMP_SWAP(8, 9);
    CMP_SWAP(2, 4);
    CMP_SWAP(7, 9);
    CMP_SWAP(3, 5);
    CMP_SWAP(6, 8);
    CMP_SWAP(3, 4);
    CMP_SWAP(5, 6);
    CMP_SWAP(7, 8);
    break;
  case 10:
    CMP_SWAP(1, 8);
    CMP_SWAP(0, 4);
    CMP_SWAP(5, 9);
    CMP_SWAP(2, 6);
    CMP_SWAP(3, 7);
    CMP_SWAP(0, 3);
    CMP_SWAP(6, 9);
    CMP_SWAP(2, 5);
    CMP_SWAP(0, 1);
    CMP_SWAP(3, 6);
    CMP_SWAP(8, 9);
    CMP_SWAP(4, 7);
    CMP_SWAP(0, 2);
    CMP_SWAP(4, 8);
    CMP_SWAP(1, 5);
    CMP_SWAP(7, 9);

    CMP_SWAP(1, 2);
    CMP_SWAP(3, 4);
    CMP_SWAP(5, 6);
    CMP_SWAP(7, 8);

    CMP_SWAP(1, 3);
    CMP_SWAP(6, 8);
    CMP_SWAP(2, 4);
    CMP_SWAP(5, 7);
    CMP_SWAP(2, 3);
    CMP_SWAP(6, 7);
    CMP_SWAP(3, 5);
    CMP_SWAP(4, 6);
    CMP_SWAP(4, 5);
    break;
  case 9:
    CMP_SWAP(0, 1);
    CMP_SWAP(3, 4);
    CMP_SWAP(6, 7);
    CMP_SWAP(1, 2);
    CMP_SWAP(4, 5);
    CMP_SWAP(7, 8);
    CMP_SWAP(0, 1);
    CMP_SWAP(3, 4);
    CMP_SWAP(6, 7);
    CMP_SWAP(0, 3);
    CMP_SWAP(3, 6);
    CMP_SWAP(0, 3);
    CMP_SWAP(1, 4);
    CMP_SWAP(4, 7);
    CMP_SWAP(1, 4);
    CMP_SWAP(2, 5);
    CMP_SWAP(5, 8);
    CMP_SWAP(2, 5);
    CMP_SWAP(1, 3);
    CMP_SWAP(5, 7);
    CMP_SWAP(2, 6);
    CMP_SWAP(4, 6);
    CMP_SWAP(2, 4);
    CMP_SWAP(2, 3);
    CMP_SWAP(5, 6);
    break;
  case 8:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(4, 5);
    CMP_SWAP(6, 7);

    CMP_SWAP(0, 2);
    CMP_SWAP(4, 6);
    CMP_SWAP(1, 3);
    CMP_SWAP(5, 7);

    CMP_SWAP(1, 2);
    CMP_SWAP(5, 6);
    CMP_SWAP(0, 4);
    CMP_SWAP(1, 5);

    CMP_SWAP(2, 6);
    CMP_SWAP(3, 7);
    CMP_SWAP(2, 4);
    CMP_SWAP(3, 5);

    CMP_SWAP(1, 2);
    CMP_SWAP(3, 4);
    CMP_SWAP(5, 6);
    break;
  case 7:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(4, 5);
    CMP_SWAP(0, 2);
    CMP_SWAP(4, 6);
    CMP_SWAP(1, 3);
    CMP_SWAP(1, 2);
    CMP_SWAP(5, 6);
    CMP_SWAP(0, 4);
    CMP_SWAP(1, 5);
    CMP_SWAP(2, 6);
    CMP_SWAP(2, 4);
    CMP_SWAP(3, 5);
    CMP_SWAP(1, 2);
    CMP_SWAP(3, 4);
    CMP_SWAP(5, 6);
    break;
  case 6:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(4, 5);
    CMP_SWAP(0, 2);
    CMP_SWAP(1, 3);
    CMP_SWAP(1, 2);
    CMP_SWAP(0, 4);
    CMP_SWAP(1, 5);
    CMP_SWAP(2, 4);
    CMP_SWAP(3, 5);
    CMP_SWAP(1, 2);
    CMP_SWAP(3, 4);
    break;
  case 5:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(0, 2);
    CMP_SWAP(1, 3);
    CMP_SWAP(1, 2);
    CMP_SWAP(0, 4);
    CMP_SWAP(2, 4);
    CMP_SWAP(1, 2);
    CMP_SWAP(3, 4);
    break;
  case 4:
    CMP_SWAP(0, 1);
    CMP_SWAP(2, 3);
    CMP_SWAP(0, 2);
    CMP_SWAP(1, 3);
    CMP_SWAP(1, 2);
    break;
  case 3:
    CMP_SWAP(0, 1);
    CMP_SWAP(0, 2);
    CMP_SWAP(1, 2);
    break;
  case 2:
    CMP_SWAP(0, 1);
    break;
  default: {
    for (int i = 1; i < len; i++) {
      tmp = mply[i];
      int j = i;
      for (; j && tmp.weight > mply[j - 1].weight; --j)
        mply[j] = mply[j - 1];
      mply[j] = tmp;
    }
  }
  }
  return;
}

auto Moves::PrintMove(const MovePlyType &ourMply) const -> string {
  stringstream ss;

  ss << "current " << ourMply.current << ", last " << ourMply.last << "\n";
  ss << " i suit sequence rank wgt\n";
  for (int i = 0; i <= ourMply.last; i++) {
    ss << setw(2) << right << i << setw(3) << card_suit[ourMply.move[i].suit]
       << setw(9) << hex << ourMply.move[i].sequence << setw(3)
       << card_rank[ourMply.move[i].rank] << setw(3) << ourMply.move[i].weight
       << "\n";
  }
  return ss.str();
}

auto Moves::PrintMoves(const int trick, const int relHand) const -> string {
  const MovePlyType &list = moveList[trick][relHand];

  const string st = "trick " + to_string(trick) + " relHand " +
                    to_string(relHand) + " last " + to_string(list.last) +
                    " current " + to_string(list.current) + "\n";

  return st + Moves::PrintMove(list);
}

auto Moves::TrickToText(const int trick) const -> string {
  const MovePlyType &listp0 = moveList[trick][0];
  const MovePlyType &listp1 = moveList[trick][1];
  const MovePlyType &listp2 = moveList[trick][2];
  const MovePlyType &listp3 = moveList[trick][3];

  stringstream ss;
  ss << setw(16) << left << "Last trick" << card_hand[track[trick].lead_hand]
     << ": " << card_suit[listp0.move[listp0.current].suit]
     << card_rank[listp0.move[listp0.current].rank] << " - "
     << card_suit[listp1.move[listp1.current].suit]
     << card_rank[listp1.move[listp1.current].rank] << " - "
     << card_suit[listp2.move[listp2.current].suit]
     << card_rank[listp2.move[listp2.current].rank] << " - "
     << card_suit[listp3.move[listp3.current].suit]
     << card_rank[listp3.move[listp3.current].rank] << "\n";

  return ss.str();
}

auto Moves::UpdateStatsEntry(moveStatsType &stat, const int findex,
                             const int hit, const int len) const -> void {
  bool found = false;
  int fno = 0;
  for (int i = 0; i < stat.nfuncs; i++) {
    if (stat.list[i].findex == findex) {
      found = true;
      fno = i;
      break;
    }
  }

  moveStatType *funp;
  if (found) {
    funp = &stat.list[fno];
    funp->count++;
    funp->sumHits += hit;
    funp->sumLengths += len;
  } else {
    // Internal invariant: nfuncs must not exceed array size
    assert(stat.nfuncs < static_cast<int>(MgType::SIZE) &&
           "UpdateStatsEntry: nfuncs overflow");

    funp = &stat.list[stat.nfuncs++];

    funp->count++;
    funp->findex = findex;
    funp->sumHits += hit;
    funp->sumLengths += len;
  }
}

auto Moves::RegisterHit(const int trick, const int relHand) -> void {
  const MovePlyType &list = moveList[trick][relHand];

  const int findex = static_cast<int>(lastCall[trick][relHand]);
  const int len = list.last + 1;

  // Internal invariant: lastCall must be initialized before RegisterHit
  assert(findex != -1 && "RegisterHit: lastCall not initialized");

  const int curr = list.current;
  // Internal invariant: current must be within valid range [1, len]
  assert(curr >= 1 && curr <= len && "RegisterHit: current out of bounds");

  const int moveSuit = list.move[curr - 1].suit;
  int numSuit = 0;
  int numSeen = 0;

  for (int i = 0; i < len; i++) {
    if (list.move[i].suit == moveSuit) {
      numSuit++;
      if (i == curr - 1)
        numSeen = numSuit;
    }
  }

  // Now we know enough to update the statistics tables.

  trickTable[trick][relHand].count++;
  trickTable[trick][relHand].sumHits += curr;
  trickTable[trick][relHand].sumLengths += len;

  trickSuitTable[trick][relHand].count++;
  trickSuitTable[trick][relHand].sumHits += numSeen;
  trickSuitTable[trick][relHand].sumLengths += numSuit;

  Moves::UpdateStatsEntry(trickDetailTable[trick][relHand], findex, curr, len);

  Moves::UpdateStatsEntry(trickDetailSuitTable[trick][relHand], findex, numSeen,
                          numSuit);

  Moves::UpdateStatsEntry(trickFuncTable, findex, curr, len);

  Moves::UpdateStatsEntry(trickFuncSuitTable, findex, numSeen, numSuit);
}

auto Moves::AverageString(const moveStatType &stat) const -> string {
  stringstream ss;
  if (stat.count == 0)
    ss << setw(5) << right << "--" << setw(5) << "--";
  else {
    ss << setw(5) << setprecision(2) << fixed
       << stat.sumHits / static_cast<double>(stat.count) << setw(5)
       << setprecision(1) << fixed
       << 100. * stat.sumHits / static_cast<double>(stat.sumLengths);
  }

  return ss.str();
}

auto Moves::FullAverageString(const moveStatType &stat) const -> string {
  stringstream ss;
  if (stat.count == 0) {
    ss << setw(6) << right << "--" << setw(6) << "--" << setw(5) << "--"
       << setw(9) << "--" << setw(5) << "--";
  } else {
    double avg = stat.sumHits / static_cast<double>(stat.count);

    ss << setw(5) << setprecision(3) << fixed << avg << setw(6)
       << setprecision(2) << fixed
       << stat.sumLengths / static_cast<double>(stat.count) << setw(5)
       << setprecision(1) << fixed
       << 100. * stat.sumHits / static_cast<double>(stat.sumLengths) << setw(9)
       << stat.count << setprecision(0) << fixed
       << (avg * avg * avg - 1) * stat.count;
  }

  return ss.str();
}

auto Moves::PrintTrickTable(const moveStatType tablep[][DDS_HANDS]) const
    -> string {
  stringstream ss;

  ss << setw(5) << "Trick" << setw(12) << "Hand 0" << setw(12) << "Hand 1"
     << setw(12) << "Hand 2" << setw(12) << "Hand 3" << "\n";

  ss << setw(6) << "" << setw(6) << "Avg" << setw(5) << "%" << setw(6) << "Avg"
     << setw(5) << "%" << setw(6) << "Avg" << setw(5) << "%" << setw(6) << "Avg"
     << setw(5) << "%" << "\n";

  for (int t = 12; t >= 0; t--) {
    ss << setw(5) << right << t << setw(12)
       << Moves::AverageString(tablep[t][0]) << setw(12)
       << Moves::AverageString(tablep[t][1]) << setw(12)
       << Moves::AverageString(tablep[t][2]) << setw(12)
       << Moves::AverageString(tablep[t][3]) << "\n";
  }
  return ss.str();
}

auto Moves::PrintFunctionTable(const moveStatsType &stat) const -> string {
  if (stat.nfuncs == 0)
    return "";

  stringstream ss;
  ss << setw(15) << left << "Function" << setw(6) << "Avg" << setw(6) << "Len"
     << setw(5) << "%" << setw(9) << "Count" << setw(9) << "Imp" << "\n";

  for (int fr = 0; fr < static_cast<int>(MgType::SIZE); fr++) {
    for (int f = 0; f < stat.nfuncs; f++) {
      if (stat.list[f].findex != fr)
        continue;

      ss << setw(15) << left << funcName[fr]
         << Moves::FullAverageString(stat.list[f]) << "\n";
    }
  }
  return ss.str();
}

auto Moves::PrintTrickStats(ofstream &fout) const -> void {
  fout << "Overall statistics\n\n";
  fout << Moves::PrintTrickTable(trickTable);

  fout << "\n\nStatistics for winning suit\n\n";
  fout << Moves::PrintTrickTable(trickSuitTable) << "\n\n";
}

auto Moves::PrintTrickDetails(ofstream &fout) const -> void {
  fout << "Trick detail statistics\n\n";

  for (int t = 12; t >= 0; t--) {
    for (int h = 0; h < DDS_HANDS; h++) {
      fout << "Trick " << t << ", relative hand " << h << "\n";
      fout << Moves::PrintFunctionTable(trickDetailTable[t][h]) << "\n";
    }
  }

  fout << "Suit detail statistics\n\n";

  for (int t = 12; t >= 0; t--) {
    for (int h = 0; h < DDS_HANDS; h++) {
      fout << "Trick " << t << ", relative hand " << h << "\n";
      fout << Moves::PrintFunctionTable(trickDetailSuitTable[t][h]) << "\n";
    }
  }

  fout << "\n\n";
}

auto Moves::PrintFunctionStats(ofstream &fout) const -> void {
  fout << "Function statistics\n\n";
  fout << Moves::PrintFunctionTable(trickFuncTable);

  fout << "\n\nFunction statistics for winning suit\n\n";
  fout << Moves::PrintFunctionTable(trickFuncSuitTable);
  fout << "\n\n";
}
