/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <algorithm>

#include "LaterTricks.h"


bool LaterTricksMIN(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  const ThreadData& thrd)
{
  if ((trump == DDS_NOTRUMP) || (tpos.winner[trump].rank == 0))
  {
    int sum = 0;
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      int hh = tpos.winner[ss].hand;
      if (hh != -1)
      {
        if (thrd.nodeTypeStore[hh] == MAXNODE)
          sum += max(tpos.length[hh][ss],
                     tpos.length[partner[hh]][ss]);
      }
    }

    if ((tpos.tricksMAX + sum < target) && (sum > 0))
    {
      if ((tpos.tricksMAX + (depth >> 2) >= target))
        return true;

      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        int win_hand = tpos.winner[ss].hand;

        if (win_hand == -1)
          tpos.winRanks[depth][ss] = 0;
        else if (thrd.nodeTypeStore[win_hand] == MINNODE)
        {
          if ((tpos.rankInSuit[partner[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[lho[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[rho[win_hand]][ss] == 0))
            tpos.winRanks[depth][ss] = 0;
          else
            tpos.winRanks[depth][ss] = bitMapRank[tpos.winner[ss].rank];
        }
        else
          tpos.winRanks[depth][ss] = 0;
      }
      return false;
    }
  }
  else if (thrd.nodeTypeStore[tpos.winner[trump].hand] == MINNODE)
  {
    if ((tpos.length[hand][trump] == 0) &&
        (tpos.length[partner[hand]][trump] == 0))
    {
      if (((tpos.tricksMAX + (depth >> 2) + 1 -
            max(tpos.length[lho[hand]][trump],
                tpos.length[rho[hand]][trump])) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          tpos.winRanks[depth][ss] = 0;
        return false;
      }
    }
    else if ((tpos.tricksMAX + (depth >> 2)) < target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        tpos.winRanks[depth][ss] = 0;
      tpos.winRanks[depth][trump] =
        bitMapRank[tpos.winner[trump].rank];
      return false;
    }
    else if (tpos.tricksMAX + (depth >> 2) == target)
    {
      int hh = tpos.secondBest[trump].hand;
      if (hh == -1)
        return true;

      int r2 = tpos.secondBest[trump].rank;
      if ((thrd.nodeTypeStore[hh] == MINNODE) && (r2 != 0))
      {
        if (tpos.length[hh][trump] > 1 ||
            tpos.length[partner[hh]][trump] > 1)
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            tpos.winRanks[depth][ss] = 0;
          tpos.winRanks[depth][trump] = bitMapRank[r2];
          return false;
        }
      }
    }
  }
  else // Not NT
  {
    int hh = tpos.secondBest[trump].hand;
    if (hh == -1)
      return true;

    if ((thrd.nodeTypeStore[hh] != MINNODE) ||
        (tpos.length[hh][trump] <= 1))
      return true;

    if (tpos.winner[trump].hand == rho[hh])
    {
      if (((tpos.tricksMAX + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          tpos.winRanks[depth][ss] = 0;
        tpos.winRanks[depth][trump] =
          bitMapRank[tpos.secondBest[trump].rank];
        return false;
      }
    }
    else
    {
      unsigned short aggr = tpos.aggr[trump];
      int h = thrd.rel[aggr].absRank[3][trump].hand;
      if (h == -1)
        return true;

      if ((thrd.nodeTypeStore[h] == MINNODE) &&
          ((tpos.tricksMAX + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          tpos.winRanks[depth][ss] = 0;
        tpos.winRanks[depth][trump] = bitMapRank[
          static_cast<int>(thrd.rel[aggr].absRank[3][trump].rank) ];
        return false;
      }
    }
  }
  return true;
}


bool LaterTricksMAX(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  const ThreadData& thrd)
{
  if ((trump == DDS_NOTRUMP) || (tpos.winner[trump].rank == 0))
  {
    int sum = 0;
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      int hh = tpos.winner[ss].hand;
      if (hh != -1)
      {
        if (thrd.nodeTypeStore[hh] == MINNODE)
          sum += max(tpos.length[hh][ss],
                     tpos.length[partner[hh]][ss]);
      }
    }

    if ((tpos.tricksMAX + (depth >> 2) + 1 - sum >= target) &&
        (sum > 0))
    {
      if ((tpos.tricksMAX + 1 < target))
        return false;

      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        int win_hand = tpos.winner[ss].hand;
        if (win_hand == -1)
          tpos.winRanks[depth][ss] = 0;
        else if (thrd.nodeTypeStore[win_hand] == MAXNODE)
        {
          if ((tpos.rankInSuit[partner[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[lho[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[rho[win_hand]][ss] == 0))
            tpos.winRanks[depth][ss] = 0;
          else
            tpos.winRanks[depth][ss] =
              bitMapRank[tpos.winner[ss].rank];
        }
        else
          tpos.winRanks[depth][ss] = 0;
      }
      return true;
    }
  }
  else if (thrd.nodeTypeStore[tpos.winner[trump].hand] == MAXNODE)
  {
    if ((tpos.length[hand][trump] == 0) &&
        (tpos.length[partner[hand]][trump] == 0))
    {
      int maxlen = max(tpos.length[lho[hand]][trump],
                       tpos.length[rho[hand]][trump]);

      if ((tpos.tricksMAX + maxlen) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          tpos.winRanks[depth][ss] = 0;
        return true;
      }
    }
    else if ((tpos.tricksMAX + 1) >= target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        tpos.winRanks[depth][ss] = 0;
      tpos.winRanks[depth][trump] =
        bitMapRank[tpos.winner[trump].rank];
      return true;
    }
    else
    {
      int hh = tpos.secondBest[trump].hand;
      if (hh == -1)
        return false;

      if ((thrd.nodeTypeStore[hh] == MAXNODE) &&
          (tpos.secondBest[trump].rank != 0))
      {
        if (((tpos.length[hh][trump] > 1) ||
             (tpos.length[partner[hh]][trump] > 1)) &&
            ((tpos.tricksMAX + 2) >= target))
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            tpos.winRanks[depth][ss] = 0;
          tpos.winRanks[depth][trump] =
            bitMapRank[tpos.secondBest[trump].rank];
          return true;
        }
      }
    }
  }

  else // trump != DDS_NOTRUMP)
  {
    int hh = tpos.secondBest[trump].hand;
    if (hh == -1)
      return false;

    if ((thrd.nodeTypeStore[hh] != MAXNODE) ||
        (tpos.length[hh][trump] <= 1))
      return false;

    if (tpos.winner[trump].hand == rho[hh])
    {
      if ((tpos.tricksMAX + 1) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          tpos.winRanks[depth][ss] = 0;
        tpos.winRanks[depth][trump] =
          bitMapRank[tpos.secondBest[trump].rank] ;
        return true;
      }
    }
    else
    {
      unsigned short aggr = tpos.aggr[trump];
      int h = thrd.rel[aggr].absRank[3][trump].hand;
      if (h == -1)
        return false;

      if ((thrd.nodeTypeStore[h] == MAXNODE) &&
          ((tpos.tricksMAX + 1) >= target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          tpos.winRanks[depth][ss] = 0;
        tpos.winRanks[depth][trump] = bitMapRank[
          static_cast<int>(thrd.rel[aggr].absRank[3][trump].rank) ];
        return true;
      }
    }
  }
  return false;
}

