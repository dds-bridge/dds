/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/dll.h"
#include "../include/portab.h"
#include "testStats.h"


#ifdef _WIN32
  LARGE_INTEGER frequency, tu0, tu1;
#else
#include <sys/time.h>
  timeval tu0, tu1;
#endif

#include "dtest.h"


#define SOLVE_SIZE MAXNOOFBOARDS
#define BOARD_SIZE MAXNOOFTABLES
#define TRACE_SIZE MAXNOOFBOARDS
#define PAR_REPEAT 1

int input_number;
bool GIBmode = false;


// #define DEBUG
#define BATCHTIMES
#define ZERO (static_cast<int>('0'))
#define NINE (static_cast<int>('9'))
#define SPACE (static_cast<int>(' '))
#define QUOTE (static_cast<int>('"'))



clock_t ts0, ts1;
int tu , ts, ctu, cts;


int realMain(int argc, char * argv[]);

int realMain(int argc, char * argv[])
{
  input_number = 0;
  ctu = 0;
  cts = 0;

  TestSetTimerName("Timer title");

  if (argc != 3 && argc != 4)
  {
    printf(
      "Usage: dtest file.txt solve|calc|par|dealerpar|play [ncores]\n");
    return 1;
  }

  char * fname = argv[1];
  char * type = argv[2];

  if (! strcmp(type, "solve"))
    input_number = SOLVE_SIZE;
  else if (! strcmp(type, "calc"))
    input_number = BOARD_SIZE;
  else if (! strcmp(type, "par"))
    input_number = PAR_REPEAT;
  else if (! strcmp(type, "dealerpar"))
    input_number = PAR_REPEAT;
  else if (! strcmp(type, "play"))
    input_number = TRACE_SIZE;

  set_constants();
  main_identify();
#ifdef _WIN32
  QueryPerformanceFrequency(&frequency);
#endif

  boardsPBN bop;
  solvedBoards solvedbdp;
  ddTableDealsPBN dealsp;
  ddTablesRes resp;
  allParResults parp;
  playTracesPBN playsp;
  solvedPlays solvedplp;

  int * dealer_list;
  int * vul_list;
  dealPBN * deal_list;
  futureTricks * fut_list;
  ddTableResults * table_list;
  parResults * par_list;
  parResultsDealer * dealerpar_list;
  playTracePBN * play_list;
  solvedPlay * trace_list;
  int number;

  if (read_file(fname, &number, &dealer_list, &vul_list,
        &deal_list, &fut_list, &table_list, &par_list, &dealerpar_list,
        &play_list, &trace_list) == false)
  {
    printf("read_file failed.\n");
    exit(0);
  }

  if (! strcmp(type, "solve"))
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_solve(&bop, &solvedbdp, deal_list, fut_list, number);
  }
  else if (! strcmp(type, "calc"))
  {
    loop_calc(&dealsp, &resp, &parp,
              deal_list, table_list, number);
  }
  else if (! strcmp(type, "par"))
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_par(vul_list, table_list, par_list, number);
  }
  else if (! strcmp(type, "dealerpar"))
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_dealerpar(dealer_list, vul_list, table_list,
                   dealerpar_list, number);
  }
  else if (! strcmp(type, "play"))
  {
    if (GIBmode)
    {
      printf("GIB file does not work with solve\n");
      exit(0);
    }
    loop_play(&bop, &playsp, &solvedplp,
              deal_list, play_list, trace_list, number);
  }
  else
  {
    printf("Unknown type %s\n", type);
    exit(0);
  }

  print_times(number);
  TestPrintTimer();
  TestPrintTimerList();
  TestPrintCounter();

  free(dealer_list);
  free(vul_list);
  free(deal_list);
  free(fut_list);
  free(table_list);
  free(par_list);
  free(dealerpar_list);
  free(play_list);
  free(trace_list);

  return (0);
}


void main_identify()
{
  printf("dtest main\n----------\n");

#if defined(_WIN32) || defined(__CYGWIN__)
  printf("%-12s %20s\n", "System", "Windows");
#if defined(_MSC_VER)
  printf("%-12s %20s\n", "Compiler", "Microsoft Visual C++");
#elif defined(__MINGW32__)
  printf("%-12s %20s\n", "Compiler", "MinGW");
#else
  printf("%-12s %20s\n", "Compiler", "GNU g++");
#endif

#elif defined(__linux)
  printf("%-12s %20s\n", "System", "Linux");
  printf("%-12s %20s\n", "Compiler", "GNU g++");

#elif defined(__APPLE__)
  printf("%-12s %20s\n", "System", "Apple");
#if defined(__clang__)
  printf("%-12s %20s\n", "Compiler", "clang");
#else
  printf("%-12s %20s\n", "Compiler", "GNU g++");
#endif
#endif

#if defined(__cplusplus)
  printf("%-12s %20ld\n", "Dialect", __cplusplus);
#endif

  printf("\n");
}


