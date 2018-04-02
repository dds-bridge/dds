/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/dll.h"
#include "parse.h"

using namespace std;


bool parse_NUMBER(
  char * line,
  int * number);

bool parse_PBN(
  char * line,
  int * dealer,
  int * vul,
  struct dealPBN * dl);

bool parse_FUT(
  char * line,
  struct futureTricks * fut);

bool parse_TABLE(
  char * line,
  struct ddTableResults * table);

bool parse_PAR(
  char * line,
  struct parResults * par);

bool parse_DEALERPAR(
  char * line,
  struct parResultsDealer * par);

bool parse_PLAY(
  char * line,
  struct playTracePBN * playp);

bool parse_TRACE(
  char * line,
  struct solvedPlay * solvedp);

bool parseable_GIB(
  char line[]);

bool parse_GIB(
  char line[],
  dealPBN * dl,
  ddTableResults * table);

bool consume_int(
  char * line,
  int * pos,
  int * res);

bool consume_string(
  char * line,
  int * pos,
  char * res);

bool consume_tag(
  char * line,
  int * pos,
  const char * tag);


// #define DEBUG
#define ZERO (static_cast<int>('0'))
#define NINE (static_cast<int>('9'))
#define SPACE (static_cast<int>(' '))
#define QUOTE (static_cast<int>('"'))


bool read_file(
  char const * fname,
  int * number,
  int ** dealer_list,
  int ** vul_list,
  dealPBN ** deal_list,
  futureTricks ** fut_list,
  ddTableResults ** table_list,
  parResults ** par_list,
  parResultsDealer ** dealerpar_list,
  playTracePBN ** play_list,
  solvedPlay ** trace_list,
  bool& GIBmode)
{
  char line[256];

  FILE * fp;
  fp = fopen(fname, "r");
  if (fp == NULL)
  {
    printf("fp %s is NULL\n", fname);
    return false;
  }

  if (! fgets(line, sizeof(line), fp))
  {
    printf("First line bad\n");
    return false;
  }

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
      (calloc(number_t, sizeof(int)))) == NULL)
    return false;

  if ((*vul_list = static_cast<int *>
      (calloc(number_t, sizeof(int)))) == NULL)
    return false;

  if ((*deal_list = static_cast<dealPBN *>
      (calloc(number_t, sizeof(dealPBN)))) == NULL)
    return false;

  if ((*fut_list = static_cast<futureTricks *>
      (calloc(number_t, sizeof(futureTricks)))) == NULL)
    return false;

  if ((*table_list = static_cast<ddTableResults *>
      (calloc(number_t, sizeof(ddTableResults)))) == NULL)
    return false;

  if ((*par_list = static_cast<parResults *>
      (calloc(number_t, sizeof(parResults)))) == NULL)
    return false;

  if ((*dealerpar_list = static_cast<parResultsDealer *>
      (calloc(number_t, sizeof(parResultsDealer)))) == NULL)
    return false;

  if ((*play_list = static_cast<playTracePBN *>
      (calloc(number_t, sizeof(playTracePBN)))) == NULL)
    return false;

  if ((*trace_list = static_cast<solvedPlay *>
      (calloc(number_t, sizeof(solvedPlay)))) == NULL)
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

