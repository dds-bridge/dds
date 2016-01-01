/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// General initialization of three hands to be used in examples.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/dll.h"
#include "hands.h"

#define SP 0
#define HE 1
#define DI 2
#define CL 3

#define SPADES   0
#define HEARTS   1
#define DIAMONDS 2
#define CLUBS    3
#define NOTRUMP  4

#define NORTH    0
#define EAST     1
#define SOUTH    2
#define WEST     3

#define VUL_NONE 0
#define VUL_BOTH 1
#define VUL_NS   2
#define VUL_EW   3

#define R2  0x0004
#define R3  0x0008
#define R4  0x0010
#define R5  0x0020
#define R6  0x0040
#define R7  0x0080
#define R8  0x0100
#define R9  0x0200
#define RT  0x0400
#define RJ  0x0800
#define RQ  0x1000
#define RK  0x2000
#define RA  0x4000

#define K2       2
#define K3       3
#define K4       4
#define K5       5
#define K6       6
#define K7       7
#define K8       8
#define K9       9
#define KT      10
#define KJ      11
#define KQ      12
#define KK      13
#define KA      14


//////////////////////////////////////////////////////////
//                     Inputs                           //
//////////////////////////////////////////////////////////

int trump [3] = { SPADES  , NOTRUMP, SPADES   };
int first [3] = { NORTH   , EAST   , SOUTH    };
int dealer[3] = { NORTH   , EAST   , NORTH    };
int vul   [3] = { VUL_NONE, VUL_NS , VUL_NONE };

char PBN[3][80] = {
"N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
"E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63",
"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
};

// The same hands in binary.  The second index is suit, the
// third index is hand.
unsigned int holdings[3][4][4] =
{
  {
    { RQ|RJ|R6, R8|R7|R3, RK|R5, RA|RT|R9|R4|R2 } , // spades
    { RK|R6|R5|R2, RJ|R9|R7, RT|R8|R3, RA|RQ|R4 } , // hearts
    { RJ|R8|R5, RA|RT|R7|R6|R4, RK|RQ|R9, R3|R2 } , // diamonds
    { RT|R9|R8, RQ|R4, RA|R7|R6|R5|R2, RK|RJ|R3 }}, // clubs,
  {
    { RA|RK|R9|R6, RQ|RJ|RT|R5|R4|R3|R2, 0, R8|R7},
    { RK|RQ|R8, RT, RJ|R9|R7|R5|R4|R3, RA|R6|R2 },
    { RA|R9|R8, R6, RK|R7|R5|R3|R2, RQ|RJ|RT|R4 },
    { RK|R6|R3, RQ|RJ|R8|R2, R9|R4, RA|RT|R7|R5 }},
  {
    { R7|R3, RQ|RT|R6, R5, RA|RK|RJ|R9|R8|R4|R2 },
    { RQ|RJ|RT, R8|R7|R6, RA|R9|R5|R4|R3|R2, RK },
    { RA|RQ|R5|R4, RK|RJ|R9, R7|R6|R3|R2, RT|R8 },
    { RT|R7|R5|R2, RA|RQ|R8|R4, RK|R6, RJ|R9|R3 }}
};


// Number of cards played during the played before claim
int playNo[3] = { 45, 52, 12 };

// Actual cards played before claim
char play[3][106] = {
"CTC4CACJH8H4HKH9D5DAD9D2S7S5S2SQD8D4DQD3H3HAH6H7C3C8CQC2S3SKSAS6HQH5HJHTCKC9D6C5S4SJS8C6DJ",
"SQD2S8SAHKHTH3H2HQS2H4H6H8D6HJHAS7SKS4C4D8C2DKD4H9C5S6S3H7C7C3S5H5CTD9STD3DQDAC8S9SJC9DTCQD5CAC6DJCKCJD7",
"HAHKHQH7D7D8DAD9C5CAC6C3" 
};