void set_constants()
{
  dbitMapRank[15] = 0x2000;
  dbitMapRank[14] = 0x1000;
  dbitMapRank[13] = 0x0800;
  dbitMapRank[12] = 0x0400;
  dbitMapRank[11] = 0x0200;
  dbitMapRank[10] = 0x0100;
  dbitMapRank[ 9] = 0x0080;
  dbitMapRank[ 8] = 0x0040;
  dbitMapRank[ 7] = 0x0020;
  dbitMapRank[ 6] = 0x0010;
  dbitMapRank[ 5] = 0x0008;
  dbitMapRank[ 4] = 0x0004;
  dbitMapRank[ 3] = 0x0002;
  dbitMapRank[ 2] = 0x0001;
  dbitMapRank[ 1] = 0;
  dbitMapRank[ 0] = 0;

  dcardRank[ 2] = '2';
  dcardRank[ 3] = '3';
  dcardRank[ 4] = '4';
  dcardRank[ 5] = '5';
  dcardRank[ 6] = '6';
  dcardRank[ 7] = '7';
  dcardRank[ 8] = '8';
  dcardRank[ 9] = '9';
  dcardRank[10] = 'T';
  dcardRank[11] = 'J';
  dcardRank[12] = 'Q';
  dcardRank[13] = 'K';
  dcardRank[14] = 'A';
  dcardRank[15] = '-';

  dcardSuit[0] = 'S';
  dcardSuit[1] = 'H';
  dcardSuit[2] = 'D';
  dcardSuit[3] = 'C';
  dcardSuit[4] = 'N';
}


bool read_file(
  char * fname,
  int * number,
  int ** dealer_list,
  int ** vul_list,
  dealPBN ** deal_list,
  futureTricks ** fut_list,
  ddTableResults ** table_list,
  parResults ** par_list,
  parResultsDealer ** dealerpar_list,
  playTracePBN ** play_list,
  solvedPlay ** trace_list)
{
  char line[256];
  char * name;

  FILE * fp;
  fp = fopen(fname, "r");
  if (fp == nullptr)
  {
    char backup[80];
    sprintf(backup, "../hands/%s", fname);
    fp = fopen(backup, "r");
    name = backup;
    if (fp == nullptr)
      return false;
  }
  else
    name = fname;

  if (! fgets(line, sizeof(line), fp))
    return false;
  if (parse_NUMBER(line, number) == false)
  {
    if (parseable_GIB(line))
    {
      GIBmode = true;

      * number = 1;
      // Count lines, then start over.
      while (1)
      {
        if (! fgets(line, sizeof(line), fp))
          break;
        (*number)++;
      }

      fclose(fp);
      fp = fopen(name, "r");
      if (fp == nullptr)
        return false;
    }
    else
      return false;
  }

  if (*number < 0 || *number > 100000)
  {
    printf("Suspect number %d\n", *number);
    return false;
  }

  size_t number_t = static_cast<size_t>(* number);

  if ((*dealer_list = static_cast<int *>
      (calloc(number_t, sizeof(int)))) == nullptr)
    return false;

  if ((*vul_list = static_cast<int *>
      (calloc(number_t, sizeof(int)))) == nullptr)
    return false;

  if ((*deal_list = static_cast<dealPBN *>
      (calloc(number_t, sizeof(dealPBN)))) == nullptr)
    return false;

  if ((*fut_list = static_cast<futureTricks *>
      (calloc(number_t, sizeof(futureTricks)))) == nullptr)
    return false;

  if ((*table_list = static_cast<ddTableResults *>
      (calloc(number_t, sizeof(ddTableResults)))) == nullptr)
    return false;

  if ((*par_list = static_cast<parResults *>
      (calloc(number_t, sizeof(parResults)))) == nullptr)
    return false;

  if ((*dealerpar_list = static_cast<parResultsDealer *>
      (calloc(number_t, sizeof(parResultsDealer)))) == nullptr)
    return false;

  if ((*play_list = static_cast<playTracePBN *>
      (calloc(number_t, sizeof(playTracePBN)))) == nullptr)
    return false;

  if ((*trace_list = static_cast<solvedPlay *>
      (calloc(number_t, sizeof(solvedPlay)))) == nullptr)
    return false;

  if (GIBmode)
  {
    int n = 0;
    while (fgets(line, sizeof(line), fp))
    {
      if (parse_GIB(line, &(*deal_list)[n], &(*table_list)[n])
          == false)
        return false;
      n++;
    }
  }
  else
  {
    for (int n = 0; n < *number; n++)
    {
#ifdef DEBUG
      printf("Starting to read hand number %d\n", n);
      printf("-------------------------------\n");
      printf("play_list[%d].number = %d\n", n, (*play_list)[n].number);
#endif

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_PBN(line, &(*dealer_list)[n], &(*vul_list)[n],
                    &(*deal_list)[n]) == false) return false;

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_FUT(line, &(*fut_list)[n]) == false) return false;

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_TABLE(line, &(*table_list)[n]) == false) return false;

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_PAR(line, &(*par_list)[n]) == false) return false;

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_DEALERPAR(line, &(*dealerpar_list)[n]) == false)
        return false;

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_PLAY(line, &(*play_list)[n]) == false) return false;

      if (! fgets(line, sizeof(line), fp)) return false;
      if (parse_TRACE(line, &(*trace_list)[n]) == false) return false;

      /*
      print_PBN (&(*deal_list )[n]);
      print_FUT (&(*fut_list )[n]);
      print_TABLE (&(*table_list )[n]);
      print_PAR (&(*par_list )[n]);
      print_DEALERPAR(&(*dealerpar_list)[n]);
      print_PLAY (&(*play_list )[n]);
      print_TRACE (&(*trace_list )[n]);
      */
    }
  }

  fclose(fp);
  return 1;
}


