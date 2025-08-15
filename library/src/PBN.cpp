/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "PBN.h"

int IsCard(const char cardChar);


int ConvertFromPBN(
  char const * dealBuff,
  unsigned int remainCards[DDS_HANDS][DDS_SUITS])
{
  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      remainCards[h][s] = 0;

  int bp = 0;
  while (((dealBuff[bp] != 'W') && (dealBuff[bp] != 'N') &&
          (dealBuff[bp] != 'E') && (dealBuff[bp] != 'S') &&
          (dealBuff[bp] != 'w') && (dealBuff[bp] != 'n') &&
          (dealBuff[bp] != 'e') && (dealBuff[bp] != 's')) && (bp < 3))
    bp++;

  if (bp >= 3)
    return 0;

  int first;
  if ((dealBuff[bp] == 'N') || (dealBuff[bp] == 'n'))
    first = 0;
  else if ((dealBuff[bp] == 'E') || (dealBuff[bp] == 'e'))
    first = 1;
  else if ((dealBuff[bp] == 'S') || (dealBuff[bp] == 's'))
    first = 2;
  else
    first = 3;

  bp++;
  bp++;

  int handRelFirst = 0;
  int suitInHand = 0;
  int card, hand;

  while ((bp < 80) && (dealBuff[bp] != '\0'))
  {
    card = IsCard(dealBuff[bp]);
    if (card)
    {
      switch (first)
      {
        case 0:
          hand = handRelFirst;
          break;
        case 1:
          if (handRelFirst == 0)
            hand = 1;
          else if (handRelFirst == 3)
            hand = 0;
          else
            hand = handRelFirst + 1;
          break;
        case 2:
          if (handRelFirst == 0)
            hand = 2;
          else if (handRelFirst == 1)
            hand = 3;
          else
            hand = handRelFirst - 2;
          break;
        default:
          if (handRelFirst == 0)
            hand = 3;
          else
            hand = handRelFirst - 1;
      }

      remainCards[hand][suitInHand] |=
        static_cast<unsigned>((bitMapRank[card] << 2));

    }
    else if (dealBuff[bp] == '.')
      suitInHand++;
    else if (dealBuff[bp] == ' ')
    {
      handRelFirst++;
      suitInHand = 0;
    }
    bp++;
  }
  return RETURN_NO_FAULT;
}


int IsCard(const char cardChar)
{
  switch (cardChar)
  {
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case 'T':
    case 't':
      return 10;
    case 'J':
    case 'j':
      return 11;
    case 'Q':
    case 'q':
      return 12;
    case 'K':
    case 'k':
      return 13;
    case 'A':
    case 'a':
      return 14;
    default:
      return 0;
  }
}


int ConvertPlayFromPBN(
  const playTracePBN& playPBN,
  playTraceBin& playBin)
{
  const int n = playPBN.number;

  if (n < 0 || n > 52)
    return RETURN_PLAY_FAULT;

  playBin.number = n;

  for (int i = 0; i < 2 * n; i += 2)
  {
    char suit = playPBN.cards[i];
    int s;

    if (suit == 's' || suit == 'S')
      s = 0;
    else if (suit == 'h' || suit == 'H')
      s = 1;
    else if (suit == 'd' || suit == 'D')
      s = 2;
    else if (suit == 'c' || suit == 'C')
      s = 3;
    else
      return RETURN_PLAY_FAULT;
    playBin.suit[i >> 1] = s;

    int rank = IsCard(playPBN.cards[i+1]);
    if (rank == 0)
      return RETURN_PLAY_FAULT;

    playBin.rank[i >> 1] = rank;
  }
  return RETURN_NO_FAULT;
}

