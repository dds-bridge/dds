/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "dd_table_for_deal.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <limits>
#include <regex>
#include <string>
#include <unordered_set>

namespace dd_table_for_deal {
namespace {

const std::regex DEAL_TAG_RE{
    R"re(\[Deal\s*"([^"]*)")re",
    std::regex::icase};


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


auto format_contract(
    const ContractType& contract,
    bool include_seats) -> std::optional<std::string>
{
  char out[16];
  const char doubled = contract.under_tricks > 0 ? 'x' : '\0';
  if (include_seats)
  {
    if (doubled)
    {
      std::snprintf(
          out,
          sizeof(out),
          "%s %d%cx",
          seat_name(contract.seats),
          contract.level,
          denom_char(contract.denom));
    }
    else
    {
      std::snprintf(
          out,
          sizeof(out),
          "%s %d%c",
          seat_name(contract.seats),
          contract.level,
          denom_char(contract.denom));
    }
  }
  else if (doubled)
  {
    std::snprintf(
        out,
        sizeof(out),
        "%d%cx",
        contract.level,
        denom_char(contract.denom));
  }
  else
  {
    std::snprintf(
        out,
        sizeof(out),
        "%d%c",
        contract.level,
        denom_char(contract.denom));
  }
  return std::string(out);
}

}  // namespace


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


auto parse_limit(std::string_view text) -> std::optional<std::size_t>
{
  if (text.empty())
    return std::nullopt;

  std::size_t value = 0;
  for (char ch : text)
  {
    if (ch < '0' || ch > '9')
      return std::nullopt;
    const std::size_t digit = static_cast<std::size_t>(ch - '0');
    if (value > (std::numeric_limits<std::size_t>::max() - digit) / 10)
      return std::nullopt;
    value = value * 10 + digit;
  }
  if (value == 0)
    return std::nullopt;
  return value;
}


auto apply_deal_limit(
    std::vector<std::string> deals,
    std::optional<std::size_t> limit) -> std::vector<std::string>
{
  if (!limit.has_value() || *limit >= deals.size())
    return deals;
  deals.resize(*limit);
  return deals;
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


auto unique_deals(std::vector<std::string> const& deals)
    -> std::vector<std::string>
{
  std::vector<std::string> unique;
  std::unordered_set<std::string> seen;
  unique.reserve(deals.size());
  for (const auto& deal : deals)
  {
    if (seen.insert(deal).second)
      unique.push_back(deal);
  }
  return unique;
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
  return text;
}


auto path_is_openable(std::string_view path) -> bool
{
  std::ifstream file(std::filesystem::path(path), std::ios::binary);
  return static_cast<bool>(file);
}


auto should_report_failed_stream_read(std::istream const& in) -> bool
{
  return in.eof() || in.bad();
}


auto format_par_line(ParResultsMaster const sidesRes[2])
    -> std::optional<std::string>
{
  if (sidesRes[0].score == 0 && sidesRes[1].score == 0)
    return std::string("Par: 0");

  if (sidesRes[0].number <= 0 && sidesRes[1].number <= 0)
    return std::nullopt;

  const ContractType& first = sidesRes[0].number > 0
      ? sidesRes[0].contracts[0]
      : sidesRes[1].contracts[0];
  const int side =
      (first.seats == 4 || first.seats == 0 || first.seats == 2) ? 0 : 1;
  const ParResultsMaster& chosen = sidesRes[side];
  if (chosen.number <= 0)
    return std::nullopt;

  std::string body;
  for (int i = 0; i < chosen.number; ++i)
  {
    const auto piece = format_contract(chosen.contracts[i], /*include_seats=*/i == 0);
    if (!piece)
      return std::nullopt;
    if (i > 0)
      body += ", ";
    body += *piece;
  }

  const ContractType& contract = chosen.contracts[0];
  char result[8];
  if (contract.under_tricks > 0)
    std::snprintf(result, sizeof(result), "-%d", contract.under_tricks);
  else if (contract.over_tricks > 0)
    std::snprintf(result, sizeof(result), "+%d", contract.over_tricks);
  else
    std::snprintf(result, sizeof(result), "=");

  char line[256];
  const int written = std::snprintf(
      line,
      sizeof(line),
      "Par: %s %s %d",
      body.c_str(),
      result,
      chosen.score);
  if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(line))
    return std::nullopt;
  return std::string(line);
}

}  // namespace dd_table_for_deal
