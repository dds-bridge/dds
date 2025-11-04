/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <iomanip>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <cstring>

#include "Moves.h"
#include "heuristic_sorting/heuristic_sorting.h"
// Optional arena-backed scratch support (wired via TLS by SolverContext)
#include "utility/ScratchAllocTLS.h"
#include <lookup_tables/LookupTables.h>

#ifdef DDS_MOVES
  #define MG_REGISTER(a, b) lastCall[currTrick][b] = a
  const MGtype RegisterList[16] =
    {
      MG_NT0, MG_TRUMP0,
      MG_SIZE, MG_SIZE, // Unused

      MG_NT_NOTVOID1, MG_TRUMP_NOTVOID1,
      MG_NT_VOID1, MG_TRUMP_VOID1,

      MG_NT_NOTVOID2, MG_TRUMP_NOTVOID2,
      MG_NT_VOID2, MG_TRUMP_VOID2,

      MG_COMB_NOTVOID3, MG_COMB_NOTVOID3,
      MG_NT_VOID3, MG_TRUMP_VOID3
    };
#else
  #define MG_REGISTER(a, b) 1;
#endif


Moves::Moves()
{
  funcName[MG_NT0] = "NT0";
  funcName[MG_TRUMP0] = "Trump0";
  funcName[MG_NT_VOID1] = "NT_Void1";
  funcName[MG_TRUMP_VOID1] = "Trump_Void1";
  funcName[MG_NT_NOTVOID1] = "NT_Notvoid1";
  funcName[MG_TRUMP_NOTVOID1] = "Trump_Notvoid1";
  funcName[MG_NT_VOID2] = "NT_Void2";
  funcName[MG_TRUMP_VOID2] = "Trump_Void2";
  funcName[MG_NT_NOTVOID2] = "NT_Notvoid2";
  funcName[MG_TRUMP_NOTVOID2] = "Trump_Notvoid2";
  funcName[MG_NT_VOID3] = "NT_Void3";
  funcName[MG_TRUMP_VOID3] = "Trump_Void3";
  funcName[MG_COMB_NOTVOID3] = "Comb_Notvoid3";

  for (int t = 0; t < 13; t++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      lastCall[t][h] = MG_SIZE;

      trickTable[t][h].count = 0;
      trickSuitTable[t][h].count = 0;

      trickDetailTable [t][h].nfuncs = 0;
      trickDetailSuitTable[t][h].nfuncs = 0;
      for (int i = 0; i < MG_SIZE; i++)
      {
        trickDetailTable [t][h].list[i].count = 0;
        trickDetailSuitTable[t][h].list[i].count = 0;
      }
    }
  }

  trickFuncTable.nfuncs = 0;
  trickFuncSuitTable.nfuncs = 0;
  for (int i = 0; i < MG_SIZE; i++)
  {
    trickFuncTable .list[i].count = 0;
    trickFuncSuitTable.list[i].count = 0;
  }
}

Moves::~Moves()
{
}


void Moves::Init(
  const int tricks,
  const int relStartHand,
  const int initialRanks[],
  const int initialSuits[],
  const unsigned short rankInSuit[DDS_HANDS][DDS_SUITS],
  const int ourTrump,
  const int ourLeadHand)
{
  currTrick = tricks;
  trump = ourTrump;

  if (relStartHand == 0)
    track[tricks].leadHand = ourLeadHand;

  for (int m = 0; m < 13; m++)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      moveList[m][h].current = 0;
      moveList[m][h].last = 0;

    }
  }

  // 0x1ffff would be enough, but this is for compatibility.
  for (int s = 0; s < DDS_SUITS; s++)
    track[tricks].removedRanks[s] = 0xffff;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      track[tricks].removedRanks[s] ^= rankInSuit[h][s];

  for (int n = 0; n < relStartHand; n++)
  {
    int s = initialSuits[n];
    int r = initialRanks[n];

    track[tricks].removedRanks[s] ^= bitMapRank[r];
  }
}


void Moves::Reinit(
  const int tricks,
  const int ourLeadHand)
{
  track[tricks].leadHand = ourLeadHand;
}


