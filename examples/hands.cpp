/* 
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// General initialization of three hands to be used in examples.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"

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

int trump_suit_ [3] = { SPADES  , NOTRUMP, SPADES   };
int first_hand_ [3] = { NORTH   , EAST   , SOUTH    };
int dealer_hand_[3] = { NORTH   , EAST   , NORTH    };
int vulnerability_ [3] = { VUL_NONE, VUL_NS , VUL_NONE };

char pbn_hands_[3][80] = {
"N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
"E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63",
"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
};

// The same hands in binary.  The second index is suit, the
// third index is hand.
unsigned int holdings_[3][4][4] =
{
  { // North       East     South          West
    { RQ|RJ|R6, R8|R7|R3, RK|R5, RA|RT|R9|R4|R2 } , // spades
    { RK|R6|R5|R2, RJ|R9|R7, RT|R8|R3, RA|RQ|R4 } , // hearts
    { RJ|R8|R5, RA|RT|R7|R6|R4, RK|RQ|R9, R3|R2 } , // diamonds
    { RT|R9|R8, RQ|R4, RA|R7|R6|R5|R2, RK|RJ|R3 }}, // clubs
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
int play_count_[3] = { 45, 52, 12 };

// Actual cards played before claim
char play_sequence_[3][106] = {
"CTC4CACJH8H4HKH9D5DAD9D2S7S5S2SQD8D4DQD3H3HAH6H7C3C8CQC2S3SKSAS6HQH5HJHTCKC9D6C5S4SJS8C6DJ",
"SQD2S8SAHKHTH3H2HQS2H4H6H8D6HJHAS7SKS4C4D8C2DKD4H9C5S6S3H7C7C3S5H5CTD9STD3DQDAC8S9SJC9DTCQD5CAC6DJCKCJD7",
"HAHKHQH7D7D8DAD9C5CAC6C3" 
};

int play_suit_[3][52] = {
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

int play_rank_[3][52] = {
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
int cards_solutions2_[3] = { 6, 3, 4 };

// Number of cards returned for solutions == 3, i.e. for
// all legally playable cards (taking into account equivalences).
int cards_solutions3_[3] = { 9, 7, 8 };

// Suits of cards returned. Padded with zeroes.
int card_suits_[3][13] = {
  { 2, 2, 2, 3, 0, 0, 1, 1, 1,    0, 0, 0, 0 },
  { 3, 3, 3, 1, 2, 0, 0,    0, 0, 0, 0, 0, 0 },
  { 1, 2, 2, 0, 1, 1, 3, 3,    0, 0, 0, 0, 0 }
};

// Ranks for cards returned (2 .. 14).  Padded with zeroes.
int card_ranks_[3][13] = {
  { 5, 8,11,10, 6,12, 2, 6,13,    0, 0, 0, 0 },
  { 2, 8,12,10, 6,12, 5,    0, 0, 0, 0, 0, 0 },
  {14, 3, 7, 5, 5, 9, 6,13,    0, 0, 0, 0, 0 }
};

// Scores for cards returned.
int card_scores_[3][13] = {
  { 5, 5, 5, 5, 5, 5, 4, 4, 4,    0, 0, 0, 0 },
  { 4, 4, 4, 3, 3, 3, 2,    0, 0, 0, 0, 0, 0 },
  { 3, 3, 3, 3, 2, 2, 1, 1,    0, 0, 0, 0, 0 }
};

// Equals for cards returned, i.e. equivalent cards (rank vectors).
int card_equals_[3][13] = {
  { 0,   0,   0, 768,   0,2048,   0,  32,   0,    0,0,0,0},
  { 0,   0,2048,   0,   0,3072,  28,          0,0,0,0,0,0},
  { 0,   4,  64,   0,  28,   0,   0,   0,       0,0,0,0,0}
};

// Double dummy table.  The order here is:
// Spades: North, East, South, West
// Hearts: same
// etc.
int dd_table_[3][20] = {
  { 5, 8, 5, 8,  6, 6, 6, 6,  5, 7, 5, 7,  7, 5, 7, 5,  6, 6, 6, 6 },
  { 4, 9, 4, 9, 10, 2,10, 2,  8, 3, 8, 3,  6, 7, 6, 7,  9, 3, 9, 3 },
  { 3,10, 3,10,  9, 4, 9, 4,  8, 4, 8, 4,  3, 9, 3, 9,  4, 8, 4, 8 }
};

// Number of results expected for the play analysis.
// Generally the number of cards + 1.  For example, if only one
// card is played, then there is a result before the opening lead
// and after the opening lead.  Limited to 49 as the last trick 
//holds no excitement.
int trace_count_[3] = { 46, 49, 13 };

// Results expected from the play analysis.  Padded with zeroes here.
int trace_results_[3][53] = {
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
       
char par_scores_[3][2][10] = {
  { "NS -110", "EW 110"  },
  { "NS 100" , "EW -100" },
  { "NS -300", "EW 300"  }
};

char par_strings_[3][2][10] = {
  { "NS:EW 2S" , "EW:EW 2S"  },
  { "NS:EW 4Sx", "EW:EW 4Sx" },
  { "NS:NS 5Hx", "EW:NS 5Hx" }
};

// Number of dealer par contracts expected.
int dealer_par_count_[3] = { 1, 1, 1 };

// Dealer par scores expected.
int dealer_scores_[3] = { -110, 100, -300 };

// Dealer par contracts expected, here only one per Deal.
// That is not always the case.
char dealer_contracts_[3][4][10] = { 
  { "2S-EW"   , "", "", "" },
  { "4S*-EW-1", "", "", "" },
  { "5H*-NS-2", "", "", "" }
};


//////////////////////////////////////////////////////////
//                 Useful constants                     //
//////////////////////////////////////////////////////////

unsigned short int bit_map_rank_[16] =
{
  0x0000, 0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000
};

unsigned char card_rank_chars_[16] =
{ 
  'x', 'x', '2', '3', '4', '5', '6', '7',
  '8', '9', 'T', 'J', 'Q', 'K', 'A', '-'
};

unsigned char card_suit_chars_[5] = { 'S', 'H', 'D', 'C', 'N' };
unsigned char card_hand_chars_[4] = { 'N', 'E', 'S', 'W' };



auto print_future_tricks(char title[], FutureTricks * fut) -> void
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
           card_suit_chars_[ fut->suit[i] ],
           card_rank_chars_[ fut->rank[i] ],
           res,
           fut->score[i]);
  }
  printf("\n");
}


auto equals_to_string(int equals, char * res) -> void
{
  int pos = 0;
  int mask = equals >> 2;
  for (int i = 15; i >= 2; i--)
  {
    if (mask & static_cast<int>(bit_map_rank_[i]))
      res[pos++] = static_cast<char>(card_rank_chars_[i]);
  }
  res[pos] = 0;
}


auto compare_future_tricks(FutureTricks * fut, int handno, int solutions) -> bool
{
  if (solutions == 2)
  {
    if (fut->cards != cards_solutions2_[handno])
      return false;
  }
  else if (fut->cards != cards_solutions3_[handno])
    return false;

  for (int i = 0; i < fut->cards; i++)
  {
    if (fut->suit [i] != card_suits_ [handno][i]) return false;
    if (fut->rank [i] != card_ranks_ [handno][i]) return false;
    if (fut->equals[i] != card_equals_[handno][i]) return false;
    if (fut->score [i] != card_scores_[handno][i]) return false;
  }
  return true;
}


auto set_table(DdTableResults * table, int handno) -> void
{
  for (int suit = 0; suit < DDS_STRAINS; suit++)
    for (int pl = 0; pl <= 3; pl++)
      table->res_table[suit][pl] = dd_table_[handno][4 * suit + pl];
}


auto compare_table(DdTableResults * table, int handno) -> bool
{
  for (int suit = 0; suit < DDS_STRAINS; suit++)
  {
    for (int pl = 0; pl <= 3; pl++)
    {
      if (table->res_table[suit][pl] != dd_table_[handno][4 * suit + pl])
        return false;
    }
  }
  return true;
}


auto print_table(DdTableResults * table) -> void
{
  printf("%5s %-5s %-5s %-5s %-5s\n",
         "", "North", "South", "East", "West");

  printf("%5s %5d %5d %5d %5d\n",
         "NT",
         table->res_table[4][0],
         table->res_table[4][2],
         table->res_table[4][1],
         table->res_table[4][3]);

  for (int suit = 0; suit < DDS_SUITS; suit++)
  {
    printf("%5c %5d %5d %5d %5d\n",
           card_suit_chars_[suit],
           table->res_table[suit][0],
           table->res_table[suit][2],
           table->res_table[suit][1],
           table->res_table[suit][3]);
  }
  printf("\n");
}


auto compare_par(ParResults * par, int handno) -> bool
{
  if (strcmp(par->par_score[0], par_scores_[handno][0])) return false;
  if (strcmp(par->par_score[1], par_scores_[handno][1])) return false;

  if (strcmp(par->par_contracts_string[0], par_strings_[handno][0]))
    return false;
  if (strcmp(par->par_contracts_string[1], par_strings_[handno][1]))
    return false;
  return true;
}


auto compare_dealer_par(ParResultsDealer * par, int handno) -> bool
{
  if (par->number != dealer_par_count_[handno]) return false;
  if (par->score != dealer_scores_[handno]) return false;

  for (int i = 0; i < par->number; i++)
  {
    if (strcmp(par->contracts[i], dealer_contracts_[handno][i]))
      return false;
  }
  return true;
}


auto print_par(ParResults * par) -> void
{
  printf("NS score: %s\n", par->par_score[0]);
  printf("EW score: %s\n", par->par_score[1]);
  printf("NS list : %s\n", par->par_contracts_string[0]);
  printf("EW list : %s\n", par->par_contracts_string[1]);
  printf("\n");
}


auto print_dealer_par(ParResultsDealer * par) -> void
{
  printf("Score : %d\n", par->score);
  printf("Pars : %d\n", par->number);

  for (int i = 0; i < par->number; i++)
    printf("Par %d : %s\n", i, par->contracts[i]);

  printf("\n");
}


auto compare_play(SolvedPlay * solved, int handno) -> bool
{
  if (solved->number != trace_count_[handno])
  {
    printf("err %d %d\n", solved->number, trace_count_[handno]);
    return false;
  }

  for (int i = 0; i < solved->number; i++)
    if (solved->tricks[i] != trace_results_[handno][i])
    {
      printf("error %d %d %d\n", i, solved->tricks[i],
             trace_results_[handno][i]);
      return false;
    }

  return true;
}


auto print_bin_play(PlayTraceBin * playp, SolvedPlay * solved) -> void
{
  printf("Number : %d\n", solved->number);

  printf("Play %2d: %s %d\n",
         0, "--", solved->tricks[0]);

  for (int i = 1; i < solved->number; i++)
  {
    printf("Play %2d: %c%c %d\n",
           i,
           card_suit_chars_[playp->suit[i - 1]],
           card_rank_chars_[playp->rank[i - 1]],
           solved->tricks[i]);
  }
  printf("\n");
}


auto print_pbn_play(PlayTracePBN * playp, SolvedPlay * solved) -> void
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

auto print_hand(char title[],
               unsigned int remainCards[DDS_HANDS][DDS_SUITS]) -> void
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
        if ((remainCards[h][s] >> 2) & bit_map_rank_[r])
          text[line + s][c++] = static_cast<char>(card_rank_chars_[r]);
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


auto print_pbn_hand(char title[], char remainCardsPBN[]) -> void
{
  unsigned int remainCards[DDS_HANDS][DDS_SUITS];
  convert_pbn(remainCardsPBN, remainCards);
  print_hand(title, remainCards);
}


auto convert_pbn(char * dealBuff,
               unsigned int remainCards[DDS_HANDS][DDS_SUITS]) -> int
{
  int buffer_pos = 0, first_hand, card, hand, hand_rel_first, suit_in_hand, h, s;

  for (h = 0; h < DDS_HANDS; h++)
    for (s = 0; s < DDS_SUITS; s++)
      remainCards[h][s] = 0;

  while (((dealBuff[buffer_pos] != 'W') && (dealBuff[buffer_pos] != 'N') &&
          (dealBuff[buffer_pos] != 'E') && (dealBuff[buffer_pos] != 'S') &&
          (dealBuff[buffer_pos] != 'w') && (dealBuff[buffer_pos] != 'n') &&
          (dealBuff[buffer_pos] != 'e') && (dealBuff[buffer_pos] != 's')) && (buffer_pos < 3))
    buffer_pos++;

  if (buffer_pos >= 3)
    return 0;

  if ((dealBuff[buffer_pos] == 'N') || (dealBuff[buffer_pos] == 'n'))
    first_hand = 0;
  else if ((dealBuff[buffer_pos] == 'E') || (dealBuff[buffer_pos] == 'e'))
    first_hand = 1;
  else if ((dealBuff[buffer_pos] == 'S') || (dealBuff[buffer_pos] == 's'))
    first_hand = 2;
  else
    first_hand = 3;

  buffer_pos++;
  buffer_pos++;

  hand_rel_first = 0;
  suit_in_hand = 0;

  while ((buffer_pos < 80) && (dealBuff[buffer_pos] != '\0'))
  {
    card = is_a_card(dealBuff[buffer_pos]);
    if (card)
    {
      switch (first_hand)
      {
        case 0:
          hand = hand_rel_first;
          break;
        case 1:
          if (hand_rel_first == 0)
            hand = 1;
          else if (hand_rel_first == 3)
            hand = 0;
          else
            hand = hand_rel_first + 1;
          break;
        case 2:
          if (hand_rel_first == 0)
            hand = 2;
          else if (hand_rel_first == 1)
            hand = 3;
          else
            hand = hand_rel_first - 2;
          break;
        default:
          if (hand_rel_first == 0)
            hand = 3;
          else
            hand = hand_rel_first - 1;
      }

      remainCards[hand][suit_in_hand] |=
        static_cast<unsigned>((bit_map_rank_[card] << 2));

    }
    else if (dealBuff[buffer_pos] == '.')
      suit_in_hand++;
    else if (dealBuff[buffer_pos] == ' ')
    {
      hand_rel_first++;
      suit_in_hand = 0;
    }
    buffer_pos++;
  }
  return RETURN_NO_FAULT;
}


auto is_a_card(char cardChar) -> int
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

