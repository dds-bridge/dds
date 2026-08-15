/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <api/dll.h>

namespace dd_table_for_deal {

constexpr std::size_t PBN_FILE_MAX = 16 * 1024 * 1024;
constexpr std::size_t PBN_DEAL_MAX = sizeof(DdTableDealPBN::cards);

auto parse_vulnerable(std::string_view text) -> std::optional<int>;

// Positive deal count, or nullopt if the text is not a valid positive integer.
auto parse_limit(std::string_view text) -> std::optional<std::size_t>;

// Keep the first `limit` deals when set; otherwise return deals unchanged.
auto apply_deal_limit(
    std::vector<std::string> deals,
    std::optional<std::size_t> limit) -> std::vector<std::string>;

auto extract_deal_tags(std::string_view text) -> std::vector<std::string>;

auto unique_deals(std::vector<std::string> const& deals)
    -> std::vector<std::string>;

auto looks_like_path(std::string_view arg) -> bool;

// Read until EOF. Empty input yields an empty string. Oversized input prints an
// error and returns nullopt.
auto read_pbn_stream(std::istream& in) -> std::optional<std::string>;

auto path_is_openable(std::string_view path) -> bool;

// True when a failed stream read should report empty/IO failure to the user.
// False when the stream is still readable (e.g. oversize already reported).
auto should_report_failed_stream_read(std::istream const& in) -> bool;

auto format_par_line(ParResultsMaster const sidesRes[2])
    -> std::optional<std::string>;

}  // namespace dd_table_for_deal
