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

int IsCard(char cardChar);


int ConvertFromPBN(
  char 		* dealBuff, 
  unsigned int 	remainCards[DDS_HANDS][DDS_SUITS]) 
{
  int bp=0, first, card, hand, handRelFirst, suitInHand, h, s;
  int IsCard(char cardChar);

  for (h=0; h<DDS_HANDS; h++)
    for (s=0; s<DDS_SUITS; s++)
      remainCards[h][s]=0;

  while (((dealBuff[bp]!='W')&&(dealBuff[bp]!='N')&&
	(dealBuff[bp]!='E')&&(dealBuff[bp]!='S')&&
        (dealBuff[bp]!='w')&&(dealBuff[bp]!='n')&&
	(dealBuff[bp]!='e')&&(dealBuff[bp]!='s'))&&(bp<3))
    bp++;

  if (bp>=3)
    return 0;

  if ((dealBuff[bp]=='N')||(dealBuff[bp]=='n'))
    first=0;
  else if ((dealBuff[bp]=='E')||(dealBuff[bp]=='e'))
    first=1;
  else if ((dealBuff[bp]=='S')||(dealBuff[bp]=='s'))
    first=2;
  else
    first=3;

  bp++;
  bp++;

  handRelFirst=0;  suitInHand=0;

  while ((bp<80)&&(dealBuff[bp]!='\0')) {
    card=IsCard(dealBuff[bp]);
    if (card) {
      switch (first) {
	case 0:
	  hand=handRelFirst;
	  break;
	case 1:
	  if (handRelFirst==0)
	    hand=1;
	  else if (handRelFirst==3)
	    hand=0;
	  else
	    hand=handRelFirst+1;
	    break;
	case 2:
	  if (handRelFirst==0)
	    hand=2;
	  else if (handRelFirst==1)
	    hand=3;
	  else
	    hand=handRelFirst-2;
	  break;
	default:
          if (handRelFirst==0)
	    hand=3;
	  else
	    hand=handRelFirst-1;
      }

      remainCards[hand][suitInHand]|=(bitMapRank[card]<<2);

    }
    else if (dealBuff[bp]=='.')
      suitInHand++;
    else if (dealBuff[bp]==' ') {
      handRelFirst++;
      suitInHand=0;
    }
    bp++;
  }
  return RETURN_NO_FAULT;
}


int IsCard(char cardChar)   {
  switch (cardChar)  {
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
      return 10;
    case 'J':
      return 11;
    case 'Q':
      return 12;
    case 'K':
      return 13;
    case 'A':
      return 14;
    case 't':
      return 10;
    case 'j':
      return 11;
    case 'q':
      return 12;
    case 'k':
      return 13;
    case 'a':
      return 14;
    default:
      return 0;
   }
 }


int ConvertPlayFromPBN(
  struct playTracePBN	*playPBN,
  struct playTraceBin	*playBin)
{
  int n = playPBN->number;

  if (n < 0 || n > 52)
    return RETURN_PLAY_FAULT;

  playBin->number = n;

  for (int i = 0; i < 2*n; i += 2)
  {
    char suit = playPBN->cards[i];
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
    playBin->suit[i >> 1] = s;

    int rank = IsCard(playPBN->cards[i+1]);
    if (rank == 0)
      return RETURN_PLAY_FAULT;

    playBin->rank[i >> 1] = rank;
  }
  return RETURN_NO_FAULT;
}

