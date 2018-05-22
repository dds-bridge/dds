/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <algorithm>

#include "QuickTricks.h"


int QtricksLeadHandNT(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  int& lhoTrumpRanks,
  int& rhoTrumpRanks,
  const bool commPartner,
  const int commSuit,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  const int trump,
  int& res);

int QtricksLeadHandTrump(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  const int lhoTrumpRanks,
  const int rhoTrumpRanks,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  int& res);

int QuickTricksPartnerHandTrump(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  const int lhoTrumpRanks,
  const int rhoTrumpRanks,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  const int commSuit,
  const int commRank,
  int& res,
  const ThreadData& thrd);

int QuickTricksPartnerHandNT(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  const int commSuit,
  const int commRank,
  int& res,
  const ThreadData& thrd);


int QuickTricks(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  bool& result,
  const ThreadData& thrd)
{
  int suit, commRank = 0, commSuit = -1;
  int res;
  int lhoTrumpRanks = 0, rhoTrumpRanks = 0;
  int cutoff, lowestQtricks = 0;

  result = true;
  int qtricks = 0;

  if (thrd.nodeTypeStore[hand] == MAXNODE)
    cutoff = target - tpos.tricksMAX;
  else
    cutoff = tpos.tricksMAX - target + (depth >> 2) + 2;

  bool commPartner = false;
  const unsigned short (* ris)[DDS_SUITS] = tpos.rankInSuit;
  const unsigned char (* len)[DDS_SUITS] = tpos.length;
  highCardType const * winner = tpos.winner;

  for (int s = 0; s < DDS_SUITS; s++)
  {
    if ((trump != DDS_NOTRUMP) && (trump != s))
    {
      /* Trump game, and we lead a non-trump suit */
      if (winner[s].hand == partner[hand])
      {
        /* Partner has winning card */
        if (ris[hand][s] != 0 && /* Own hand has card */
            (((ris[lho[hand]][s] != 0) || /* LHO not void */
              (ris[lho[hand]][trump] == 0)) && /* LHO no trump */
             ((ris[rho[hand]][s] != 0) || /* RHO not void */
              (ris[rho[hand]][trump] == 0)))) /* RHO no trump */
        {
          commPartner = true;
          commSuit = s;
          commRank = winner[s].rank;
          break;
        }
      }
      else if ((tpos.secondBest[s].hand == partner[hand]) &&
               (winner[s].hand == hand) &&
               (len[hand][s] >= 2) &&
               (len[partner[hand]][s] >= 2))
      {
        /* Can cross to partner's card: Type Kx opposite Ax */
        if (((ris[lho[hand]][s] != 0) || /* LHO not void */
             (ris[lho[hand]][trump] == 0)) /* LHO no trump */
            && ((ris[rho[hand]][s] != 0) || /* RHO not void */
                (ris[rho[hand]][trump] == 0))) /* RHO no trump */
        {
          commPartner = true;
          commSuit = s;
          commRank = tpos.secondBest[s].rank;
          break;
        }
      }
    }
    else if (trump == DDS_NOTRUMP)
    {
      if (winner[s].hand == partner[hand])
      {
        /* Partner has winning card in NT */
        if (ris[hand][s] != 0) /* Own hand has card */
        {
          commPartner = true;
          commSuit = s;
          commRank = winner[s].rank;
          break;
        }
      }
      else if ((tpos.secondBest[s].hand == partner[hand]) &&
               (winner[s].hand == hand) &&
               (len[hand][s] >= 2) &&
               (len[partner[hand]][s] >= 2))
      {
        /* Can cross to partner's card: Type Kx opposite Ax */
        commPartner = true;
        commSuit = s;
        commRank = tpos.secondBest[s].rank;
        break;
      }
    }
  }

  if ((trump != DDS_NOTRUMP) && (!commPartner) &&
      (ris[hand][trump] != 0) &&
      (winner[trump].hand == partner[hand]))
  {
    /* Communication in trump suit */
    commPartner = true;
    commSuit = trump;
    commRank = winner[trump].rank;
  }

  if (trump != DDS_NOTRUMP)
  {
    suit = trump;
    lhoTrumpRanks = len[lho[hand]][trump];
    rhoTrumpRanks = len[rho[hand]][trump];
  }
  else
    suit = 0;

  do
  {
    int countOwn = len[hand][suit];
    int countLho = len[lho[hand]][suit];
    int countRho = len[rho[hand]][suit];
    int countPart = len[partner[hand]][suit];
    int opps = countLho | countRho;

    if (!opps && (countPart == 0))
    {
      if (countOwn == 0)
      {
        /* Continue with next suit. */
        if ((trump != DDS_NOTRUMP) && (trump != suit))
        {
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
        }
        else
        {
          if ((trump != DDS_NOTRUMP) && (trump == suit))
          {
            if (trump == 0)
              suit = 1;
            else
              suit = 0;
          }
          else
            suit++;
        }
        continue;
      }

      /* Long tricks when only leading hand have cards in the suit. */
      if ((trump != DDS_NOTRUMP) && (trump != suit))
      {
        if ((lhoTrumpRanks == 0) && (rhoTrumpRanks == 0))
        {
          qtricks += countOwn;
          if (qtricks >= cutoff)
            return qtricks;
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
        else
        {
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
      }
      else
      {
        qtricks += countOwn;
        if (qtricks >= cutoff)
          return qtricks;

        if ((trump != DDS_NOTRUMP) && (suit == trump))
        {
          if (trump == 0)
            suit = 1;
          else
            suit = 0;
        }
        else
        {
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
        }
        continue;
      }
    }
    else
    {
      if (!opps && (trump != DDS_NOTRUMP) && (suit == trump))
      {
        /* The partner but not the opponents have cards in
           the trump suit. */

        int sum = max(countOwn, countPart);
        for (int s = 0; s < DDS_SUITS; s++)
        {
          if ((sum > 0) &&
              (s != trump) &&
              (countOwn >= countPart) &&
              (len[hand][s] > 0) &&
              (len[partner[hand]][s] == 0))
          {
            sum++;
            break;
          }
        }
        /* If the additional trick by ruffing causes a cutoff.
           (qtricks not incremented.) */
        if (sum >= cutoff)
          return sum;
      }
      else if (!opps)
      {
        /* The partner but not the opponents have cards in the suit. */
        int sum = min(countOwn, countPart);
        if (trump == DDS_NOTRUMP)
        {
          if (sum >= cutoff)
            return sum;
        }
        else if ((suit != trump) &&
                 (lhoTrumpRanks == 0) &&
                 (rhoTrumpRanks == 0))
        {
          if (sum >= cutoff)
            return sum;
        }
      }

      if (commPartner)
      {
        if (!opps && (countOwn == 0))
        {
          if ((trump != DDS_NOTRUMP) && (trump != suit))
          {
            if ((lhoTrumpRanks == 0) && (rhoTrumpRanks == 0))
            {
              qtricks += countPart;
              tpos.winRanks[depth][commSuit] |=
                bitMapRank[commRank];

              if (qtricks >= cutoff)
                return qtricks;

              suit++;
              if ((trump != DDS_NOTRUMP) && (suit == trump))
                suit++;
              continue;
            }
            else
            {
              suit++;
              if ((trump != DDS_NOTRUMP) && (suit == trump))
                suit++;
              continue;
            }
          }
          else
          {
            qtricks += countPart;
            tpos.winRanks[depth][commSuit] |=
              bitMapRank[commRank];

            if (qtricks >= cutoff)
              return qtricks;

            if ((trump != DDS_NOTRUMP) && (suit == trump))
            {
              if (trump == 0)
                suit = 1;
              else
                suit = 0;
            }
            else
            {
              suit++;
              if ((trump != DDS_NOTRUMP) && (suit == trump))
                suit++;
            }
            continue;
          }
        }
        else
        {
          if (!opps && (trump != DDS_NOTRUMP) && (suit == trump))
          {
            int sum = max(countOwn, countPart);
            for (int s = 0; s < DDS_SUITS; s++)
            {
              if ((sum > 0) &&
                  (s != trump) &&
                  (countOwn <= countPart) &&
                  (len[partner[hand]][s] > 0) &&
                  (len[hand][s] == 0))
              {
                sum++;
                break;
              }
            }
            if (sum >= cutoff)
            {
              tpos.winRanks[depth][commSuit] |=
                bitMapRank[commRank];
              return sum;
            }
          }
          else if (!opps)
          {
            int sum = min(countOwn, countPart);
            if (trump == DDS_NOTRUMP)
            {
              if (sum >= cutoff)
                return sum;
            }
            else if ((suit != trump) &&
                     (lhoTrumpRanks == 0) &&
                     (rhoTrumpRanks == 0))
            {
              if (sum >= cutoff)
                return sum;
            }
          }
        }
      }
    }

    if (winner[suit].rank == 0)
    {
      if ((trump != DDS_NOTRUMP) && (suit == trump))
      {
        if (trump == 0)
          suit = 1;
        else
          suit = 0;
      }
      else
      {
        suit++;
        if ((trump != DDS_NOTRUMP) && (suit == trump))
          suit++;
      }
      continue;
    }

    if (winner[suit].hand == hand)
    {
      if ((trump != DDS_NOTRUMP) && (trump != suit))
      {
        qtricks = QtricksLeadHandTrump(hand, tpos, cutoff, depth,
          countLho, countRho, lhoTrumpRanks, rhoTrumpRanks,
          countOwn, countPart, suit, qtricks, res);

        if (res == 1)
          return qtricks;
        else if (res == 2)
        {
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
      }
      else
      {
        qtricks = QtricksLeadHandNT(hand, tpos, cutoff, depth,
          countLho, countRho, lhoTrumpRanks, rhoTrumpRanks,
          commPartner, commSuit, countOwn, countPart,
          suit, qtricks, trump, res);

        if (res == 1)
          return qtricks;
        else if (res == 2)
        {
          if ((trump != DDS_NOTRUMP) && (trump == suit))
          {
            if (trump == 0)
              suit = 1;
            else
              suit = 0;
          }
          else
            suit++;
          continue;
        }
      }
    }

    /* It was not possible to take a quick trick by own winning
       card in the suit */
    else
    {
      /* Partner winning card? */
      if (winner[suit].hand == partner[hand])
      {
        /* Winner found at partner*/
        if (commPartner)
        {
          /* There is communication with the partner */
          if ((trump != DDS_NOTRUMP) && (trump != suit))
          {
            qtricks = QuickTricksPartnerHandTrump(hand, tpos,
              cutoff, depth, countLho, countRho,
              lhoTrumpRanks, rhoTrumpRanks, countOwn,
              countPart, suit, qtricks, commSuit, commRank, res, thrd);

            if (res == 1)
              return qtricks;
            else if (res == 2)
            {
              suit++;
              if ((trump != DDS_NOTRUMP) && (suit == trump))
                suit++;
              continue;
            }
          }
          else
          {
            qtricks = QuickTricksPartnerHandNT(hand, tpos, cutoff,
              depth, countLho, countRho, countOwn, countPart,
              suit, qtricks, commSuit, commRank, res, thrd);

            if (res == 1)
              return qtricks;
            else if (res == 2)
            {
              if ((trump != DDS_NOTRUMP) && (trump == suit))
              {
                if (trump == 0)
                  suit = 1;
                else
                  suit = 0;
              }
              else
                suit++;
              continue;
            }
          }
        }
      }
    }
    if ((trump != DDS_NOTRUMP) && (suit != trump) &&
        (countOwn > 0) && (lowestQtricks == 0) &&
        ((qtricks == 0) ||
         ((winner[suit].hand != hand) &&
          (winner[suit].hand != partner[hand]) &&
          (winner[trump].hand != hand) &&
          (winner[trump].hand != partner[hand]))))
    {
      if ((countPart == 0) && (len[partner[hand]][trump] > 0))
      {
        if (((countRho > 0) || (len[rho[hand]][trump] == 0)) &&
            ((countLho > 0) || (len[lho[hand]][trump] == 0)))
        {
          lowestQtricks = 1;
          if (1 >= cutoff)
            return 1;
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
        else if ((countRho == 0) && (countLho == 0))
        {
          if ((ris[lho[hand]][trump] |
               ris[rho[hand]][trump]) <
              ris[partner[hand]][trump])
          {
            lowestQtricks = 1;

            int rr = highestRank[ris[partner[hand]][trump]];
            if (rr != 0)
            {
              tpos.winRanks[depth][trump] |= bitMapRank[rr];
              if (1 >= cutoff)
                return 1;
            }
          }
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
        else if (countLho == 0)
        {
          if (ris[lho[hand]][trump] <
              ris[partner[hand]][trump])
          {
            lowestQtricks = 1;
            for (int rr = 14; rr >= 2; rr--)
            {
              if ((ris[partner[hand]][trump] & bitMapRank[rr]) != 0)
              {
                tpos.winRanks[depth][trump] |= bitMapRank[rr];
                break;
              }
            }
            if (1 >= cutoff)
              return 1;
          }
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
        else if (countRho == 0)
        {
          if (ris[rho[hand]][trump] <
              ris[partner[hand]][trump])
          {
            lowestQtricks = 1;
            for (int rr = 14; rr >= 2; rr--)
            {
              if ((ris[partner[hand]][trump] & bitMapRank[rr]) != 0)
              {
                tpos.winRanks[depth][trump] |= bitMapRank[rr];
                break;
              }
            }
            if (1 >= cutoff)
              return 1;
          }
          suit++;
          if ((trump != DDS_NOTRUMP) && (suit == trump))
            suit++;
          continue;
        }
      }
    }

    if (qtricks >= cutoff)
      return qtricks;

    if ((trump != DDS_NOTRUMP) && (suit == trump))
    {
      if (trump == 0)
        suit = 1;
      else
        suit = 0;
    }
    else
    {
      suit++;
      if ((trump != DDS_NOTRUMP) && (suit == trump))
        suit++;
    }
  }
  while (suit <= 3);

  if (qtricks == 0)
  {
    if ((trump == DDS_NOTRUMP) || (winner[trump].hand == -1))
    {
      for (int ss = 0; ss < DDS_SUITS; ss++)
      {
        if (winner[ss].hand == -1)
          continue;
        if (len[hand][ss] > 0)
        {
          tpos.winRanks[depth][ss] = bitMapRank[winner[ss].rank];
        }
      }

      if (thrd.nodeTypeStore[hand] != MAXNODE)
        cutoff = target - tpos.tricksMAX;
      else
      {
        cutoff = tpos.tricksMAX - target + (depth >> 2) + 2;
      }

      if (1 >= cutoff)
        return 0;
    }
  }

  result = false;
  return qtricks;
}


int QtricksLeadHandTrump(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  const int lhoTrumpRanks,
  const int rhoTrumpRanks,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  int& res)
{
  /* res=0 Continue with same suit.
     res=1 Cutoff.
     res=2 Continue with next suit. */

  res = 1;
  int qt = qtricks;
  if (((countLho != 0) ||
       (lhoTrumpRanks == 0)) &&
      ((countRho != 0) || (rhoTrumpRanks == 0)))
  {
    tpos.winRanks[depth][suit] |=
      bitMapRank[tpos.winner[suit].rank];
    qt++;
    if (qt >= cutoff)
      return qt;

    if ((countLho <= 1) &&
        (countRho <= 1) &&
        (countPart <= 1) &&
        (lhoTrumpRanks == 0) &&
        (rhoTrumpRanks == 0))
    {
      qt += countOwn - 1;
      if (qt >= cutoff)
        return qt;
      res = 2;
      return qt;
    }
  }

  if (tpos.secondBest[suit].hand == hand)
  {
    if ((lhoTrumpRanks == 0) && (rhoTrumpRanks == 0))
    {
      tpos.winRanks[depth][suit] |=
        bitMapRank[tpos.secondBest[suit].rank];
      qt++;
      if (qt >= cutoff)
        return qt;
      if ((countLho <= 2) && (countRho <= 2) && (countPart <= 2))
      {
        qt += countOwn - 2;
        if (qt >= cutoff)
          return qt;
        res = 2;
        return qt;
      }
    }
  }
  else if ((tpos.secondBest[suit].hand == partner[hand])
           && (countOwn > 1) && (countPart > 1))
  {
    /* Second best at partner and suit length of own
       hand and partner > 1 */
    if ((lhoTrumpRanks == 0) && (rhoTrumpRanks == 0))
    {
      tpos.winRanks[depth][suit] |=
        bitMapRank[tpos.secondBest[suit].rank];
      qt++;
      if (qt >= cutoff)
        return qt;
      if ((countLho <= 2) &&
          (countRho <= 2) &&
          ((countPart <= 2) || (countOwn <= 2)))
      {
        qt += max(countOwn - 2, countPart - 2);
        if (qt >= cutoff)
          return qt;
        res = 2;
        return qt;
      }
    }
  }
  res = 0;
  return qt;
}

int QtricksLeadHandNT(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  int& lhoTrumpRanks,
  int& rhoTrumpRanks,
  const bool commPartner,
  const int commSuit,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  const int trump,
  int& res)
{
  /* res=0 Continue with same suit.
     res=1 Cutoff.
     res=2 Continue with next suit. */

  res = 1;
  int qt = qtricks;
  tpos.winRanks[depth][suit] |=
    bitMapRank[tpos.winner[suit].rank];

  qt++;
  if (qt >= cutoff)
    return qt;
  if ((trump == suit) && ((!commPartner) || (suit != commSuit)))
  {
    lhoTrumpRanks = max(0, lhoTrumpRanks - 1);
    rhoTrumpRanks = max(0, rhoTrumpRanks - 1);
  }

  if ((countLho <= 1) && (countRho <= 1) && (countPart <= 1))
  {
    qt += countOwn - 1;
    if (qt >= cutoff)
      return qt;
    res = 2;
    return qt;
  }

  if (tpos.secondBest[suit].hand == hand)
  {
    tpos.winRanks[depth][suit] |=
      bitMapRank[tpos.secondBest[suit].rank];
    qt++;
    if (qt >= cutoff)
      return qt;
    if ((trump == suit) && ((!commPartner) || (suit != commSuit)))
    {
      lhoTrumpRanks = max(0, lhoTrumpRanks - 1);
      rhoTrumpRanks = max(0, rhoTrumpRanks - 1);
    }
    if ((countLho <= 2) && (countRho <= 2) && (countPart <= 2))
    {
      qt += countOwn - 2;
      if (qt >= cutoff)
        return qt;
      res = 2;
      return qt;
    }
  }
  else if ((tpos.secondBest[suit].hand == partner[hand])
           && (countOwn > 1) && (countPart > 1))
  {
    /* Second best at partner and suit length of own
       hand and partner > 1 */
    tpos.winRanks[depth][suit] |=
      bitMapRank[tpos.secondBest[suit].rank];
    qt++;
    if (qt >= cutoff)
      return qt;
    if ((trump == suit) && ((!commPartner) || (suit != commSuit)))
    {
      lhoTrumpRanks = max(0, lhoTrumpRanks - 1);
      rhoTrumpRanks = max(0, rhoTrumpRanks - 1);
    }
    if ((countLho <= 2) &&
        (countRho <= 2) &&
        ((countPart <= 2) || (countOwn <= 2)))
    {
      qt += max(countOwn - 2, countPart - 2);
      if (qt >= cutoff)
        return qt;
      res = 2;
      return qt;
    }
  }

  res = 0;
  return qt;
}


int QuickTricksPartnerHandTrump(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  const int lhoTrumpRanks,
  const int rhoTrumpRanks,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  const int commSuit,
  const int commRank,
  int& res,
  const ThreadData& thrd)
{
  /* res=0 Continue with same suit.
     res=1 Cutoff.
     res=2 Continue with next suit. */

  res = 1;
  int qt = qtricks;
  if (((countLho != 0) || (lhoTrumpRanks == 0)) &&
      ((countRho != 0) || (rhoTrumpRanks == 0)))
  {
    tpos.winRanks[depth][suit] |=
      bitMapRank[tpos.winner[suit].rank];

    tpos.winRanks[depth][commSuit] |= bitMapRank[commRank];

    qt++; /* A trick can be taken */
    if (qt >= cutoff)
      return qt;
    if ((countLho <= 1) &&
        (countRho <= 1) &&
        (countOwn <= 1) &&
        (lhoTrumpRanks == 0) &&
        (rhoTrumpRanks == 0))
    {
      qt += countPart - 1;
      if (qt >= cutoff)
        return qt;
      res = 2;
      return qt;
    }
  }

  if (tpos.secondBest[suit].hand == partner[hand])
  {
    /* Second best found in partners hand */
    if ((lhoTrumpRanks == 0) && (rhoTrumpRanks == 0))
    {
      /* Opponents have no trump */
      tpos.winRanks[depth][suit] |=
        bitMapRank[tpos.secondBest[suit].rank];

      tpos.winRanks[depth][commSuit] |= bitMapRank[commRank];
      qt++;
      if (qt >= cutoff)
        return qt;
      if ((countLho <= 2) && (countRho <= 2) && (countOwn <= 2))
      {
        qt += countPart - 2;
        if (qt >= cutoff)
          return qt;
        res = 2;
        return qt;
      }
    }
  }
  else if ((tpos.secondBest[suit].hand == hand) &&
           (countPart > 1) &&
           (countOwn > 1))
  {
    /* Second best found in own hand and suit lengths of own hand
       and partner > 1*/

    if ((lhoTrumpRanks == 0) && (rhoTrumpRanks == 0))
    {
      /* Opponents have no trump */
      tpos.winRanks[depth][suit] |=
        bitMapRank[tpos.secondBest[suit].rank];

      tpos.winRanks[depth][commSuit] |= bitMapRank[commRank];

      qt++;
      if (qt >= cutoff)
        return qt;
      if ((countLho <= 2) &&
          (countRho <= 2) &&
          ((countOwn <= 2) || (countPart <= 2)))
      {
        qt += max(countPart - 2, countOwn - 2);
        if (qt >= cutoff)
          return qt;
        res = 2;
        return qt;
      }
    }
  }
  else if ((suit == commSuit) &&
           (tpos.secondBest[suit].hand == lho[hand]) &&
           ((countLho >= 2) || (lhoTrumpRanks == 0)) &&
           ((countRho >= 2) || (rhoTrumpRanks == 0)))
  {
    unsigned short ranks = 0;
    for (int h = 0; h < DDS_HANDS; h++)
      ranks |= tpos.rankInSuit[h][suit];

    if (thrd.rel[ranks].absRank[3][suit].hand == partner[hand])
    {
      tpos.winRanks[depth][suit] |= bitMapRank[
        static_cast<int>(thrd.rel[ranks].absRank[3][suit].rank) ];

      tpos.winRanks[depth][commSuit] |= bitMapRank[commRank];

      qt++;
      if (qt >= cutoff)
        return qt;
      if ((countOwn <= 2) &&
          (countLho <= 2) &&
          (countRho <= 2) &&
          (lhoTrumpRanks == 0) &&
          (rhoTrumpRanks == 0))
      {
        qt += countPart - 2;
        if (qt >= cutoff)
          return qt;
      }
    }
  }
  res = 0;
  return qt;
}


int QuickTricksPartnerHandNT(
  const int hand,
  pos& tpos,
  const int cutoff,
  const int depth,
  const int countLho,
  const int countRho,
  const int countOwn,
  const int countPart,
  const int suit,
  const int qtricks,
  const int commSuit,
  const int commRank,
  int& res,
  const ThreadData& thrd)
{
  res = 1;
  int qt = qtricks;

  tpos.winRanks[depth][suit] |=
    bitMapRank[tpos.winner[suit].rank];

  tpos.winRanks[depth][commSuit] |= bitMapRank[commRank];

  qt++;
  if (qt >= cutoff)
    return qt;
  if ((countLho <= 1) && (countRho <= 1) && (countOwn <= 1))
  {
    qt += countPart - 1;
    if (qt >= cutoff)
      return qt;
    res = 2;
    return qt;
  }

  if (tpos.secondBest[suit].hand == partner[hand])
  {
    /* Second best found in partners hand */
    tpos.winRanks[depth][suit] |=
      bitMapRank[tpos.secondBest[suit].rank];

    qt++;
    if (qt >= cutoff)
      return qt;
    if ((countLho <= 2) && (countRho <= 2) && (countOwn <= 2))
    {
      qt += countPart - 2;
      if (qt >= cutoff)
        return qt;
      res = 2;
      return qt;
    }
  }
  else if ((tpos.secondBest[suit].hand == hand)
           && (countPart > 1) && (countOwn > 1))
  {
    /* Second best found in own hand and own and
       partner's suit length > 1 */
    tpos.winRanks[depth][suit] |=
      bitMapRank[tpos.secondBest[suit].rank];

    qt++;
    if (qt >= cutoff)
      return qt;
    if ((countLho <= 2) &&
        (countRho <= 2) &&
        ((countOwn <= 2) || (countPart <= 2)))
    {
      qt += max(countPart - 2, countOwn - 2);
      if (qt >= cutoff)
        return qt;
      res = 2;
      return qt;
    }
  }
  else if ((suit == commSuit) &&
           (tpos.secondBest[suit].hand == lho[hand]))
  {
    unsigned short ranks = 0;
    for (int h = 0; h < DDS_HANDS; h++)
      ranks |= tpos.rankInSuit[h][suit];

    if (thrd.rel[ranks].absRank[3][suit].hand == partner[hand])
    {
      tpos.winRanks[depth][suit] |= bitMapRank[
        static_cast<int>(thrd.rel[ranks].absRank[3][suit].rank) ];
      qt++;
      if (qt >= cutoff)
        return qt;
      if ((countOwn <= 2) && (countLho <= 2) && (countRho <= 2))
      {
        // TODO: Is the fix to qt correct?
        // qtricks += countPart - 2;
        qt += countPart - 2;
        if (qt >= cutoff)
          return qt;
      }
    }
  }
  res = 0;
  return qt;
}


bool QuickTricksSecondHand(
  pos& tpos,
  const int hand,
  const int depth,
  const int target,
  const int trump,
  const ThreadData& thrd)
{
  if (depth == thrd.iniDepth)
    return false;

  int ss = tpos.move[depth + 1].suit;
  unsigned short (*ris)[DDS_SUITS] = tpos.rankInSuit;
  unsigned short ranks = static_cast<unsigned short>
                         (ris[hand][ss] | ris[partner[hand]][ss]);

  for (int s = 0; s < DDS_SUITS; s++)
    tpos.winRanks[depth][s] = 0;

  if ((trump != DDS_NOTRUMP) && (ss != trump) &&
      (((ris[hand][ss] == 0) && (ris[hand][trump] != 0)) ||
       ((ris[partner[hand]][ss] == 0) &&
        (ris[partner[hand]][trump] != 0))))
  {
    if ((ris[lho[hand]][ss] == 0) &&
        (ris[lho[hand]][trump] != 0))
      return false;

    /* Own side can ruff, their side can't. */
  }

  else if (ranks > (bitMapRank[tpos.move[depth + 1].rank] |
                    ris[lho[hand]][ss]))
  {
    if ((trump != DDS_NOTRUMP) && (ss != trump) &&
        (ris[lho[hand]][trump] != 0) &&
        (ris[lho[hand]][ss] == 0))
      return false;

    /* Own side has highest card in suit, which LHO can't ruff. */

    int rr = highestRank[ranks];
    tpos.winRanks[depth][ss] = bitMapRank[rr];
  }
  else
  {
    /* No easy way to win current trick for own side. */
    return false;
  }

  int qtricks = 1;

  int cutoff;
  if (thrd.nodeTypeStore[hand] == MAXNODE)
    cutoff = target - tpos.tricksMAX;
  else
    cutoff = tpos.tricksMAX - target + (depth >> 2) + 3;

  if (qtricks >= cutoff)
    return true;

  if (trump != DDS_NOTRUMP)
    return false;

  /* In NT, second winner (by rank) in same suit. */

  int hh;
  if (ris[hand][ss] > ris[partner[hand]][ss])
    hh = hand; /* Hand to lead next trick */
  else
    hh = partner[hand];

  if ((tpos.winner[ss].hand == hh) &&
      (tpos.secondBest[ss].rank != 0) &&
      (tpos.secondBest[ss].hand == hh))
  {
    qtricks++;
    tpos.winRanks[depth][ss] |=
      bitMapRank[tpos.secondBest[ss].rank];

    if (qtricks >= cutoff)
      return true;
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    if ((s == ss) || (tpos.length[hh][s] == 0))
      continue;

    if ((tpos.length[lho[hh]][s] == 0) &&
        (tpos.length[rho[hh]][s] == 0) &&
        (tpos.length[partner[hh]][s] == 0))
    {
      /* Long other suit which nobody else holds. */
      qtricks += counttable[ris[hh][s]];
      if (qtricks >= cutoff)
        return true;
    }
    else if ((tpos.winner[s].rank != 0) &&
             (tpos.winner[s].hand == hh))
    {
      /* Top winners in other suits. */
      qtricks++;
      tpos.winRanks[depth][s] |=
        bitMapRank[tpos.winner[s].rank];

      if (qtricks >= cutoff)
        return true;
    }
  }

  return false;
}