int playSuit[3][52] = {
  { CL, CL, CL, CL,    HE, HE, HE, HE,    DI, DI, DI, DI,
    SP, SP, SP, SP,    DI, DI, DI, DI,    HE, HE, HE, HE,
    CL, CL, CL, CL,    SP, SP, SP, SP,    HE, HE, HE, HE,
    CL, CL, DI, CL,    SP, SP, SP, CL,    DI, -1, -1, -1,
    -1, -1, -1, -1 },
  { SP, DI, SP, SP,    HE, HE, HE, HE,    HE, SP, HE, HE,
    HE, DI, HE, HE,    SP, SP, SP, CL,    DI, CL, DI, DI,
    HE, CL, SP, SP,    HE, CL, CL, SP,    HE, CL, DI, SP,
    DI, DI, DI, CL,    SP, SP, CL, DI,    CL, DI, CL, CL,
    DI, CL, CL, DI },
  { HE, HE, HE, HE,    DI, DI, DI, DI,    CL, CL, CL, CL,
    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,
    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,
    -1, -1, -1, -1 }
};

int playRank[3][52] = {
  { KT, K4, KA, KJ,    K8, K4, KK, K9,    K5, KA, K9, K2,
    K7, K5, K2, KQ,    K8, K4, KQ, K3,    K3, KA, K6, K7,
    K3, K8, KQ, K2,    K3, KK, KA, K6,    KQ, K5, KJ, KT,
    KK, K9, K6, K5,    K4, KJ, K8, K6,    KJ, -1, -1, -1,
    -1, -1, -1, -1 },
  { KQ, K2, K8, KA,    KK, KT, K3, K2,    KQ, K2, K4, K6,
    K8, K6, KJ, KA,    K7, KK, K4, K4,    K8, K2, KK, K4,
    K9, K5, K6, K3,    K7, K7, K3, K5,    K5, KT, K9, KT,
    K3, KQ, KA, K8,    K9, KJ, K9, KT,    KQ, K5, KA, K6,
    KJ, KK, KJ, K7 },
  { KA, KK, KQ, K7,    K7, K8, KA, K9,    K5, KA, K6, K3,
    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,
    -1, -1, -1, -1,    -1, -1, -1, -1,    -1, -1, -1, -1,
    -1, -1, -1, -1 }
};


//////////////////////////////////////////////////////////
//                 Expected outputs                     //
//////////////////////////////////////////////////////////

// Number of cards returned for solutions == 2, i.e. for
// all cards leading to the optimal score (taking into
// account equivalences.
int cardsSoln2[3] = { 6, 3, 4 };

// Number of cards returned for solutions == 3, i.e. for
// all legally playable cards (taking into account equivalences).
int cardsSoln3[3] = { 9, 7, 8 };

// Suits of cards returned. Padded with zeroes.
int cardsSuits[3][13] = {
  { 2, 2, 2, 3, 0, 0, 1, 1, 1,    0, 0, 0, 0 },
  { 3, 3, 3, 1, 2, 0, 0,    0, 0, 0, 0, 0, 0 },
  { 1, 2, 2, 0, 1, 1, 3, 3,    0, 0, 0, 0, 0 }
};

// Ranks for cards returned (2 .. 14).  Padded with zeroes.
int cardsRanks[3][13] = {
  { 5, 8,11,10, 6,12, 2, 6,13,    0, 0, 0, 0 },
  { 2, 8,12,10, 6,12, 5,    0, 0, 0, 0, 0, 0 },
  {14, 3, 7, 5, 5, 9, 6,13,    0, 0, 0, 0, 0 }
};

// Scores for cards returned.
int cardsScores[3][13] = {
  { 5, 5, 5, 5, 5, 5, 4, 4, 4,    0, 0, 0, 0 },
  { 4, 4, 4, 3, 3, 3, 2,    0, 0, 0, 0, 0, 0 },
  { 3, 3, 3, 3, 2, 2, 1, 1,    0, 0, 0, 0, 0 }
};

// Equals for cards returned, i.e. equivalent cards (rank vectors).
int cardsEquals[3][13] = {
  { 0,   0,   0, 768,   0,2048,   0,  32,   0,    0,0,0,0},
  { 0,   0,2048,   0,   0,3072,  28,          0,0,0,0,0,0},
  { 0,   4,  64,   0,  28,   0,   0,   0,       0,0,0,0,0}
};

