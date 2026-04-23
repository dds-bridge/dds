/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <algorithm>

#include "later_tricks.hpp"
#include <solver_context/solver_context.hpp>


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
 * @param ctx Solver context
 * @return true if target can be reached, false otherwise
 */
bool LaterTricksMIN(
  Pos& tpos,
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
            ctx.search().node_type_store(hh) == MAXNODE)
          sum += std::max(tpos.length[hh][ss],
                     tpos.length[partner[hh]][ss]);
      }
    }

    if ((tpos.tricks_max + sum < target) && (sum > 0))
    {
      if ((tpos.tricks_max + (depth >> 2) >= target))
        return true;

      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        int win_hand = tpos.winner[ss].hand;

        if (win_hand == -1) {
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        }
        else if (static_cast<unsigned>(win_hand) >= static_cast<unsigned>(DDS_HANDS)) {
          // Invalid hand index; avoid using partner/lho/rho with OOB index.
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
          continue;
        }
        else if (ctx.search().node_type_store(win_hand) == MINNODE)
        {
          if ((tpos.rank_in_suit[partner[win_hand]][ss] == 0) &&
              (tpos.rank_in_suit[lho[win_hand]][ss] == 0) &&
              (tpos.rank_in_suit[rho[win_hand]][ss] == 0))
            { if (depth_ok) tpos.win_ranks[depth][ss] = 0; }
          else
            { if (depth_ok) tpos.win_ranks[depth][ss] = bit_map_rank[tpos.winner[ss].rank]; }
        }
        else {
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        }
      }
      return false;
    }
  }
  else if (ctx.search().node_type_store(tpos.winner[trump].hand) == MINNODE)
  {
    if ((tpos.length[hand][trump] == 0) &&
        (tpos.length[partner[hand]][trump] == 0))
    {
      if (((tpos.tricks_max + (depth >> 2) + 1 -
            std::max(tpos.length[lho[hand]][trump],
                tpos.length[rho[hand]][trump])) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        return false;
      }
    }
    else if ((tpos.tricks_max + (depth >> 2)) < target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        if (depth_ok) tpos.win_ranks[depth][ss] = 0;
      if (depth_ok) tpos.win_ranks[depth][trump] =
        bit_map_rank[tpos.winner[trump].rank];
      return false;
    }
    else if (tpos.tricks_max + (depth >> 2) == target)
    {
      int hh = tpos.second_best[trump].hand;
      if (hh == -1)
        return true;

      if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
        return true;

      int r2 = tpos.second_best[trump].rank;
      if ((ctx.search().node_type_store(hh) == MINNODE) && (r2 != 0))
      {
        if (tpos.length[hh][trump] > 1 ||
            tpos.length[partner[hh]][trump] > 1)
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            if (depth_ok) tpos.win_ranks[depth][ss] = 0;
          if (depth_ok) tpos.win_ranks[depth][trump] = bit_map_rank[r2];
          return false;
        }
      }
    }
  }
  else // Not NT
  {
    int hh = tpos.second_best[trump].hand;
    if (hh == -1)
      return true;
    if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
      return true;

    if ((ctx.search().node_type_store(hh) != MINNODE) ||
        (tpos.length[hh][trump] <= 1))
      return true;

    if (tpos.winner[trump].hand == rho[hh])
    {
      if (((tpos.tricks_max + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        if (depth_ok) tpos.win_ranks[depth][trump] =
          bit_map_rank[tpos.second_best[trump].rank];
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
      int h = ctx.thread()->rel[aggr].abs_rank[3][trump].hand;
      if (h == -1)
        return true;

      if (static_cast<unsigned>(h) >= static_cast<unsigned>(DDS_HANDS))
        return true;

      if ((ctx.search().node_type_store(h) == MINNODE) &&
          ((tpos.tricks_max + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        if (depth_ok) tpos.win_ranks[depth][trump] = bit_map_rank[
          static_cast<int>(static_cast<unsigned char>(ctx.thread()->rel[aggr].abs_rank[3][trump].rank)) ];
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
 * @param ctx Solver context
 * @return true if target can be reached, false otherwise
 */
bool LaterTricksMAX(
  Pos& tpos,
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
            ctx.search().node_type_store(hh) == MINNODE)
          sum += std::max(tpos.length[hh][ss],
                     tpos.length[partner[hh]][ss]);
      }
    }

    if ((tpos.tricks_max + (depth >> 2) + 1 - sum >= target) &&
        (sum > 0))
    {
      if ((tpos.tricks_max + 1 < target))
        return false;

      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        int win_hand = tpos.winner[ss].hand;
        if (win_hand == -1) {
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        }
        else if (static_cast<unsigned>(win_hand) >= static_cast<unsigned>(DDS_HANDS)) {
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
          continue;
        }
        else if (ctx.search().node_type_store(win_hand) == MAXNODE)
        {
          if ((tpos.rank_in_suit[partner[win_hand]][ss] == 0) &&
              (tpos.rank_in_suit[lho[win_hand]][ss] == 0) &&
              (tpos.rank_in_suit[rho[win_hand]][ss] == 0))
            { if (depth_ok) tpos.win_ranks[depth][ss] = 0; }
          else
            { if (depth_ok) tpos.win_ranks[depth][ss] =
              bit_map_rank[tpos.winner[ss].rank]; }
        }
        else {
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        }
      }
      return true;
    }
  }
  else if (ctx.search().node_type_store(tpos.winner[trump].hand) == MAXNODE)
  {
    if ((tpos.length[hand][trump] == 0) &&
        (tpos.length[partner[hand]][trump] == 0))
    {
      int maxlen = std::max(tpos.length[lho[hand]][trump],
                       tpos.length[rho[hand]][trump]);

      if ((tpos.tricks_max + maxlen) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        return true;
      }
    }
    else if ((tpos.tricks_max + 1) >= target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        if (depth_ok) tpos.win_ranks[depth][ss] = 0;
      if (depth_ok) tpos.win_ranks[depth][trump] =
        bit_map_rank[tpos.winner[trump].rank];
      return true;
    }
    else
    {
      int hh = tpos.second_best[trump].hand;
      if (hh == -1)
        return false;

      if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
        return false;

      if ((ctx.search().node_type_store(hh) == MAXNODE) &&
          (tpos.second_best[trump].rank != 0))
      {
        if (((tpos.length[hh][trump] > 1) ||
             (tpos.length[partner[hh]][trump] > 1)) &&
            ((tpos.tricks_max + 2) >= target))
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            if (depth_ok) tpos.win_ranks[depth][ss] = 0;
          if (depth_ok) tpos.win_ranks[depth][trump] =
            bit_map_rank[tpos.second_best[trump].rank];
          return true;
        }
      }
    }
  }

  else // trump != DDS_NOTRUMP)
  {
    int hh = tpos.second_best[trump].hand;
    if (hh == -1)
      return false;
    if (static_cast<unsigned>(hh) >= static_cast<unsigned>(DDS_HANDS))
      return false;

    if ((ctx.search().node_type_store(hh) != MAXNODE) ||
        (tpos.length[hh][trump] <= 1))
      return false;

    if (tpos.winner[trump].hand == rho[hh])
    {
      if ((tpos.tricks_max + 1) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        if (depth_ok) tpos.win_ranks[depth][trump] =
          bit_map_rank[tpos.second_best[trump].rank] ;
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
      int h = ctx.thread()->rel[aggr].abs_rank[3][trump].hand;
      if (h == -1)
        return false;

      if (static_cast<unsigned>(h) >= static_cast<unsigned>(DDS_HANDS))
        return false;

      if ((ctx.search().node_type_store(h) == MAXNODE) &&
          ((tpos.tricks_max + 1) >= target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          if (depth_ok) tpos.win_ranks[depth][ss] = 0;
        if (depth_ok) tpos.win_ranks[depth][trump] = bit_map_rank[
          static_cast<int>(static_cast<unsigned char>(ctx.thread()->rel[aggr].abs_rank[3][trump].rank)) ];
        return true;
      }
    }
  }
  return false;
}