bool parse_NUMBER(char * line, int * number)
{
#ifdef DEBUG
  printf("parse_NUMBER: Got line '%s'\n", line);
#endif
  int pos = 0;
  if (consume_tag(line, &pos, "NUMBER") == false) return false;
#ifdef DEBUG
  printf("parse_NUMBER: Got tag 'NUMBER'\n");
#endif

  if (consume_int(line, &pos, number) == false) return false;
#ifdef DEBUG
  printf("parse_NUMBER: Read number %d\n", * number);
#endif
  return true;
}


bool parse_PBN(
  char * line,
  int * dealer,
  int * vul,
  dealPBN * dl)
{
#ifdef DEBUG
  printf("parse_PBN: Got line '%s'\n", line);
#endif
  int pos = 0;
  if (consume_tag(line, &pos, "PBN") == false) return false;
#ifdef DEBUG
  printf("parse_PBN: Got tag 'PBN'\n");
#endif

  if (consume_int(line, &pos, dealer) == false) return false;
#ifdef DEBUG
  printf("parse_PBN: Read dealer %d\n", * dealer);
#endif

  if (consume_int(line, &pos, vul) == false) return false;
#ifdef DEBUG
  printf("parse_PBN: Read vul %d\n", * dealer);
#endif

  if (consume_int(line, &pos, &dl->trump) == false) return false;
#ifdef DEBUG
  printf("parse_PBN: Read trump %d\n", dl->trump);
#endif

  if (consume_int(line, &pos, &dl->first) == false) return false;
#ifdef DEBUG
  printf("parse_PBN: Read first %d\n", dl->first);
#endif

  if (consume_string(line, &pos, dl->remainCards) == false)
    return false;
#ifdef DEBUG
  printf("parse_PBN: Read string '%s'\n", dl->remainCards);
#endif

  return true;
}


bool parse_FUT(char * line, futureTricks * fut)
{
#ifdef DEBUG
  printf("parse_FUT: Got line '%s'\n", line);
#endif
  int pos = 0;
  if (consume_tag(line, &pos, "FUT") == false) return false;
#ifdef DEBUG
  printf("parse_FUT: Got tag 'FUT'\n");
#endif

  if (consume_int(line, &pos, &fut->cards) == false) return false;
#ifdef DEBUG
  printf("parse_FUT: Read cards %d\n", fut->cards);
#endif

  for (int c = 0; c < fut->cards; c++)
  {
    if (consume_int(line, &pos, &fut->suit[c]) == false) return false;
#ifdef DEBUG
    printf("parse_FUT: Read suit[%d]: %d\n", c, fut->suit[c]);
#endif
  }

  for (int c = 0; c < fut->cards; c++)
  {
    if (consume_int(line, &pos, &fut->rank[c]) == false) return false;
#ifdef DEBUG
    printf("parse_FUT: Read rank[%d]: %d\n", c, fut->rank[c]);
#endif
  }

  for (int c = 0; c < fut->cards; c++)
  {
    if (consume_int(line, &pos, &fut->equals[c]) == false) return false;
#ifdef DEBUG
    printf("parse_FUT: Read equals[%d]: %d\n", c, fut->equals[c]);
#endif
  }

  for (int c = 0; c < fut->cards; c++)
  {
    if (consume_int(line, &pos, &fut->score[c]) == false) return false;
#ifdef DEBUG
    printf("parse_FUT: Read score[%d]: %d\n", c, fut->score[c]);
#endif
  }

  return true;
}