// Double dummy table.  The order here is:
// Spades: North, East, South, West
// Hearts: same
// etc.
int DDtable[3][20] = {
  { 5, 8, 5, 8,  6, 6, 6, 6,  5, 7, 5, 7,  7, 5, 7, 5,  6, 6, 6, 6 },
  { 4, 9, 4, 9, 10, 2,10, 2,  8, 3, 8, 3,  6, 7, 6, 7,  9, 3, 9, 3 },
  { 3,10, 3,10,  9, 4, 9, 4,  8, 4, 8, 4,  3, 9, 3, 9,  4, 8, 4, 8 }
};

// Number of results expected for the play analysis.
// Generally the number of cards + 1.  For example, if only one
// card is played, then there is a result before the opening lead
// and after the opening lead.  Limited to 49 as the last trick 
//holds no excitement.
int traceNo[3] = { 46, 49, 13 };

// Results expected from the play analysis.  Padded with zeroes here.
int trace[3][53] = {
  {8,   8, 8, 8, 8,   8, 8, 8, 8,   8, 8, 8, 8,   8, 8, 8, 8, 
        8, 8, 8, 8,   8, 8, 8, 8,   8, 8, 8, 8,   8, 8, 8, 8, 
        8, 8, 8, 8,   8, 8, 8, 8,   8, 8, 8, 8,   8, 0, 0, 0,
        0, 0, 0, 0 },
  {9,  10,10,10,10,  10,10,10,10,  10,10,10,10,  10,10,10,10,
       10,10,10,10,  10,10,10,10,  10,10,10,10,  10,10,10,10,
       10,10,10,10,   9, 9, 9, 9,   9, 9, 9, 9,   9, 9, 9, 9,
        0, 0, 0, 0 },
  {10, 10,10,10,10,  10,10,10,10,  10,10,10,10,   0, 0, 0, 0,
        0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
        0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,
        0, 0, 0, 0 }
};
       
char parScore[3][2][10] = {
  { "NS -110", "EW 110"  },
  { "NS 100" , "EW -100" },
  { "NS -300", "EW 300"  }
};

char parString[3][2][10] = {
  { "NS:EW 2S" , "EW:EW 2S"  },
  { "NS:EW 4Sx", "EW:EW 4Sx" },
  { "NS:NS 5Hx", "EW:NS 5Hx" }
};

// Number of dealer par contracts expected.
int dealerParNo[3] = { 1, 1, 1 };

// Dealer par scores expected.
int dealerScore[3] = { -110, 100, -300 };

// Dealer par contracts expected, here only one per deal.
// That is not always the case.
char dealerContract[3][4][10] = { 
  { "2S-EW"   , "", "", "" },
  { "4S*-EW-1", "", "", "" },
  { "5H*-NS-2", "", "", "" }
};


//////////////////////////////////////////////////////////
//                 Useful constants                     //
//////////////////////////////////////////////////////////

unsigned short int dbitMapRank[16] =
{
  0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000
};

unsigned char dcardRank[16] =
{ 
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'
};

unsigned char dcardSuit[5] = { 'S', 'H', 'D', 'C', 'N' };
unsigned char dcardHand[4] = { 'N', 'E', 'S', 'W' };



void PrintFut(char title[], futureTricks * fut)
{
  printf("%s\n", title);

  printf("%6s %-6s %-6s %-6s %-6s\n",
         "card", "suit", "rank", "equals", "score");

  for (int i = 0; i < fut->cards; i++)
  {
    char res[15] = "";
    equals_to_string(fut->equals[i], res);
    printf("%6d %-6c %-6c %-6s %-6d\n",
           i,
           dcardSuit[ fut->suit[i] ],
           dcardRank[ fut->rank[i] ],
           res,
           fut->score[i]);
  }
  printf("\n");
}


void equals_to_string(int equals, char * res)
{
  int p = 0;
  int m = equals >> 2;
  for (int i = 15; i >= 2; i--)
  {
    if (m & static_cast<int>(dbitMapRank[i]))
      res[p++] = static_cast<char>(dcardRank[i]);
  }
  res[p] = 0;
}


