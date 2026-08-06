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

auto extract_deal_tags(std::string_view text) -> std::vector<std::string>;

auto unique_deals(std::vector<std::string> const& deals)
    -> std::vector<std::string>;

auto looks_like_path(std::string_view arg) -> bool;

// True when a failed stream read should report empty/IO failure to the user.
// False when the stream is still readable (e.g. oversize already reported).
auto should_report_failed_stream_read(std::istream const& in) -> bool;

auto format_par_line(ParResultsMaster const sidesRes[2])
    -> std::optional<std::string>;

}  // namespace dd_table_for_deal