bool parse_TABLE(char * line, ddTableResults * table)
{
  int pos = 0;
  if (consume_tag(line, &pos, "TABLE") == false) return false;
#ifdef DEBUG
  printf("parse_FUT: Got tag 'TABLE'\n");
#endif

  for (int suit = 0; suit <= 4; suit++)
  {
    for (int pl = 0; pl <= 3; pl++)
    {
      if (consume_int(line, &pos, &table->resTable[suit][pl]) == false)
        return false;
#ifdef DEBUG
      printf("parse_TABLE: Read table[%d][%d] = %d\n",
             suit, pl, table->resTable[suit][pl]);
#endif
    }
  }
  return true;
}


bool parse_PAR(char * line, parResults * par)
{
#ifdef DEBUG
  printf("parse_PAR: Got line '%s'\n", line);
#endif

  int pos = 0;
  if (consume_tag(line, &pos, "PAR") == false) return false;
#ifdef DEBUG
  printf("parse_PAR: Got tag 'PAR'\n");
#endif

  if (consume_string(line, &pos, par->parScore[0]) == false)
    return false;
#ifdef DEBUG
  printf("parse_PAR: Read string '%s'\n", par->parScore[0]);
#endif

  if (consume_string(line, &pos, par->parScore[1]) == false)
    return false;
#ifdef DEBUG
  printf("parse_PAR: Read string '%s'\n", par->parScore[1]);
#endif

  if (consume_string(line, &pos, par->parContractsString[0]) == false)
    return false;
#ifdef DEBUG
  printf("parse_PAR: Read string '%s'\n", par->parContractsString[0]);
#endif

  if (consume_string(line, &pos, par->parContractsString[1]) == false)
    return false;
#ifdef DEBUG
  printf("parse_PAR: Read string '%s'\n", par->parContractsString[1]);
#endif

  return true;
}


bool parse_DEALERPAR(char * line, parResultsDealer * par)
{
#ifdef DEBUG
  printf("parse_DEALERPAR: Got line '%s'\n", line);
#endif

  int pos = 0;
  if (consume_tag(line, &pos, "PAR2") == false) return false;
#ifdef DEBUG
  printf("parse_DEALERPAR: Got tag 'PAR2'\n");
#endif

  char str[256];
  if (consume_string(line, &pos, str) == false)
    return false;
#ifdef DEBUG
  printf("parse_DEALERPAR: Read string '%s'\n", str);
#endif
  if (sscanf(str, "%d", &par->score) != 1)
    return false;
#ifdef DEBUG
  printf("parse_DEALERPAR: Read string '%s', number %d\n",
         str, par->score);
#endif

  int no = 0;
  while (1)
  {
    if (consume_string(line, &pos, par->contracts[no]) == false)
      break;
#ifdef DEBUG
    printf("parse_DEALERPAR: Read string number %d, '%s'\n",
           no, par->contracts[no]);
#endif
    no++;
  }

  par->number = no;
#ifdef DEBUG
  printf("parse_DEALERPAR: Read to number incl %d\n", no - 1);
#endif

  return true;
}


bool parse_PLAY(char * line, playTracePBN * playp)
{
#ifdef DEBUG
  printf("parse_PLAY: Got line '%s'\n", line);
#endif

  int pos = 0;
  if (consume_tag(line, &pos, "PLAY") == false) return false;
#ifdef DEBUG
  printf("parse_PLAY: Got tag 'PLAY', pos now %d\n", pos);
#endif

  if (consume_int(line, &pos, &playp->number) == false) return false;
#ifdef DEBUG
  printf("parse_PLAY: Read number %d\n", playp->number);
#endif

  if (consume_string(line, &pos, playp->cards) == false)
    return false;
#ifdef DEBUG
  printf("parse_PLAY: Read cards '%s'\n", playp->cards);
#endif

  return true;
}


