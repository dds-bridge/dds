/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "threadmem.h"
#include "LaterTricks.h"


bool LaterTricksMIN(
  pos                   * posPoint, 
  int                   hand, 
  int                   depth, 
  int                   target,
  int                   trump, 
  localVarType          * thrp)
{
  if ((trump == DDS_NOTRUMP) || (posPoint->winner[trump].rank == 0))
  {
    int sum = 0;
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      int hh = posPoint->winner[ss].hand;
      if (hh != -1)
      {
        if (thrp->nodeTypeStore[hh] == MAXNODE)
          sum += Max(posPoint->length[hh][ss], 
                     posPoint->length[partner[hh]][ss]);
      }
    }

    if ((posPoint->tricksMAX + sum < target) && (sum > 0))
    {
      if ((posPoint->tricksMAX + (depth >> 2) >= target))
        return true;

      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        int win_hand = posPoint->winner[ss].hand;

        if (win_hand == -1)
          posPoint->winRanks[depth][ss] = 0;
        else if (thrp->nodeTypeStore[win_hand] == MINNODE)
        {
          if ((posPoint->rankInSuit[partner[win_hand]][ss] == 0) &&
              (posPoint->rankInSuit[lho[win_hand]][ss] == 0) &&
              (posPoint->rankInSuit[rho[win_hand]][ss] == 0))
            posPoint->winRanks[depth][ss] = 0;
          else
            posPoint->winRanks[depth][ss] = 
              bitMapRank[posPoint->winner[ss].rank];
        }
        else
          posPoint->winRanks[depth][ss] = 0;
      }
      return false;
    }
  }
  else if (thrp->nodeTypeStore[posPoint->winner[trump].hand] == MINNODE)
  {
    if ((posPoint->length[hand][trump] == 0) &&
        (posPoint->length[partner[hand]][trump] == 0))
    {
      if (((posPoint->tricksMAX + (depth >> 2) + 1 -
            Max(posPoint->length[lho[hand]][trump],
                posPoint->length[rho[hand]][trump])) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          posPoint->winRanks[depth][ss] = 0;
        return false;
      }
    }
    else if ((posPoint->tricksMAX + (depth >> 2)) < target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 0;
      posPoint->winRanks[depth][trump] = 
        bitMapRank[posPoint->winner[trump].rank];
      return false;
    }
    else if (posPoint->tricksMAX + (depth >> 2) == target)
    {
      int hh = posPoint->secondBest[trump].hand;
      if (hh == -1)
        return true;

      int r2 = posPoint->secondBest[trump].rank;
      if ((thrp->nodeTypeStore[hh] == MINNODE) && (r2 != 0))
      {
        if (posPoint->length[hh][trump] > 1 || 
            posPoint->length[partner[hh]][trump] > 1)
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            posPoint->winRanks[depth][ss] = 0;
          posPoint->winRanks[depth][trump] = bitMapRank[r2];
          return false;
        }
      }
    }
  }
  else // Not NT
  {
    int hh = posPoint->secondBest[trump].hand;
    if (hh == -1)
      return true;

    if ((thrp->nodeTypeStore[hh] != MINNODE) ||
        (posPoint->length[hh][trump] <= 1))
        return true;

    if (posPoint->winner[trump].hand == rho[hh])
    {
      if (((posPoint->tricksMAX + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          posPoint->winRanks[depth][ss] = 0;
        posPoint->winRanks[depth][trump] =
          bitMapRank[posPoint->secondBest[trump].rank];
        return false;
      }
    }
    else
    {
      unsigned short aggr = posPoint->aggr[trump];
      int h = thrp->rel[aggr].absRank[3][trump].hand;
      if (h == -1)
        return true;

      if ((thrp->nodeTypeStore[h] == MINNODE) &&
          ((posPoint->tricksMAX + (depth >> 2)) < target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          posPoint->winRanks[depth][ss] = 0;
        posPoint->winRanks[depth][trump] = bitMapRank[ 
            static_cast<int>(thrp->rel[aggr].absRank[3][trump].rank) ];
        return false; 
      }
    }
  }
  return true;
}


bool LaterTricksMAX(
  pos                   * posPoint, 
  int                   hand, 
  int                   depth, 
  int                   target,
  int                   trump, 
  localVarType          * thrp)
{
  if ((trump == DDS_NOTRUMP) || (posPoint->winner[trump].rank == 0))
  {
    int sum = 0;
    for (int ss = 0; ss < DDS_SUITS; ss++)
    {
      int hh = posPoint->winner[ss].hand;
      if (hh != -1)
      {
        if (thrp->nodeTypeStore[hh] == MINNODE)
          sum += Max(posPoint->length[hh][ss], 
                     posPoint->length[partner[hh]][ss]);
      }
    }

    if ((posPoint->tricksMAX + (depth >> 2) + 1 - sum >= target) &&
        (sum > 0))
    {
      if ((posPoint->tricksMAX + 1 < target))
        return false;

      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        int win_hand = posPoint->winner[ss].hand;
        if (win_hand == -1)
          posPoint->winRanks[depth][ss] = 0;
        else if (thrp->nodeTypeStore[win_hand] == MAXNODE)
        {
          if ((posPoint->rankInSuit[partner[win_hand]][ss] == 0) &&
              (posPoint->rankInSuit[lho[win_hand]][ss] == 0) &&
              (posPoint->rankInSuit[rho[win_hand]][ss] == 0))
            posPoint->winRanks[depth][ss] = 0;
          else
            posPoint->winRanks[depth][ss] = 
              bitMapRank[posPoint->winner[ss].rank];
        }
        else
          posPoint->winRanks[depth][ss] = 0;
      }
      return true;
    }
  }
  else if (thrp->nodeTypeStore[posPoint->winner[trump].hand] == MAXNODE)
  {
    if ((posPoint->length[hand][trump] == 0) &&
        (posPoint->length[partner[hand]][trump] == 0))
    {
      int maxlen = Max(posPoint->length[lho[hand]][trump],
                       posPoint->length[rho[hand]][trump]);

      if ((posPoint->tricksMAX + maxlen) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          posPoint->winRanks[depth][ss] = 0;
        return true;
      }
    }
    else if ((posPoint->tricksMAX + 1) >= target)
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
        posPoint->winRanks[depth][ss] = 0;
      posPoint->winRanks[depth][trump] =
        bitMapRank[posPoint->winner[trump].rank];
      return true;
    }
    else
    {
      int hh = posPoint->secondBest[trump].hand;
      if (hh == -1)
        return false;

      if ((thrp->nodeTypeStore[hh] == MAXNODE) && 
         (posPoint->secondBest[trump].rank != 0))
      {
        if (((posPoint->length[hh][trump] > 1) ||
             (posPoint->length[partner[hh]][trump] > 1)) &&
            ((posPoint->tricksMAX + 2) >= target))
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            posPoint->winRanks[depth][ss] = 0;
          posPoint->winRanks[depth][trump] =
            bitMapRank[posPoint->secondBest[trump].rank];
          return true;
        }
      }
    }
  }

  else // trump != DDS_NOTRUMP)
  {
    int hh = posPoint->secondBest[trump].hand;
    if (hh == -1)
      return false;

    if ((thrp->nodeTypeStore[hh] != MAXNODE) ||
        (posPoint->length[hh][trump] <= 1))
      return false;

    if (posPoint->winner[trump].hand == rho[hh])
    {
      if ((posPoint->tricksMAX + 1) >= target)
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          posPoint->winRanks[depth][ss] = 0;
        posPoint->winRanks[depth][trump] =
          bitMapRank[posPoint->secondBest[trump].rank] ;
        return true;
      }
    }
    else
    {
      unsigned short aggr = posPoint->aggr[trump];
      int h = thrp->rel[aggr].absRank[3][trump].hand;
      if (h == -1)
        return false;

      if ((thrp->nodeTypeStore[h] == MAXNODE) &&
          ((posPoint->tricksMAX + 1) >= target))
      {
        for (int ss = 0; ss < DDS_SUITS; ss++)
          posPoint->winRanks[depth][ss] = 0;
        posPoint->winRanks[depth][trump] = bitMapRank[
          static_cast<int>(thrp->rel[aggr].absRank[3][trump].rank) ];
         return true;
      }
    }
  }
  return false;
}

