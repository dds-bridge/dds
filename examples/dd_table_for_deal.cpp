/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Print the double-dummy table for a deal from the command line or a PBN file.

// Coded by Cursor, based on calc_dd_table.cpp

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <api/dll.h>
#include "hands.hpp"


#define PBN_FILE_MAX 8192


static auto stdin_is_tty() -> bool
{
#if defined(_WIN32)
  return _isatty(_fileno(stdin)) != 0;
#else
  return isatty(STDIN_FILENO) != 0;
#endif
}


static auto read_pbn_stream(FILE * stream, char * buf, size_t buf_size) -> bool
{
  const size_t n = fread(buf, 1, buf_size - 1, stream);
  if (n == 0)
  {
    return false;
  }

  buf[n] = '\0';
  return true;
}


static auto read_pbn_file(const char * path, char * buf, size_t buf_size) -> bool
{
  FILE * f = fopen(path, "rb");
  if (f == nullptr)
  {
    return false;
  }

  const bool ok = read_pbn_stream(f, buf, buf_size);
  fclose(f);
  return ok;
}


static auto read_pbn_file_workspace_relative(
  const char * path,
  char * buf,
  size_t buf_size) -> bool
{
  if (read_pbn_file(path, buf, buf_size))
  {
    return true;
  }

  // bazel run uses a runfiles cwd; BUILD_WORKSPACE_DIRECTORY is the repo root.
  const char * workspace = getenv("BUILD_WORKSPACE_DIRECTORY");
  if (workspace == nullptr)
  {
    return false;
  }

  char combined[PBN_FILE_MAX];
  const int n = snprintf(
      combined,
      sizeof(combined),
      "%s/%s",
      workspace,
      path);
  if (n < 0 || static_cast<size_t>(n) >= sizeof(combined))
  {
    return false;
  }

  return read_pbn_file(combined, buf, buf_size);
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
  const char * source = arg;

  if (strcmp(arg, "-") == 0)
  {
    source = "stdin";
    if (!read_pbn_stream(stdin, file_buf, sizeof(file_buf)))
    {
      fprintf(stderr, "No PBN input on stdin\n");
      return 1;
    }
  }
  else if (read_pbn_file_workspace_relative(arg, file_buf, sizeof(file_buf)))
  {
    source = arg;
  }
  else
  {
    source = nullptr;
  }

  if (source != nullptr)
  {
    if (!extract_deal_tag(file_buf, deal, deal_size))
    {
      fprintf(stderr, "No [Deal \"...\"] tag found in %s\n", source);
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


static auto print_usage(const char * prog) -> void
{
  fprintf(stderr,
          "Usage: %s <pbn_deal_or_file>\n"
          "       %s -h | --help\n"
          "\n"
          "Calculate double-dummy tricks for all strains and leads.\n"
          "\n"
          "Arguments:\n"
          "  <pbn_deal_or_file>  DDS PBN deal string, or path to a .pbn file\n"
          "\n"
          "If stdin is not a terminal, PBN is read from stdin (uses [Deal \"...\"]).\n"
          "\n"
          "Examples:\n"
          "  %s \"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
          "5.A95432.7632.K6 AKJ9842.K.T8.J93\"\n"
          "  %s hands/example.pbn\n"
          "  %s < hands/example.pbn\n",
          prog,
          prog,
          prog,
          prog,
          prog);
}


auto main(int argc, char * argv[]) -> int
{
  const char * input;

  if (argc == 2)
  {
    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
      print_usage(argv[0]);
      return 0;
    }
    input = argv[1];
  }
  else if (argc == 1 && !stdin_is_tty())
  {
    input = "-";
  }
  else
  {
    print_usage(argv[0]);
    return 1;
  }

  DdTableDealPBN tableDealPBN;
  DdTableResults table;
  char line[80];

  if (load_deal(input, tableDealPBN.cards, sizeof(tableDealPBN.cards)) != 0)
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

  sprintf(line, "dd_table_for_deal:\n");
  print_pbn_hand(line, tableDealPBN.cards);
  print_table(&table);

  return 0;
}