bool parse_TRACE(char * line, solvedPlay * solvedp)
{
#ifdef DEBUG
  printf("parse_TRACE: Got line '%s'\n", line);
#endif

  int pos = 0;
  if (consume_tag(line, &pos, "TRACE") == false) return false;
#ifdef DEBUG
  printf("parse_TRACE: Got tag 'TRACE'\n");
#endif

  if (consume_int(line, &pos, &solvedp->number) == false) return false;
#ifdef DEBUG
  printf("parse_TRACE: Read number %d\n", solvedp->number);
#endif

  for (int i = 0; i < solvedp->number; i++)
  {
    if (consume_int(line, &pos, &solvedp->tricks[i]) == false)
      return false;
#ifdef DEBUG
    printf("parse_TRACE: Read tricks[%d] = %d\n", i, solvedp->tricks[i]);
#endif
  }
  return true;
}


bool parseable_GIB(char line[])
{
  if (strlen(line) != 89)
    return false;

  if (line[67] != ':')
    return false;

  return true;
}

int GIB_TO_DDS[4] = {1, 0, 3, 2};

bool parse_GIB(char line[], dealPBN * dl, ddTableResults * table)
{
  strcpy(dl->remainCards, "W:");
  strncpy(dl->remainCards + 2, line, 67);
  dl->remainCards[69] = '\0';

  int zero = static_cast<int>('0');
  int leta = static_cast<int>('A') - 10;
  int dds_strain, dds_hand;

  for (int s = 0; s < DDS_STRAINS; s++)
  {
    dds_strain = (s == 0 ? 4 : s - 1);
    for (int h = 0; h < DDS_HANDS; h++)
    {
      dds_hand = GIB_TO_DDS[h];
      char c = line[68 + 4 * s + h];
      int d;
      if (c >= '0' && c <= '9')
        d = static_cast<int> (line[68 + 4 * s + h] - zero);
      else if (c >= 'A' && c <= 'F')
        d = static_cast<int> (line[68 + 4 * s + h] - leta);
      else
        return false;

      if (dds_hand & 1)
        d = 13 - d;

      table->resTable[dds_strain][dds_hand] = d;
    }
  }
  return true;
}


bool compare_PBN(dealPBN * dl1, dealPBN * dl2)
{
  if (dl1->trump != dl2->trump) return false;
  if (dl1->first != dl2->first) return false;
  if (strcmp(dl1->remainCards, dl2->remainCards)) return false;
  return true;
}


bool compare_FUT(futureTricks * fut1, futureTricks * fut2)
{
  if (fut1->cards != fut2->cards)
    return false;

// TEMPNODE
// printf(" %8d\n", fut1->nodes);
  for (int i = 0; i < fut1->cards; i++)
  {
    if (fut1->suit [i] != fut2->suit [i]) return false;
    if (fut1->rank [i] != fut2->rank [i]) return false;
    if (fut1->equals[i] != fut2->equals[i]) return false;
    if (fut1->score [i] != fut2->score [i]) return false;
  }
  return true;
}


bool compare_TABLE(ddTableResults * table1, ddTableResults * table2)
{
  for (int suit = 0; suit <= 4; suit++)
  {
    for (int pl = 0; pl <= 3; pl++)
    {
      if (table1->resTable[suit][pl] != table2->resTable[suit][pl])
        return false;
    }
  }
  return true;
}


bool compare_PAR(
  parResults * par1,
  parResults * par2)
{
  if (strcmp(par1->parScore[0], par2->parScore[0])) return false;
  if (strcmp(par1->parScore[1], par2->parScore[1])) return false;

  if (strcmp(par1->parContractsString[0],
             par2->parContractsString[0]))
    return false;
  if (strcmp(par1->parContractsString[1],
             par2->parContractsString[1]))
    return false;
  return true;
}


bool compare_DEALERPAR(
  parResultsDealer * par1,
  parResultsDealer * par2)
{
  if (par1->score != par2->score) return false;

  for (int i = 0; i < par1->number; i++)
  {
    if (strcmp(par1->contracts[i], par2->contracts[i]))
      return false;
  }
  return true;
}


