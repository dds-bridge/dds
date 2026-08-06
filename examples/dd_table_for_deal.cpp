/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


// Print the double-dummy table and par for a deal from the command line or a PBN file.

// Coded by Cursor, based on calc_dd_table.cpp

#include <algorithm>
#include <cctype>
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
#include <vector>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <api/dll.h>
#include "hands.hpp"


namespace {

constexpr std::size_t PBN_FILE_MAX = 16 * 1024 * 1024;
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
  std::string text;
  text.reserve(64 * 1024);
  char buffer[4096];
  while (in)
  {
    in.read(buffer, static_cast<std::streamsize>(sizeof(buffer)));
    const auto n = in.gcount();
    if (n > 0)
      text.append(buffer, static_cast<std::size_t>(n));
    if (text.size() > PBN_FILE_MAX)
    {
      std::cerr << "PBN input too large (max " << PBN_FILE_MAX << " characters)\n";
      return std::nullopt;
    }
  }
  if (text.empty())
    return std::nullopt;
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

  // bazelisk run uses a runfiles cwd; BUILD_WORKSPACE_DIRECTORY is the repo root.
  if (const char* workspace = std::getenv("BUILD_WORKSPACE_DIRECTORY"))
  {
    return read_pbn_file(std::filesystem::path(workspace) / path);
  }

  return std::nullopt;
}


auto extract_deal_tags(std::string_view text) -> std::vector<std::string>
{
  std::vector<std::string> deals;
  auto begin = text.cbegin();
  const auto end = text.cend();
  std::match_results<std::string_view::const_iterator> match;
  while (std::regex_search(begin, end, match, DEAL_TAG_RE) && match.size() > 1)
  {
    deals.emplace_back(match[1].first, match[1].second);
    begin = match[0].second;
  }
  return deals;
}


auto parse_vulnerable(std::string_view text) -> std::optional<int>
{
  std::string lower(text);
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

  if (lower == "none" || lower == "0")
    return 0;
  if (lower == "both" || lower == "1")
    return 1;
  if (lower == "ns" || lower == "2")
    return 2;
  if (lower == "ew" || lower == "3")
    return 3;
  return std::nullopt;
}


auto looks_like_path(std::string_view arg) -> bool
{
  if (arg.find('/') != std::string_view::npos
      || arg.find('\\') != std::string_view::npos)
  {
    return true;
  }

  if (arg.size() >= 4)
  {
    std::string lower(arg.substr(arg.size() - 4));
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) {
                     return static_cast<char>(std::tolower(c));
                   });
    if (lower == ".pbn" || lower == ".txt")
      return true;
  }
  return false;
}


auto load_deals(std::string_view arg) -> std::optional<std::vector<std::string>>
{
  if (arg == "-")
  {
    const auto text = read_pbn_stream(std::cin);
    if (!text)
    {
      std::cerr << "No PBN input on stdin\n";
      return std::nullopt;
    }

    const auto deals = extract_deal_tags(*text);
    if (deals.empty())
    {
      std::cerr << "No [Deal \"...\"] tag found in stdin\n";
      return std::nullopt;
    }

    return deals;
  }

  if (const auto text = read_pbn_file_workspace_relative(arg))
  {
    const auto deals = extract_deal_tags(*text);
    if (deals.empty())
    {
      std::cerr << "No [Deal \"...\"] tag found in " << arg << "\n";
      return std::nullopt;
    }

    return deals;
  }

  if (looks_like_path(arg))
  {
    std::cerr << "Cannot read file: " << arg << "\n";
    return std::nullopt;
  }

  if (arg.size() >= PBN_DEAL_MAX)
  {
    std::cerr << "PBN deal too long (max " << (PBN_DEAL_MAX - 1)
              << " characters)\n";
    return std::nullopt;
  }

  return std::vector<std::string>{std::string(arg)};
}


auto denom_char(int denom) -> char
{
  static constexpr char kDenomChars[] = "NSHDC";
  if (denom < 0 || denom > 4)
    return '?';
  return kDenomChars[denom];
}


auto seat_name(int seats) -> const char *
{
  static const char * kSeatNames[] = {"N", "E", "S", "W", "NS", "EW"};
  if (seats < 0 || seats > 5)
    return "?";
  return kSeatNames[seats];
}


auto format_par_line(
    ParResultsMaster const sidesRes[2],
    int vulnerable,
    char * line,
    std::size_t line_size) -> bool
{
  if (line_size < 32)
    return false;

  if (sidesRes[0].score == 0 && sidesRes[1].score == 0)
  {
    std::snprintf(line, line_size, "Par: 0");
    return true;
  }

  if (sidesRes[0].number != 1 || sidesRes[1].number != 1)
    return false;

  const ContractType& contract = sidesRes[0].contracts[0];
  const char * seats = seat_name(contract.seats);
  const char result_sign = contract.under_tricks > 0 ? '-' : '+';
  const int result_value =
      contract.under_tricks > 0 ? contract.under_tricks : contract.over_tricks;

  if (contract.under_tricks > 0)
  {
    std::snprintf(
        line,
        line_size,
        "Par: %s %d%c%c %c%d %d",
        seats,
        contract.level,
        denom_char(contract.denom),
        'x',
        result_sign,
        result_value,
        sidesRes[0].score);
  }
  else
  {
    std::snprintf(
        line,
        line_size,
        "Par: %s %d%c %c%d %d",
        seats,
        contract.level,
        denom_char(contract.denom),
        result_sign,
        result_value,
        sidesRes[0].score);
  }

  return true;
}


