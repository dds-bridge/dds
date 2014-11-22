/*
   DDS 2.7.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
   permission to include a C++ adaptation in DDS.

   The PlayAnalyser analyses the played cards of the deal and presents
   their double dummy values. The par calculation function DealerPar
   provides an alternative way of calculating and presenting par
   results.  Both these functions have been written by Soren Hein.
   He has also made numerous contributions to the code, especially in
   the initialization part.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
   implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "dds.h"


bool LaterTricksMIN(
  struct pos		* posPoint, 
  int 			hand, 
  int 			depth, 
  int 			target,
  int 			trump, 
  struct localVarType	* thrp)
{
  int		win_trump_rank, win_trump_hand;

  win_trump_rank = posPoint->winner[trump].rank;
  win_trump_hand = posPoint->winner[trump].hand;

  if ((trump == DDS_NOTRUMP) || (win_trump_rank == 0))
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
  else if ((trump != DDS_NOTRUMP) && (win_trump_rank != 0) &&
           (thrp->nodeTypeStore[win_trump_hand] == MINNODE))
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
      posPoint->winRanks[depth][trump] = bitMapRank[win_trump_rank];
      return false;
    }
    else
    {
      int hh = posPoint->secondBest[trump].hand;
      if (hh == -1)
        return true;

      int r2 = posPoint->secondBest[trump].rank;
      if ((thrp->nodeTypeStore[hh] == MINNODE) && (r2 != 0))
      {
        if (((posPoint->length[hh][trump] > 1) ||
             (posPoint->length[partner[hh]][trump] > 1)) &&
            ((posPoint->tricksMAX + (depth >> 2) - 1) < target))
        {
          for (int ss = 0; ss < DDS_SUITS; ss++)
            posPoint->winRanks[depth][ss] = 0;
          posPoint->winRanks[depth][trump] = bitMapRank[r2];
          return false;
        }
      }
    }
  }
  else if (trump != DDS_NOTRUMP)
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
        posPoint->winRanks[depth][trump] =
          bitMapRank[thrp->rel[aggr].absRank[3][trump].rank];
        return false; 
      }
    }
  }
  return true;
}


bool LaterTricksMAX(
  struct pos 		* posPoint, 
  int 			hand, 
  int 			depth, 
  int 			target,
  int 			trump, 
  struct localVarType	* thrp)
{
  int		win_trump_rank, win_trump_hand;

  win_trump_rank = posPoint->winner[trump].rank;
  win_trump_hand = posPoint->winner[trump].hand;

  if ((trump == DDS_NOTRUMP) || (win_trump_rank == 0))
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
  else if ((trump != DDS_NOTRUMP) && (win_trump_rank != 0) &&
           (thrp->nodeTypeStore[win_trump_hand] == MAXNODE))
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

  else if (trump != DDS_NOTRUMP)
  {
    int hh = posPoint->secondBest[trump].hand;
    if (hh == -1)
      return false;

    if ((thrp->nodeTypeStore[hh] != MAXNODE) ||
        (posPoint->length[hh][trump] <= 1))
      return false;

    if (win_trump_hand == rho[hh])
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
        posPoint->winRanks[depth][trump] =
          bitMapRank[thrp->rel[aggr].absRank[3][trump].rank];
         return true;
      }
    }
  }
  return false;
}