bool compare_TRACE(
  solvedPlay * trace1,
  solvedPlay * trace2)
{
  // In a buglet, Trace returned trace1 == -3 if there is
  // no input at all (trace2 is then 0).
  if (trace1->number != trace2->number &&
      trace2->number > 0)
  {
    printf("number %d != %d\n", trace1->number, trace2->number);
    return true;
  }

  // Once that was fixed, the input file had length 0, not 1.
  if (trace1->number == 1 && trace2->number == 0)
    return true;

  for (int i = 0; i < trace1->number; i++)
  {
    if (trace1->tricks[i] != trace2->tricks[i])
    {
      printf("i %d: %d != %d\n", i, trace1->tricks[i],
             trace2->tricks[i]);
      return false;
    }
  }
  return true;
}


bool print_PBN(dealPBN * dl)
{
  printf("%10s %d\n", "trump", dl->trump);
  printf("%10s %d\n", "first", dl->first);
  printf("%10s %s\n", "cards", dl->remainCards);
  return true;
}


bool print_FUT(futureTricks * fut)
{
  printf("%6s %d\n", "cards", fut->cards);
  printf("%6s %-6s %-6s %-6s %-6s\n",
         "", "suit", "rank", "equals", "score");

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
  return true;
}


void equals_to_string(int equals, char * res)
{
  int p = 0;
  for (int i = 15; i >= 2; i--)
  {
    if (equals & dbitMapRank[i])
      res[p++] = static_cast<char>(dcardRank[i]);
  }
  res[p] = 0;
}


bool print_TABLE(ddTableResults * table)
{
  printf("%5s %-5s %-5s %-5s %-5s\n",
         "", "North", "South", "East", "West");

  printf("%5s %5d %5d %5d %5d\n",
         "NT",
         table->resTable[4][0],
         table->resTable[4][2],
         table->resTable[4][1],
         table->resTable[4][3]);

  for (int suit = 0; suit <= 3; suit++)
  {
    printf("%5c %5d %5d %5d %5d\n",
           dcardSuit[suit],
           table->resTable[suit][0],
           table->resTable[suit][2],
           table->resTable[suit][1],
           table->resTable[suit][3]);
  }
  return true;
}


bool print_PAR(parResults * par)
{
  printf("NS score: %s\n", par->parScore[0]);
  printf("EW score: %s\n", par->parScore[1]);
  printf("NS list : %s\n", par->parContractsString[0]);
  printf("EW list : %s\n", par->parContractsString[1]);
  return true;
}


bool print_DEALERPAR(parResultsDealer * par)
{
  printf("Score : %d\n", par->score);
  printf("Pars : %d\n", par->number);

  for (int i = 0; i < par->number; i++)
  {
    printf("Par %d: %s\n", i, par->contracts[i]);
  }
  return true;
}


bool print_PLAY(playTracePBN * play)
{
  printf("Number : %d\n", play->number);

  for (int i = 0; i < play->number; i++)
  {
    printf("Play %d: %c%c\n", i, play->cards[2 * i], play->cards[2 * i + 1]);
  }
  return true;
}


bool print_TRACE(solvedPlay * solvedp)
{
  printf("Number : %d\n", solvedp->number);

  for (int i = 0; i < solvedp->number; i++)
  {
    printf("Trick %d: %d\n", i, solvedp->tricks[i]);
  }
  return true;
}


