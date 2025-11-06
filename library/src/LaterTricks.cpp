/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <algorithm>

#include "LaterTricks.hpp"
#include <solver_context/SolverContext.hpp>


/**
 * @brief Evaluate minimum possible tricks for a given position in double dummy analysis.
 *
 * This function estimates whether the minimum achievable tricks for the given position,
 * hand, depth, and target can satisfy the target, considering the trump suit and current state.
 *
 * @param tpos Position state
 * @param hand Current hand
 * @param depth Current search depth
 * @param target Target number of tricks
 * @param trump Trump suit
 * @param thrd Thread-local data
 * @return true if target can be reached, false otherwise
 */
bool LaterTricksMIN(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  SolverContext& ctx)
{
  
  const bool depth_ok = (depth >= 0 && depth < 50);
  if ((trump == DDS_NOTRUMP) || (tpos.winner[trump].rank == 0))
  {
    int sum = 0;
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      int hh = tpos.winner[ss].hand;
      if (hh != -1)
      {
        if (static_cast<unsigned>(hh) < static_cast<unsigned>(DDS_HANDS) &&
            ctx.search().nodeTypeStore(hh) == MAXNODE)
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

        if (win_hand == -1) {
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        }
        else if (static_cast<unsigned>(win_hand) >= static_cast<unsigned>(DDS_HANDS)) {
          // Invalid hand index; avoid using partner/lho/rho with OOB index.
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
          continue;
        }
        else if (ctx.search().nodeTypeStore(win_hand) == MINNODE)
        {
          if ((tpos.rankInSuit[partner[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[lho[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[rho[win_hand]][ss] == 0))
            { if (depth_ok) tpos.winRanks[depth][ss] = 0; }
          else
            { if (depth_ok) tpos.winRanks[depth][ss] = bitMapRank[tpos.winner[ss].rank]; }
        }
        else {
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        }
      }
      return false;
    }
  }
  else if (ctx.search().nodeTypeStore(tpos.winner[trump].hand) == MINNODE)
  {
    if ((tpos.length[hand][trump] == 0) &&
        (tpos.length[partner[hand]][trump] == 0))
    {
      if (((tpos.tricksMAX + (depth >> 2) + 1 -
            max(tpos.length[lho[hand]][trump],
                tpos.length[rho[hand]][trump])) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        return false;
      }
    }
    else if ((tpos.tricksMAX + (depth >> 2)) < target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        if (depth_ok) tpos.winRanks[depth][ss] = 0;
      if (depth_ok) tpos.winRanks[depth][trump] =
        bitMapRank[tpos.winner[trump].rank];
      return false;
    }
    else if (tpos.tricksMAX + (depth >> 2) == target)
    {
      int hh = tpos.secondBest[trump].hand;
      if (hh == -1)
        return true;

      if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
        return true;

      int r2 = tpos.secondBest[trump].rank;
      if ((ctx.search().nodeTypeStore(hh) == MINNODE) && (r2 != 0))
      {
        if (tpos.length[hh][trump] > 1 ||
            tpos.length[partner[hh]][trump] > 1)
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            if (depth_ok) tpos.winRanks[depth][ss] = 0;
          if (depth_ok) tpos.winRanks[depth][trump] = bitMapRank[r2];
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
    if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
      return true;

    if ((ctx.search().nodeTypeStore(hh) != MINNODE) ||
        (tpos.length[hh][trump] <= 1))
      return true;

    if (tpos.winner[trump].hand == rho[hh])
    {
      if (((tpos.tricksMAX + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        if (depth_ok) tpos.winRanks[depth][trump] =
          bitMapRank[tpos.secondBest[trump].rank];
        return false;
      }
    }
    else
    {
      unsigned short aggr = tpos.aggr[trump];
      // Defensive check: rel[] is sized 8192 in ThreadData. If aggr
      // is out of bounds we avoid a crash and return a conservative result.
      if (aggr >= 8192u)
      {
        fprintf(stderr, "LaterTricksMIN: invalid aggr=%u (depth=%d)", aggr, depth);
        return true; // conservative fallback
      }
  int h = ctx.thread()->rel[aggr].absRank[3][trump].hand;
      if (h == -1)
        return true;

      if (static_cast<unsigned>(h) >= static_cast<unsigned>(DDS_HANDS))
        return true;

      if ((ctx.search().nodeTypeStore(h) == MINNODE) &&
          ((tpos.tricksMAX + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        if (depth_ok) tpos.winRanks[depth][trump] = bitMapRank[
          static_cast<int>(static_cast<unsigned char>(ctx.thread()->rel[aggr].absRank[3][trump].rank)) ];
        return false;
      }
    }
  }
  return true;
}


/**
 * @brief Evaluate maximum possible tricks for a given position in double dummy analysis.
 *
 * This function estimates whether the maximum achievable tricks for the given position,
 * hand, depth, and target can satisfy the target, considering the trump suit and current state.
 *
 * @param tpos Position state
 * @param hand Current hand
 * @param depth Current search depth
 * @param target Target number of tricks
 * @param trump Trump suit
 * @param thrd Thread-local data
 * @return true if target can be reached, false otherwise
 */
bool LaterTricksMAX(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  SolverContext& ctx)
{
  
  const bool depth_ok = (depth >= 0 && depth < 50);
  if ((trump == DDS_NOTRUMP) || (tpos.winner[trump].rank == 0))
  {
    int sum = 0;
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      int hh = tpos.winner[ss].hand;
      if (hh != -1)
      {
        if (static_cast<unsigned>(hh) < static_cast<unsigned>(DDS_HANDS) &&
            ctx.search().nodeTypeStore(hh) == MINNODE)
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
        if (win_hand == -1) {
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        }
        else if (static_cast<unsigned>(win_hand) >= static_cast<unsigned>(DDS_HANDS)) {
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
          continue;
        }
        else if (ctx.search().nodeTypeStore(win_hand) == MAXNODE)
        {
          if ((tpos.rankInSuit[partner[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[lho[win_hand]][ss] == 0) &&
              (tpos.rankInSuit[rho[win_hand]][ss] == 0))
            { if (depth_ok) tpos.winRanks[depth][ss] = 0; }
          else
            { if (depth_ok) tpos.winRanks[depth][ss] =
              bitMapRank[tpos.winner[ss].rank]; }
        }
        else {
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        }
      }
      return true;
    }
  }
  else if (ctx.search().nodeTypeStore(tpos.winner[trump].hand) == MAXNODE)
  {
    if ((tpos.length[hand][trump] == 0) &&
        (tpos.length[partner[hand]][trump] == 0))
    {
      int maxlen = max(tpos.length[lho[hand]][trump],
                       tpos.length[rho[hand]][trump]);

      if ((tpos.tricksMAX + maxlen) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        return true;
      }
    }
    else if ((tpos.tricksMAX + 1) >= target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        if (depth_ok) tpos.winRanks[depth][ss] = 0;
      if (depth_ok) tpos.winRanks[depth][trump] =
        bitMapRank[tpos.winner[trump].rank];
      return true;
    }
    else
    {
      int hh = tpos.secondBest[trump].hand;
      if (hh == -1)
        return false;

      if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
        return false;

      if ((ctx.search().nodeTypeStore(hh) == MAXNODE) &&
          (tpos.secondBest[trump].rank != 0))
      {
        if (((tpos.length[hh][trump] > 1) ||
             (tpos.length[partner[hh]][trump] > 1)) &&
            ((tpos.tricksMAX + 2) >= target))
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            if (depth_ok) tpos.winRanks[depth][ss] = 0;
          if (depth_ok) tpos.winRanks[depth][trump] =
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
    if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
      return false;

    if ((ctx.search().nodeTypeStore(hh) != MAXNODE) ||
        (tpos.length[hh][trump] <= 1))
      return false;

    if (tpos.winner[trump].hand == rho[hh])
    {
      if ((tpos.tricksMAX + 1) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        if (depth_ok) tpos.winRanks[depth][trump] =
          bitMapRank[tpos.secondBest[trump].rank] ;
        return true;
      }
    }
    else
    {
      unsigned short aggr = tpos.aggr[trump];
      // Defensive check mirroring LaterTricksMIN: ThreadData::rel has 8192 entries.
      if (aggr >= 8192u)
      {
        fprintf(stderr, "LaterTricksMAX: invalid aggr=%u (depth=%d)\n", aggr, depth);
        return false; // conservative fallback for MAX
      }
  int h = ctx.thread()->rel[aggr].absRank[3][trump].hand;
      if (h == -1)
        return false;

      if (static_cast<unsigned>(h) >= static_cast<unsigned>(DDS_HANDS))
        return false;

      if ((ctx.search().nodeTypeStore(h) == MAXNODE) &&
          ((tpos.tricksMAX + 1) >= target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.winRanks[depth][ss] = 0;
        if (depth_ok) tpos.winRanks[depth][trump] = bitMapRank[
          static_cast<int>(static_cast<unsigned char>(ctx.thread()->rel[aggr].absRank[3][trump].rank)) ];
        return true;
      }
    }
  }
  return false;
}