int Moves::MoveGen0(
  const int tricks,
  const pos& tpos,
  const moveType& bestMove,
  const moveType& bestMoveTT,
  const relRanksType thrp_rel[])
{
  trackp = &track[tricks];
  leadHand = trackp->leadHand;
  currHand = leadHand;
  currTrick = tricks;

  const MoveGroupType * mp;
  int removed, g, rank, seq;

  movePlyType& list = moveList[tricks][0];
  mply = list.move;
  for (int s = 0; s < DDS_SUITS; s++)
    trackp->lowestWin[0][s] = 0;
  numMoves = 0;

  for (suit = 0; suit < DDS_SUITS; suit++)
  {
    unsigned short ris = tpos.rankInSuit[leadHand][suit];
    if (ris == 0) continue;

    lastNumMoves = numMoves;
    mp = &group_data[ris];
    g = mp->last_group_;
    removed = trackp->removedRanks[suit];

    while (g >= 0)
    {
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

    Moves::CallHeuristic(tpos, bestMove, bestMoveTT, thrp_rel);
  }

#ifdef DDS_MOVES
  bool ftest = ((trump != DDS_NOTRUMP) &&
                (tpos.winner[trump].rank != 0));
  if (ftest)
    MG_REGISTER(MG_TRUMP0, 0);
  else
    MG_REGISTER(MG_NT0, 0);
#endif

  list.current = 0;
  list.last = numMoves - 1;

#ifdef DDS_SKIP_HEURISTIC
  return numMoves;
#endif
  if (numMoves != 1)
    Moves::MergeSort();
  return numMoves;
}


int Moves::MoveGen123(
  const int tricks,
  const int handRel,
  const pos& tpos)
{
  trackp = &track[tricks];
  leadHand = trackp->leadHand;
  currHand = handId(leadHand, handRel);
  currTrick = tricks;
  leadSuit = track[tricks].leadSuit;

  const MoveGroupType * mp;
  int removed, g, rank, seq;

  movePlyType& list = moveList[tricks][handRel];
  mply = list.move;

  for (int s = 0; s < DDS_SUITS; s++)
    trackp->lowestWin[handRel][s] = 0;
  numMoves = 0;

  [[maybe_unused]] int findex;
  int ftest = ((trump != DDS_NOTRUMP) &&
               (tpos.winner[trump].rank != 0) ? 1 : 0);

  unsigned short ris = tpos.rankInSuit[currHand][leadSuit];

  if (ris != 0)
  {
    mp = &group_data[ris];
    g = mp->last_group_;
    removed = trackp->removedRanks[leadSuit];

    while (g >= 0)
    {
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
#ifdef DDS_SKIP_HEURISTIC
  return numMoves;
#endif
      Moves::CallHeuristic(tpos, moveType{}, moveType{}, nullptr);

    Moves::MergeSort();
    return numMoves;
  }

  findex = 4 * handRel + ftest + 2;

#ifdef DDS_MOVES
  MG_REGISTER(RegisterList[findex], handRel);
#endif

  for (suit = 0; suit < DDS_SUITS; suit++)
  {
    ris = tpos.rankInSuit[currHand][suit];
    if (ris == 0) continue;

    lastNumMoves = numMoves;
    mp = &group_data[ris];
    g = mp->last_group_;
    removed = trackp->removedRanks[suit];

    while (g >= 0)
    {
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

    Moves::CallHeuristic(tpos, moveType{}, moveType{}, nullptr);
  }

  list.current = 0;
  list.last = numMoves - 1;
#ifdef DDS_SKIP_HEURISTIC
    return numMoves;
#endif
  if (numMoves != 1)
    Moves::MergeSort();
  return numMoves;
}

void Moves::GetTopNumber(
  const int ris,
  const int prank,
  int& topNumber,
  int& mno) const
{
  topNumber = -10;

  // Find the lowest move that still overtakes partner's card.
  mno = 0;
  while (mno < numMoves - 1 && mply[1 + mno].rank > prank)
    mno++;

  const MoveGroupType& mp = group_data[ris];
  int g = mp.last_group_;

  // Remove partner's card as well.
  int removed = static_cast<int>(trackp->removedRanks[leadSuit] |
                                 bitMapRank[prank]);

  int fullseq = mp.fullseq_[g];

  while (g >= 1 && ((mp.gap_[g] & removed) == mp.gap_[g]))
    fullseq |= mp.fullseq_[--g];

  topNumber = count_table[fullseq] - 1;
}


inline bool Moves::WinningMove(
  const moveType& mvp1,
  const extCard& mvp2,
  const int ourTrump) const
{
  /* Return true if move 1 wins over move 2, with the assumption that
  move 2 is the presently winning card of the trick */

  if (mvp1.suit == mvp2.suit)
  {
    if (mvp1.rank > mvp2.rank)
      return true;
    else
      return false;
  }
  else if (mvp1.suit == ourTrump)
    return true;
  else
    return false;
}


int Moves::GetLength(
  const int trick,
  const int relHand) const
{
  return moveList[trick][relHand].last + 1;
}


void Moves::MakeSpecific(
  const moveType& ourMply,
  const int trick,
  const int relHand)
{
  trackp = &track[trick];

  if (relHand == 0)
  {
    trackp->move[0].suit = ourMply.suit;
    trackp->move[0].rank = ourMply.rank;
    trackp->move[0].sequence = ourMply.sequence;
    trackp->high[0] = 0;

    trackp->leadSuit = ourMply.suit;
  }
  else if (ourMply.suit == trackp->move[relHand - 1].suit)
  {
    if (ourMply.rank > trackp->move[relHand - 1].rank)
    {
      trackp->move[relHand].suit = ourMply.suit;
      trackp->move[relHand].rank = ourMply.rank;
      trackp->move[relHand].sequence = ourMply.sequence;
      trackp->high[relHand] = relHand;
    }
    else
    {
      trackp->move[relHand] = trackp->move[relHand - 1];
      trackp->high[relHand] = trackp->high[relHand - 1];
    }
  }
  else if (ourMply.suit == trump)
  {
    trackp->move[relHand].suit = ourMply.suit;
    trackp->move[relHand].rank = ourMply.rank;
    trackp->move[relHand].sequence = ourMply.sequence;
    trackp->high[relHand] = relHand;
  }
  else
  {
    trackp->move[relHand] = trackp->move[relHand - 1];
    trackp->high[relHand] = trackp->high[relHand - 1];
  }

  trackp->playSuits[relHand] = ourMply.suit;
  trackp->playRanks[relHand] = ourMply.rank;

  if (relHand == 3)
  {
    trackType * newp = &track[trick - 1];

    newp->leadHand = (trackp->leadHand + trackp->high[3]) % 4;

    int r, s;
    for (s = 0; s < DDS_SUITS; s++)
      newp->removedRanks[s] = trackp->removedRanks[s];

    for (int h = 0; h < DDS_HANDS; h++)
    {
      r = trackp->playRanks[h];
      s = trackp->playSuits[h];
      newp->removedRanks[s] |= bitMapRank[r];
    }
  }
}


moveType const * Moves::MakeNext(
  const int trick,
  const int relHand,
  const unsigned short ourWinRanks[DDS_SUITS])
{
  // Find moves that are >= ourWinRanks[suit], but allow one
  // "small" move per suit.

  int * lwp = track[trick].lowestWin[relHand];
  movePlyType& list = moveList[trick][relHand];
  trackp = &track[trick];

  moveType * currp = nullptr, * prevp;

  bool found = false;
  if (list.last == -1)
    return NULL;
  else if (list.current == 0)
  {
    currp = &list.move[0];
    found = true;
  }
  else
  {
    prevp = &list.move[ list.current - 1 ];
    if (lwp[ prevp->suit ] == 0)
    {
      int low = lowest_rank[ ourWinRanks[prevp->suit] ];
      if (low == 0)
        low = 15;
      if (prevp->rank < low)
        lwp[ prevp->suit ] = low;
    }

    while (list.current <= list.last && ! found)
    {
      currp = &list.move[ list.current ];
      if (currp->rank >= lwp[ currp->suit ])
        found = true;
      else
        list.current++;
    }

    if (! found)
      return NULL;
  }

  if (relHand == 0)
  {
    trackp->move[0].suit = currp->suit;
    trackp->move[0].rank = currp->rank;
    trackp->move[0].sequence = currp->sequence;
    trackp->high[0] = 0;

    trackp->leadSuit = currp->suit;
  }
  else if (currp->suit == trackp->move[relHand - 1].suit)
  {
    if (currp->rank > trackp->move[relHand - 1].rank)
    {
      trackp->move[relHand].suit = currp->suit;
      trackp->move[relHand].rank = currp->rank;
      trackp->move[relHand].sequence = currp->sequence;
      trackp->high[relHand] = relHand;
    }
    else
    {
      trackp->move[relHand] = trackp->move[relHand - 1];
      trackp->high[relHand] = trackp->high[relHand - 1];
    }
  }
  else if (currp->suit == trump)
  {
    trackp->move[relHand].suit = currp->suit;
    trackp->move[relHand].rank = currp->rank;
    trackp->move[relHand].sequence = currp->sequence;
    trackp->high[relHand] = relHand;
  }
  else
  {
    trackp->move[relHand] = trackp->move[relHand - 1];
    trackp->high[relHand] = trackp->high[relHand - 1];
  }

  trackp->playSuits[relHand] = currp->suit;
  trackp->playRanks[relHand] = currp->rank;

  if (relHand == 3)
  {
    trackType& newt = track[trick - 1];

    newt.leadHand = (trackp->leadHand + trackp->high[3]) % 4;

    int r, s;
    for (s = 0; s < DDS_SUITS; s++)
      newt.removedRanks[s] = trackp->removedRanks[s];

    for (int h = 0; h < DDS_HANDS; h++)
    {
      r = trackp->playRanks[h];
      s = trackp->playSuits[h];
      newt.removedRanks[s] |= bitMapRank[r];
    }
  }

  list.current++;
  return currp;
}


moveType const * Moves::MakeNextSimple(
  const int trick,
  const int relHand)
{
  // Don't worry about small moves. Why not, actually?

  movePlyType& list = moveList[trick][relHand];
  if (list.current > list.last)
    return NULL;

  const moveType& curr = list.move[list.current];

  trackp = &track[trick];

  if (relHand == 0)
  {
    trackp->move[0].suit = curr.suit;
    trackp->move[0].rank = curr.rank;
    trackp->move[0].sequence = curr.sequence;
    trackp->high[0] = 0;

    trackp->leadSuit = curr.suit;
  }
  else if (curr.suit == trackp->move[relHand-1].suit)
  {
    if (curr.rank > trackp->move[relHand-1].rank)
    {
      trackp->move[relHand].suit = curr.suit;
      trackp->move[relHand].rank = curr.rank;
      trackp->move[relHand].sequence = curr.sequence;
      trackp->high[relHand] = relHand;
    }
    else
    {
      trackp->move[relHand] = trackp->move[relHand-1];
      trackp->high[relHand] = trackp->high[relHand-1];
    }
  }
  else if (curr.suit == trump)
  {
    trackp->move[relHand].suit = curr.suit;
    trackp->move[relHand].rank = curr.rank;
    trackp->move[relHand].sequence = curr.sequence;
    trackp->high[relHand] = relHand;
  }
  else
  {
    trackp->move[relHand] = trackp->move[relHand-1];
    trackp->high[relHand] = trackp->high[relHand-1];
  }

  trackp->playSuits[relHand] = curr.suit;
  trackp->playRanks[relHand] = curr.rank;

  if (relHand == 3)
  {
    track[trick-1].leadHand = (trackp->leadHand + trackp->high[3]) % 4;
  }

  list.current++;
  return &curr;
}


void Moves::Step(
  const int tricks,
  const int relHand)
{
  moveList[tricks][relHand].current++;
}


void Moves::Rewind(
  const int tricks,
  const int relHand)
{
  moveList[tricks][relHand].current = 0;
}


void Moves::Purge(
  const int trick,
  const int ourLeadHand,
  const moveType forbiddenMoves[])
{
  movePlyType& ourMply = moveList[trick][ourLeadHand];

  for (int k = 1; k <= 13; k++)
  {
    int s = forbiddenMoves[k].suit;
    int rank = forbiddenMoves[k].rank;
    if (rank == 0) continue;

    for (int r = 0; r <= ourMply.last; r++)
    {
      if (s == ourMply.move[r].suit &&
          rank == ourMply.move[r].rank)
      {
        /* For the forbidden move r: */
        for (int n = r; n <= ourMply.last; n++)
          ourMply.move[n] = ourMply.move[n + 1];
        ourMply.last--;
      }
    }
  }
}


void Moves::Reward(
  const int tricks,
  const int relHand)
{
  moveList[tricks][relHand].
  move[ moveList[tricks][relHand].current - 1 ].weight += 100;
}


const trickDataType& Moves::GetTrickData(const int tricks)
{
  trickDataType& data = track[tricks].trickData;
  for (int s = 0; s < DDS_SUITS; s++)
    data.playCount[s] = 0;
  for (int relh = 0; relh < DDS_HANDS; relh++)
    data.playCount[ trackp->playSuits[relh] ]++;

  int sum = 0;
  for (int s = 0; s < DDS_SUITS; s++)
    sum += data.playCount[s];

  if (sum != 4)
  {
    cout << "Sum " << sum << " is not four" << endl;
    exit(1);
  }

  data.bestRank = trackp->move[3].rank;
  data.bestSuit = trackp->move[3].suit;
  data.bestSequence = trackp->move[3].sequence;
  data.relWinner = trackp->high[3];
  return data;
}


void Moves::Sort(
  const int tricks,
  const int relHand)
{
  numMoves = moveList[tricks][relHand].last + 1;
  mply = moveList[tricks][relHand].move;
  Moves::MergeSort();
}


#define CMP_SWAP(i, j) if (mply[i].weight < mply[j].weight) \
  { tmp = mply[i]; mply[i] = mply[j]; mply[j] = tmp; }

void Moves::CallHeuristic(
    const pos& tpos,
    const moveType& bestMove,
    const moveType& bestMoveTT,
    const relRanksType thrp_rel[]) {
  // Construct context once here and call the context-taking overload.
  HeuristicContext context{
    tpos,
    bestMove,
    bestMoveTT,
    thrp_rel,
    mply,
    numMoves,
    lastNumMoves,
    trump,
    suit,
    trackp,
    currTrick,
    currHand,
    leadHand,
    leadSuit
  };

  // Snapshot removedRanks into the context to avoid direct dependence on
  // the mutable Moves::trackp buffer inside heuristic code.
  for (int s = 0; s < DDS_SUITS; ++s) {
    context.removedRanks[s] = trackp ? trackp->removedRanks[s] : 0;
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

  ::CallHeuristic(context);
}

void Moves::MergeSort()
{
  moveType tmp;

  switch (numMoves)
  {
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
    default:
    {
      for (int i = 1; i < numMoves; i++)
      {
        tmp = mply[i];
        int j = i;
        for (; j && tmp.weight > mply[j - 1].weight ; --j)
          mply[j] = mply[j - 1];
        mply[j] = tmp;
      }
    }
  }
  return;
}


string Moves::PrintMove(const movePlyType& ourMply) const
{
  stringstream ss;

  ss << "current " << ourMply.current << ", last " << ourMply.last << "\n";
  ss << " i suit sequence rank wgt\n";
  for (int i = 0; i <= ourMply.last; i++)
  {
    ss << setw(2) << right << i <<
      setw(3) << cardSuit[ ourMply.move[i].suit ] <<
      setw(9) << hex << ourMply.move[i].sequence <<
      setw(3) << cardRank[ ourMply.move[i].rank ] <<
      setw(3) << ourMply.move[i].weight << "\n";
  }
  return ss.str();
}


string Moves::PrintMoves(
  const int trick,
  const int relHand) const
{
  const movePlyType& list = moveList[trick][relHand];
  
  const string st = "trick " + to_string(trick) +
    " relHand " + to_string(relHand) +
    " last " + to_string(list.last) +
    " current " + to_string(list.current) + "\n";

  return st + Moves::PrintMove(list);
}


string Moves::TrickToText(const int trick) const
{
  const movePlyType& listp0 = moveList[trick][0];
  const movePlyType& listp1 = moveList[trick][1];
  const movePlyType& listp2 = moveList[trick][2];
  const movePlyType& listp3 = moveList[trick][3];

  stringstream ss;
  ss << setw(16) << left << "Last trick" << 
    cardHand[ track[trick].leadHand ] << ": " <<
    cardSuit[ listp0.move[listp0.current].suit ] <<
    cardRank[ listp0.move[listp0.current].rank ] << " - " <<
    cardSuit[ listp1.move[listp1.current].suit ] << 
    cardRank[ listp1.move[listp1.current].rank ] << " - " <<
    cardSuit[ listp2.move[listp2.current].suit ] << 
    cardRank[ listp2.move[listp2.current].rank ] << " - " <<
    cardSuit[ listp3.move[listp3.current].suit ] << 
    cardRank[ listp3.move[listp3.current].rank ] << "\n";

  return ss.str();
}



void Moves::UpdateStatsEntry(
  moveStatsType& stat,
  const int findex,
  const int hit,
  const int len) const
{
  bool found = false;
  int fno = 0;
  for (int i = 0; i < stat.nfuncs; i++)
  {
    if (stat.list[i].findex == findex)
    {
      found = true;
      fno = i;
      break;
    }
  }

  moveStatType * funp;
  if (found)
  {
    funp = &stat.list[fno];
    funp->count++;
    funp->sumHits += hit;
    funp->sumLengths += len;
  }
  else
  {
    if (stat.nfuncs >= MG_SIZE)
    {
      cout << "Shouldn't happen, " << stat.nfuncs << endl;
      for (int i = 0; i < stat.nfuncs; i++)
        cout << i << " " << stat.list[i].findex << "\n";
      exit(1);
    }

    funp = &stat.list[stat.nfuncs++];

    funp->count++;
    funp->findex = findex;
    funp->sumHits += hit;
    funp->sumLengths += len;
  }
}


void Moves::RegisterHit(
  const int trick,
  const int relHand)
{
  const movePlyType& list = moveList[trick][relHand];

  const int findex = lastCall[trick][relHand];
  const int len = list.last + 1;

  if (findex == -1)
  {
    cout << "RegisterHit trick " << trick << 
      " relHand " << relHand << " findex -1" << endl;
    exit(1);
  }

  const int curr = list.current;
  if (curr < 1 || curr > len)
  {
    cout << "current out of bounds" << endl;
    exit(1);
  }

  const int moveSuit = list.move[curr-1].suit;
  int numSuit = 0;
  int numSeen = 0;

  for (int i = 0; i < len; i++)
  {
    if (list.move[i].suit == moveSuit)
    {
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

  Moves::UpdateStatsEntry(trickDetailTable[trick][relHand],
    findex, curr, len);

  Moves::UpdateStatsEntry(trickDetailSuitTable[trick][relHand],
    findex, numSeen, numSuit);

  Moves::UpdateStatsEntry(trickFuncTable,
    findex, curr, len);

  Moves::UpdateStatsEntry(trickFuncSuitTable,
    findex, numSeen, numSuit);
}


string Moves::AverageString(const moveStatType& stat) const
{
  stringstream ss;
  if (stat.count == 0)
    ss << setw(5) << right << "--" << setw(5) << "--";
  else
  {
    ss << setw(5) << setprecision(2) << fixed <<
      stat.sumHits / static_cast<double>(stat.count) <<
      setw(5) << setprecision(1) << fixed <<
      100. * stat.sumHits / static_cast<double>(stat.sumLengths);
  }

  return ss.str();
}


string Moves::FullAverageString(const moveStatType& stat) const
{
  stringstream ss;
  if (stat.count == 0)
  {
    ss << setw(6) << right << "--" <<
      setw(6) << "--" <<
      setw(5) << "--" <<
      setw(9) << "--" <<
      setw(5) << "--";
  }
  else
  {
    double avg = stat.sumHits / static_cast<double>(stat.count);

    ss << setw(5) << setprecision(3) << fixed << avg <<
      setw(6) << setprecision(2) << fixed <<
        stat.sumLengths / static_cast<double>(stat.count) <<
      setw(5) << setprecision(1) << fixed <<
        100. * stat.sumHits / static_cast<double>(stat.sumLengths) <<
      setw(9) << stat.count << setprecision(0) << fixed <<
        (avg * avg * avg - 1) * stat.count;
  }

  return ss.str();
}


string Moves::PrintTrickTable(
  const moveStatType tablep[][DDS_HANDS]) const
{
  stringstream ss;

  ss << setw(5) << "Trick" <<
    setw(12) << "Hand 0" <<
    setw(12) << "Hand 1" <<
    setw(12) << "Hand 2" <<
    setw(12) << "Hand 3" << "\n";

  ss << setw(6) << "" <<
    setw(6) << "Avg" << setw(5) << "%" <<
    setw(6) << "Avg" << setw(5) << "%" <<
    setw(6) << "Avg" << setw(5) << "%" <<
    setw(6) << "Avg" << setw(5) << "%" << "\n";

  for (int t = 12; t >= 0; t--)
  {
    ss << setw(5) << right << t <<
      setw(12) << Moves::AverageString(tablep[t][0]) <<
      setw(12) << Moves::AverageString(tablep[t][1]) <<
      setw(12) << Moves::AverageString(tablep[t][2]) <<
      setw(12) << Moves::AverageString(tablep[t][3]) << "\n";
  }
  return ss.str();
}


string Moves::PrintFunctionTable(const moveStatsType& stat) const
{
  if (stat.nfuncs == 0)
    return "";

  stringstream ss;
  ss << setw(15) << left << "Function" <<
    setw(6) << "Avg" <<
    setw(6) << "Len" <<
    setw(5) << "%" <<
    setw(9) << "Count" <<
    setw(9) << "Imp" << "\n";

  for (int fr = 0; fr < MG_SIZE; fr++)
  {
    for (int f = 0; f < stat.nfuncs; f++)
    {
      if (stat.list[f].findex != fr)
        continue;

      ss << setw(15) << left << funcName[fr] <<
        Moves::FullAverageString(stat.list[f]) << "\n";
    }
  }
  return ss.str();
}


void Moves::PrintTrickStats(ofstream& fout) const
{
  fout << "Overall statistics\n\n";
  fout << Moves::PrintTrickTable(trickTable);

  fout << "\n\nStatistics for winning suit\n\n";
  fout << Moves::PrintTrickTable(trickSuitTable) << "\n\n";
}


void Moves::PrintTrickDetails(ofstream& fout) const
{
  fout << "Trick detail statistics\n\n";

  for (int t = 12; t >= 0; t--)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      fout << "Trick " << t << ", relative hand " << h << "\n";
      fout << Moves::PrintFunctionTable(trickDetailTable[t][h]) << "\n";
    }
  }

  fout << "Suit detail statistics\n\n";

  for (int t = 12; t >= 0; t--)
  {
    for (int h = 0; h < DDS_HANDS; h++)
    {
      fout << "Trick " << t << ", relative hand " << h << "\n";
      fout << Moves::PrintFunctionTable(trickDetailSuitTable[t][h]) << 
        "\n";
    }
  }

  fout << "\n\n";
}


void Moves::PrintFunctionStats(ofstream& fout) const
{
  fout << "Function statistics\n\n";
  fout << Moves::PrintFunctionTable(trickFuncTable);

  fout << "\n\nFunction statistics for winning suit\n\n";
  fout << Moves::PrintFunctionTable(trickFuncSuitTable);
  fout << "\n\n";
}