void loop_solve(
  boardsPBN * bop,
  solvedBoards * solvedbdp,
  dealPBN * deal_list,
  futureTricks * fut_list,
  int number)
{
#ifdef BATCHTIMES
  printf("%8s %24s\n", "Hand no.", "Time");
#endif

  for (int i = 0; i < number; i += input_number)
  {
    int count = (i + input_number > number ? number - i : input_number);

    bop->noOfBoards = count;
    for (int j = 0; j < count; j++)
    {
      bop->deals[j] = deal_list[i + j];
      bop->target[j] = -1;
      bop->solutions[j] = 3;
      bop->mode[j] = 1;
    }

    timer_start();
    int ret;
    if ((ret = SolveAllChunks(bop, solvedbdp, 1))
        != RETURN_NO_FAULT)
    {
      printf("loop_solve i %i: Return %d\n", i, ret);
      exit(0);
    }
    tu = timer_end();

#ifdef BATCHTIMES
    printf("%8d (%5.1f%%) %15d\n",
           i + count,
           100. * (i + count) / static_cast<double>(number),
           tu);
    fflush(stdout);
#endif

    for (int j = 0; j < count; j++)
    {
      if (! compare_FUT(&solvedbdp->solvedBoard[j], &fut_list[i + j]))
      {
        printf("loop_solve i %d, j %d: Difference\n", i, j);
      }
    }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

}


bool loop_calc(
  ddTableDealsPBN * dealsp,
  ddTablesRes * resp,
  allParResults * parp,
  dealPBN * deal_list,
  ddTableResults * table_list,
  int number)
{
#ifdef BATCHTIMES
  printf("%8s %24s\n", "Hand no.", "Time");
#endif

  int filter[5] = {0, 0, 0, 0, 0};

  for (int i = 0; i < number; i += input_number)
  {
    int count = (i + input_number > number ? number - i : input_number);
    dealsp->noOfTables = count;
    for (int j = 0; j < count; j++)
    {
      strcpy(dealsp->deals[j].cards, deal_list[i + j].remainCards);
    }

    timer_start();
    int ret;
    if ((ret = CalcAllTablesPBN(dealsp, -1, filter, resp, parp))
        != RETURN_NO_FAULT)
    {
      printf("loop_solve i %i: Return %d\n", i, ret);
      exit(0);
    }
    tu = timer_end();

#ifdef BATCHTIMES
    printf("%8d (%5.1f%%) %15d\n",
           i + count,
           100. * (i + count) / static_cast<double>(number),
           tu);
    fflush(stdout);
#endif

    for (int j = 0; j < count; j++)
      if (! compare_TABLE(&resp->results[j], &table_list[i + j]))
      {
        printf("loop_calc table i %d, j %d: Difference\n", i, j);
        print_TABLE( &resp->results[j] );
        print_TABLE( &table_list[i + j]) ;
      }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

  return true;
}



bool loop_par(
  int * vul_list,
  ddTableResults * table_list,
  parResults * par_list,
  int number)
{
  /* This is so fast that there is no batch or multi-threaded
     version. We run it many times just to get meaningful times. */

  parResults presp;

  for (int i = 0; i < number; i++)
  {
    for (int j = 0; j < input_number; j++)
    {
      int ret;
      if ((ret = Par(&table_list[i], &presp, vul_list[i]))
          != RETURN_NO_FAULT)
      {
        printf("loop_par i %i, j %d: Return %d\n", i, j, ret);
        exit(0);
      }
    }

    if (! compare_PAR(&presp, &par_list[i]))
    {
      printf("loop_par i %d: Difference\n", i);
    }
  }

  return true;
}


bool loop_dealerpar(
  int * dealer_list,
  int * vul_list,
  ddTableResults * table_list,
  parResultsDealer * dealerpar_list,
  int number)
{
  /* This is so fast that there is no batch or multi-threaded
     version. We run it many times just to get meaningful times. */

  parResultsDealer presp;

  for (int i = 0; i < number; i++)
  {
    for (int j = 0; j < input_number; j++)
    {
      int ret;
      if ((ret = DealerPar(&table_list[i], &presp,
                           dealer_list[i], vul_list[i]))
          != RETURN_NO_FAULT)
      {
        printf("loop_dealerpar i %i, j %d: Return %d\n", i, j, ret);
        exit(0);
      }
    }

    if (! compare_DEALERPAR(&presp, &dealerpar_list[i]))
    {
      printf("loop_dealerpar i %d: Difference\n", i);
    }
  }

  return true;
}


bool loop_play(
  boardsPBN * bop,
  playTracesPBN * playsp,
  solvedPlays * solvedplp,
  dealPBN * deal_list,
  playTracePBN * play_list,
  solvedPlay * trace_list,
  int number)
{
#ifdef BATCHTIMES
  printf("%8s %24s\n", "Hand no.", "Time");
#endif

  for (int i = 0; i < number; i += input_number)
  {
    int count = (i + input_number > number ? number - i : input_number);

    bop->noOfBoards = count;
    playsp->noOfBoards = count;

    for (int j = 0; j < count; j++)
    {
      bop->deals[j] = deal_list[i + j];
      bop->target[j] = 0;
      bop->solutions[j] = 3;
      bop->mode[j] = 1;

      playsp->plays[j] = play_list[i + j];
    }

    timer_start();
    int ret;
    if ((ret = AnalyseAllPlaysPBN(bop, playsp, solvedplp, 1))
        != RETURN_NO_FAULT)
    {
      printf("loop_play i %i: Return %d\n", i, ret);
      exit(0);
    }
    tu = timer_end();

#ifdef BATCHTIMES
    printf("%8d (%5.1f%%) %15d\n",
           i + count,
           100. * (i + count) / static_cast<double>(number),
           tu);
    fflush(stdout);
#endif

    for (int j = 0; j < count; j++)
    {
      if (! compare_TRACE(&solvedplp->solved[j], &trace_list[i + j]))
      {
        printf("loop_play i %d, j %d: Difference\n", i, j);
        // printf("trace_list[%d]: \n", i+j);
        // print_TRACE(&trace_list[i+j]);
        // printf("solvedplp[%d]: \n", j);
        // print_TRACE(&solvedplp->solved[j]);
      }
    }
  }

#ifdef BATCHTIMES
  printf("\n");
#endif

  return true;
}


void print_times(int number)
{
  printf("%-20s %12d\n", "Number of hands", number);
  if (number == 0) return;

  if (ctu == 0)
    printf("%-20s %12s\n", "User time (ms)", "zero");
  else
  {
    printf("%-20s %12d\n", "User time (ms)", ctu);
    printf("%-20s %12.2f\n", "Avg user time (ms)",
           ctu / static_cast<float>(number));
  }

  if (cts == 0)
    printf("%-20s %12s\n", "Syst time", "zero");
  else
  {
    printf("%-20s %12d\n", "Syst time (ms)", cts);
    printf("%-20s %12.2f\n", "Avg syst time (ms)",
           cts / static_cast<float>(number));
    printf("%-20s %12.2f\n", "Ratio",
           cts / static_cast<float>(ctu));
  }
  printf("\n");
}


#ifndef _WIN32
int timeval_diff(timeval x, timeval y)
{
  /* Elapsed time, x-y, in milliseconds */
  return 1000 * (x.tv_sec - y.tv_sec )
         + (x.tv_usec - y.tv_usec) / 1000;
}
#endif


void timer_start()
{
  ts0 = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&tu0);
#else
  gettimeofday(&tu0, nullptr);
#endif
}


int timer_end()
{
  ts1 = clock();

#ifdef _WIN32
  QueryPerformanceCounter(&tu1);
  tu = static_cast<int>
       ((tu1.QuadPart - tu0.QuadPart) * 1000. / frequency.QuadPart);
#else
  gettimeofday(&tu1, nullptr);
  tu = timeval_diff(tu1, tu0);
#endif
  ctu += tu;

  ts = static_cast<int>
       ( (1000 *
          static_cast<long long>(ts1 - ts0) / 
          static_cast<double>(CLOCKS_PER_SEC)));
// TEMPNODE
// printf("%8d", ts);
  cts += ts;
  return tu;
}


bool consume_int(
  char * line,
  int * pos,
  int * res)
{
  /* Too much Perl programming spoils one... No doubt there
     is a good way to do this in C. */

  int len = static_cast<int>(strlen(line));
  int i = * pos;
  int value = 0;
  while (i < len &&
         static_cast<int>(line[i]) >= ZERO &&
         static_cast<int>(line[i]) <= NINE)
  {
    value = 10 * value + static_cast<int>(line[i++]) - ZERO;
  }
  if (static_cast<int>(line[i]) != SPACE)
  {
    printf("Doesn't end on space\n");
    return false;
  }
  *pos = i + 1;
  *res = value;
  return true;
}

bool consume_string(
  char * line,
  int * pos,
  char * res)
{
  int len = static_cast<int>(strlen(line));
  int i = * pos;

  if (static_cast<int>(line[i]) != QUOTE) return false;
  i++;

  while (i < len && static_cast<int>(line[i]) != QUOTE)
    i++;

  if (static_cast<int>(line[i] ) != QUOTE) return false;
  if (static_cast<int>(line[i + 1]) != SPACE) return false;
  i += 2;

  strncpy(res, line + *pos + 1, static_cast<size_t>(i - *pos - 3));
  res[i - *pos - 3] = '\0';
  *pos = i;
  return true;
}


bool consume_tag(
  char * line,
  int * pos,
  const char * tag)
{
  int len = static_cast<int>(strlen(line));
  int i = * pos;
  char read[80] = "";

  while (i < len && static_cast<int>(line[i]) != SPACE)
  {
    i++;
  }
  i++;

  strncpy(read, line + *pos, static_cast<size_t>(i - *pos - 1));
  if (strcmp(read, tag)) return false;

  *pos = i;
  return true;
}


void dump_string(
  const char * line)
{
  int len = static_cast<int>(strlen(line));
  printf("Dumping len %d\n", len);
  for (int i = 0; i <= len; i++)
  {
    printf("%2d %c %d\n", i, line[i], static_cast<int>(line[i]));
  }
}