auto print_par_or_verbose(
    DdTableResults const * table,
    int vulnerable) -> void
{
  ParResultsMaster sidesRes[2];
  const int res = SidesParBin(table, sidesRes, vulnerable);
  if (res != RETURN_NO_FAULT)
  {
    char line[80];
    ErrorMessage(res, line);
    fprintf(stderr, "DDS error: %s\n", line);
    return;
  }

  char line[80];
  if (format_par_line(sidesRes, vulnerable, line, sizeof(line)))
  {
    printf("%s\n", line);
    return;
  }

  ParResults par;
  const int par_res = Par(table, &par, vulnerable);
  if (par_res != RETURN_NO_FAULT)
  {
    ErrorMessage(par_res, line);
    fprintf(stderr, "DDS error: %s\n", line);
    return;
  }

  print_par(&par);
}


auto process_deal(
    std::string const& deal,
    std::size_t deal_no,
    std::size_t deal_count,
    int vulnerable) -> bool
{
  DdTableDealPBN tableDealPBN{};
  if (deal.size() >= sizeof(tableDealPBN.cards))
  {
    fprintf(stderr,
            "PBN deal too long (max %zu characters)\n",
            sizeof(tableDealPBN.cards) - 1);
    return false;
  }

  std::copy_n(deal.begin(), deal.size(), tableDealPBN.cards);
  tableDealPBN.cards[deal.size()] = '\0';

  DdTableResults table;
  char line[80];

  const int res = CalcDDtablePBN(tableDealPBN, &table);
  if (res != RETURN_NO_FAULT)
  {
    ErrorMessage(res, line);
    fprintf(stderr, "DDS error: %s\n", line);
    return false;
  }

  if (deal_count == 1)
    sprintf(line, "dd_table_for_deal:\n");
  else
    sprintf(line, "Deal %zu:\n", deal_no);

  print_pbn_hand(line, tableDealPBN.cards);
  print_table(&table);
  print_par_or_verbose(&table, vulnerable);
  if (deal_count > 1)
    printf("\n");
  return true;
}

}  // namespace


static auto print_usage(const char * prog) -> void
{
  fprintf(stderr,
          "Usage: %s [--vul none|both|ns|ew] <pbn_deal_or_file>\n"
          "       %s -h | --help\n"
          "\n"
          "Calculate double-dummy tricks and par for all strains and leads.\n"
          "\n"
          "Arguments:\n"
          "  <pbn_deal_or_file>  DDS PBN deal string, or path to a .pbn file\n"
          "  --vul              Vulnerability: none, both, ns, ew (default: none)\n"
          "\n"
          "If stdin is not a terminal, PBN is read from stdin (all [Deal \"...\"] tags).\n"
          "\n"
          "Examples:\n"
          "  %s \"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
          "5.A95432.7632.K6 AKJ9842.K.T8.J93\"\n"
          "  %s --vul ns hands/example.pbn\n"
          "  %s < hands/example.pbn\n",
          prog,
          prog,
          prog,
          prog,
          prog);
}


auto main(int argc, char * argv[]) -> int
{
  const char * input = nullptr;
  int vulnerable = 0;

  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
    {
      print_usage(argv[0]);
      return 0;
    }
    if (strcmp(argv[i], "--vul") == 0)
    {
      if (i + 1 >= argc)
      {
        fprintf(stderr, "--vul requires a value (none|both|ns|ew or 0|1|2|3)\n");
        print_usage(argv[0]);
        return 1;
      }
      const auto vul = parse_vulnerable(argv[++i]);
      if (!vul)
      {
        fprintf(stderr, "Invalid --vul value (use none|both|ns|ew or 0|1|2|3)\n");
        print_usage(argv[0]);
        return 1;
      }
      vulnerable = *vul;
      continue;
    }
    if (argv[i][0] == '-' && strcmp(argv[i], "-") != 0)
    {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
    if (input != nullptr)
    {
      print_usage(argv[0]);
      return 1;
    }
    input = argv[i];
  }

  if (input == nullptr)
  {
    if (!stdin_is_tty())
      input = "-";
    else
    {
      print_usage(argv[0]);
      return 1;
    }
  }

  const auto deals = load_deals(input);
  if (!deals)
  {
    return 1;
  }

  for (std::size_t i = 0; i < deals->size(); ++i)
  {
    if (!process_deal((*deals)[i], i + 1, deals->size(), vulnerable))
      return 1;
  }

  return 0;
}