bool CompareFut(futureTricks * fut, int handno, int solutions)
{
  if (solutions == 2)
  {
    if (fut->cards != cardsSoln2[handno])
      return false;
  }
  else if (fut->cards != cardsSoln3[handno])
    return false;

  for (int i = 0; i < fut->cards; i++)
  {
    if (fut->suit [i] != cardsSuits [handno][i]) return false;
    if (fut->rank [i] != cardsRanks [handno][i]) return false;
    if (fut->equals[i] != cardsEquals[handno][i]) return false;
    if (fut->score [i] != cardsScores[handno][i]) return false;
  }
  return true;
}


void SetTable(ddTableResults * table, int handno)
{
  for (int suit = 0; suit < DDS_STRAINS; suit++)
    for (int pl = 0; pl <= 3; pl++)
      table->resTable[suit][pl] = DDtable[handno][4 * suit + pl];
}


bool CompareTable(ddTableResults * table, int handno)
{
  for (int suit = 0; suit < DDS_STRAINS; suit++)
  {
    for (int pl = 0; pl <= 3; pl++)
    {
      if (table->resTable[suit][pl] != DDtable[handno][4 * suit + pl])
        return false;
    }
  }
  return true;
}


void PrintTable(ddTableResults * table)
{
  printf("%5s %-5s %-5s %-5s %-5s\n",
         "", "North", "South", "East", "West");

  printf("%5s %5d %5d %5d %5d\n",
         "NT",
         table->resTable[4][0],
         table->resTable[4][2],
         table->resTable[4][1],
         table->resTable[4][3]);

  for (int suit = 0; suit < DDS_SUITS; suit++)
  {
    printf("%5c %5d %5d %5d %5d\n",
           dcardSuit[suit],
           table->resTable[suit][0],
           table->resTable[suit][2],
           table->resTable[suit][1],
           table->resTable[suit][3]);
  }
  printf("\n");
}


bool ComparePar(parResults * par, int handno)
{
  if (strcmp(par->parScore[0], parScore[handno][0])) return false;
  if (strcmp(par->parScore[1], parScore[handno][1])) return false;

  if (strcmp(par->parContractsString[0], parString[handno][0]))
    return false;
  if (strcmp(par->parContractsString[1], parString[handno][1]))
    return false;
  return true;
}


bool CompareDealerPar(parResultsDealer * par, int handno)
{
  if (par->number != dealerParNo[handno]) return false;
  if (par->score != dealerScore[handno]) return false;

  for (int i = 0; i < par->number; i++)
  {
    if (strcmp(par->contracts[i], dealerContract[handno][i]))
      return false;
  }
  return true;
}


void PrintPar(parResults * par)
{
  printf("NS score: %s\n", par->parScore[0]);
  printf("EW score: %s\n", par->parScore[1]);
  printf("NS list : %s\n", par->parContractsString[0]);
  printf("EW list : %s\n", par->parContractsString[1]);
  printf("\n");
}


void PrintDealerPar(parResultsDealer * par)
{
  printf("Score : %d\n", par->score);
  printf("Pars : %d\n", par->number);

  for (int i = 0; i < par->number; i++)
    printf("Par %d : %s\n", i, par->contracts[i]);

  printf("\n");
}


bool ComparePlay(solvedPlay * solved, int handno)
{
  if (solved->number != traceNo[handno])
  {
    printf("err %d %d\n", solved->number, traceNo[handno]);
    return false;
  }

  for (int i = 0; i < solved->number; i++)
    if (solved->tricks[i] != trace[handno][i])
    {
      printf("error %d %d %d\n", i, solved->tricks[i],
             trace[handno][i]);
      return false;
    }

  return true;
}


void PrintBinPlay(playTraceBin * playp, solvedPlay * solved)
{
  printf("Number : %d\n", solved->number);

  printf("Play %2d: %s %d\n",
         0, "--", solved->tricks[0]);

  for (int i = 1; i < solved->number; i++)
  {
    printf("Play %2d: %c%c %d\n",
           i,
           dcardSuit[playp->suit[i - 1]],
           dcardRank[playp->rank[i - 1]],
           solved->tricks[i]);
  }
  printf("\n");
}


