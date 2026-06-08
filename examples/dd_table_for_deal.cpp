/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Print the double-dummy table for a deal from the command line or a PBN file.

// Coded by Cursor, based on calc_dd_table.cpp

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <api/dll.h>
#include "hands.hpp"


namespace {

constexpr std::size_t PBN_FILE_MAX = 8192;
constexpr std::size_t PBN_DEAL_MAX = sizeof(DdTableDealPBN::cards);

const std::regex DEAL_TAG_RE{
    R"re(\[Deal\s*"([^"]*)")re",
    std::regex::icase};


static auto stdin_is_tty() -> bool
{
#if defined(_WIN32)
  return _isatty(_fileno(stdin)) != 0;
#else
  return isatty(STDIN_FILENO) != 0;
#endif
}


auto read_pbn_stream(std::istream& in) -> std::optional<std::string>
{
  std::string text(PBN_FILE_MAX - 1, '\0');
  in.read(text.data(), static_cast<std::streamsize>(PBN_FILE_MAX - 1));
  const auto n = in.gcount();
  if (n <= 0)
  {
    return std::nullopt;
  }

  text.resize(static_cast<std::size_t>(n));
  return text;
}


auto read_pbn_file(const std::filesystem::path& path) -> std::optional<std::string>
{
  std::ifstream file(path, std::ios::binary);
  if (!file)
  {
    return std::nullopt;
  }

  return read_pbn_stream(file);
}


auto read_pbn_file_workspace_relative(std::string_view path)
    -> std::optional<std::string>
{
  if (auto text = read_pbn_file(std::filesystem::path(path)))
  {
    return text;
  }

  // bazel run uses a runfiles cwd; BUILD_WORKSPACE_DIRECTORY is the repo root.
  if (const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY"))
  {
    return read_pbn_file(std::filesystem::path(workspace) / path);
  }

  return std::nullopt;
}


auto extract_deal_tag(std::string_view text) -> std::optional<std::string>
{
  std::match_results<std::string_view::const_iterator> match;
  if (std::regex_search(text.cbegin(), text.cend(), match, DEAL_TAG_RE)
      && match.size() > 1)
  {
    return std::string(match[1].first, match[1].second);
  }

  return std::nullopt;
}


auto load_deal(std::string_view arg) -> std::optional<std::string>
{
  if (arg == "-")
  {
    const auto text = read_pbn_stream(std::cin);
    if (!text)
    {
      std::cerr << "No PBN input on stdin\n";
      return std::nullopt;
    }

    const auto deal = extract_deal_tag(*text);
    if (!deal)
    {
      std::cerr << "No [Deal \"...\"] tag found in stdin\n";
      return std::nullopt;
    }

    return deal;
  }

  if (const auto text = read_pbn_file_workspace_relative(arg))
  {
    const auto deal = extract_deal_tag(*text);
    if (!deal)
    {
      std::cerr << "No [Deal \"...\"] tag found in " << arg << "\n";
      return std::nullopt;
    }

    return deal;
  }

  if (arg.size() >= PBN_DEAL_MAX)
  {
    std::cerr << "PBN deal too long (max " << (PBN_DEAL_MAX - 1)
              << " characters)\n";
    return std::nullopt;
  }

  return std::string(arg);
}

}  // namespace


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

  const auto deal = load_deal(input);
  if (!deal)
  {
    return 1;
  }

  DdTableDealPBN tableDealPBN{};
  if (deal->size() >= sizeof(tableDealPBN.cards))
  {
    fprintf(stderr,
            "PBN deal too long (max %zu characters)\n",
            sizeof(tableDealPBN.cards) - 1);
    return 1;
  }

  std::copy_n(deal->begin(), deal->size(), tableDealPBN.cards);
  tableDealPBN.cards[deal->size()] = '\0';

  DdTableResults table;
  char line[80];

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
