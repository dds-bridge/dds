/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Calculate all 20 par results for a single deal from the command line or a PBN file.

// Coded by Cursor, based on calc_dd_table.cpp

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <api/dll.h>
#include "hands.hpp"


#define PBN_FILE_MAX 8192


static auto read_pbn_file(const char * path, char * buf, size_t buf_size) -> bool
{
  FILE * f = fopen(path, "rb");
  if (f == nullptr)
  {
    return false;
  }

  const size_t n = fread(buf, 1, buf_size - 1, f);
  fclose(f);

  if (n == 0)
  {
    return false;
  }

  buf[n] = '\0';
  return true;
}


static auto extract_deal_tag(const char * text, char * deal, size_t deal_size) -> bool
{
  const char * p = text;

  while ((p = strstr(p, "[Deal")) != nullptr)
  {
    p += 5;

    while (*p != '\0' && isspace(static_cast<unsigned char>(*p)))
    {
      ++p;
    }

    if (*p != '"')
    {
      continue;
    }

    ++p;
    const char * start = p;

    while (*p != '\0' && *p != '"')
    {
      ++p;
    }

    if (*p != '"')
    {
      continue;
    }

    const size_t len = static_cast<size_t>(p - start);
    if (len >= deal_size)
    {
      return false;
    }

    memcpy(deal, start, len);
    deal[len] = '\0';
    return true;
  }

  return false;
}


static auto load_deal(const char * arg, char * deal, size_t deal_size) -> int
{
  char file_buf[PBN_FILE_MAX];

  if (read_pbn_file(arg, file_buf, sizeof(file_buf)))
  {
    if (!extract_deal_tag(file_buf, deal, deal_size))
    {
      fprintf(stderr, "No [Deal \"...\"] tag found in %s\n", arg);
      return 1;
    }
  }
  else if (strlen(arg) >= deal_size)
  {
    fprintf(stderr,
            "PBN deal too long (max %zu characters)\n",
            deal_size - 1);
    return 1;
  }
  else
  {
    strcpy(deal, arg);
  }

  return 0;
}


auto main(int argc, char * argv[]) -> int
{
  if (argc != 2)
  {
    fprintf(stderr,
            "Usage: %s <pbn_deal_or_file>\n"
            "Example: %s \"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
            "5.A95432.7632.K6 AKJ9842.K.T8.J93\"\n"
            "Example: %s hand.pbn\n",
            argv[0],
            argv[0],
            argv[0]);
    return 1;
  }

  DdTableDealPBN tableDealPBN;
  DdTableResults table;
  char line[80];

  if (load_deal(argv[1], tableDealPBN.cards, sizeof(tableDealPBN.cards)) != 0)
  {
    return 1;
  }

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  const int res = CalcDDtablePBN(tableDealPBN, &table);
  if (res != RETURN_NO_FAULT)
  {
    ErrorMessage(res, line);
    fprintf(stderr, "DDS error: %s\n", line);
    return 1;
  }

  sprintf(line, "calc_dds:\n");
  print_pbn_hand(line, tableDealPBN.cards);
  print_table(&table);

  return 0;
}