void PrintPBNPlay(playTracePBN * playp, solvedPlay * solved)
{
  printf("Number : %d\n", solved->number);

  printf("Play %2d: %s %d\n",
         0, "--", solved->tricks[0]);

  for (int i = 1; i < solved->number; i++)
  {
    printf("Play %2d: %c%c %2d\n",
           i,
           playp->cards[2 * (i - 1)],
           playp->cards[2 * i - 1],
           solved->tricks[i]);
  }
  printf("\n");
}



////////////////////////////////////////////////
// From here on it is code borrowed from DDS. //
////////////////////////////////////////////////


#define DDS_FULL_LINE 80
#define DDS_HAND_OFFSET 12
#define DDS_HAND_LINES 12

void PrintHand(char title[],
               unsigned int remainCards[DDS_HANDS][DDS_SUITS])
{
  int c, h, s, r;
  char text[DDS_HAND_LINES][DDS_FULL_LINE];

  for (int l = 0; l < DDS_HAND_LINES; l++)
  {
    memset(text[l], ' ', DDS_FULL_LINE);
    text[l][DDS_FULL_LINE - 1] = '\0';
  }

  for (h = 0; h < DDS_HANDS; h++)
  {
    int offset, line;
    if (h == 0)
    {
      offset = DDS_HAND_OFFSET;
      line = 0;
    }
    else if (h == 1)
    {
      offset = 2 * DDS_HAND_OFFSET;
      line = 4;
    }
    else if (h == 2)
    {
      offset = DDS_HAND_OFFSET;
      line = 8;
    }
    else
    {
      offset = 0;
      line = 4;
    }

    for (s = 0; s < DDS_SUITS; s++)
    {
      c = offset;
      for (r = 14; r >= 2; r--)
      {
        if ((remainCards[h][s] >> 2) & dbitMapRank[r])
          text[line + s][c++] = static_cast<char>(dcardRank[r]);
      }

      if (c == offset)
        text[line + s][c++] = '-';

      if (h != 3)
        text[line + s][c] = '\0';
    }
  }
  printf("%s", title);
  char dashes[80];
  int l = static_cast<int>(strlen(title)) - 1;
  for (int i = 0; i < l; i++)
    dashes[i] = '-';
  dashes[l] = '\0';
  printf("%s\n", dashes);
  for (int i = 0; i < DDS_HAND_LINES; i++)
    printf("%s\n", text[i]);
  printf("\n\n");
}


void PrintPBNHand(char title[], char remainCardsPBN[])
{
  unsigned int remainCards[DDS_HANDS][DDS_SUITS];
  ConvertPBN(remainCardsPBN, remainCards);
  PrintHand(title, remainCards);
}


int ConvertPBN(char * dealBuff,
               unsigned int remainCards[DDS_HANDS][DDS_SUITS])
{
  int bp = 0, firstl, card, hand, handRelFirst, suitInHand, h, s;

  for (h = 0; h < DDS_HANDS; h++)
    for (s = 0; s < DDS_SUITS; s++)
      remainCards[h][s] = 0;

  while (((dealBuff[bp] != 'W') && (dealBuff[bp] != 'N') &&
          (dealBuff[bp] != 'E') && (dealBuff[bp] != 'S') &&
          (dealBuff[bp] != 'w') && (dealBuff[bp] != 'n') &&
          (dealBuff[bp] != 'e') && (dealBuff[bp] != 's')) && (bp < 3))
    bp++;

  if (bp >= 3)
    return 0;

  if ((dealBuff[bp] == 'N') || (dealBuff[bp] == 'n'))
    firstl = 0;
  else if ((dealBuff[bp] == 'E') || (dealBuff[bp] == 'e'))
    firstl = 1;
  else if ((dealBuff[bp] == 'S') || (dealBuff[bp] == 's'))
    firstl = 2;
  else
    firstl = 3;

  bp++;
  bp++;

  handRelFirst = 0;
  suitInHand = 0;

  while ((bp < 80) && (dealBuff[bp] != '\0'))
  {
    card = IsACard(dealBuff[bp]);
    if (card)
    {
      switch (firstl)
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
        static_cast<unsigned>((dbitMapRank[card] << 2));

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


int IsACard(char cardChar)
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
    default :
      return 0;
  }
}

