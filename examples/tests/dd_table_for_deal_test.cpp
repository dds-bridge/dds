/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <gtest/gtest.h>

#include <sstream>

#include <api/dll.h>
#include "dd_table_for_deal.hpp"

namespace {

auto make_contract(
    int seats,
    int level,
    int denom,
    int under_tricks,
    int over_tricks) -> ContractType
{
  ContractType c{};
  c.seats = seats;
  c.level = level;
  c.denom = denom;
  c.under_tricks = under_tricks;
  c.over_tricks = over_tricks;
  return c;
}

}  // namespace


TEST(ParseVulnerable, AcceptsAliasesAndCodes)
{
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("none"), 0);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("None"), 0);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("0"), 0);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("both"), 1);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("1"), 1);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("ns"), 2);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("NS"), 2);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("2"), 2);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("ew"), 3);
  EXPECT_EQ(dd_table_for_deal::parse_vulnerable("3"), 3);
}


TEST(ParseVulnerable, RejectsUnknown)
{
  EXPECT_FALSE(dd_table_for_deal::parse_vulnerable("").has_value());
  EXPECT_FALSE(dd_table_for_deal::parse_vulnerable("maybe").has_value());
  EXPECT_FALSE(dd_table_for_deal::parse_vulnerable("4").has_value());
}


TEST(ParseLimit, AcceptsPositiveIntegers)
{
  EXPECT_EQ(dd_table_for_deal::parse_limit("1"), 1u);
  EXPECT_EQ(dd_table_for_deal::parse_limit("25"), 25u);
}


TEST(ParseLimit, RejectsNonPositiveAndNonNumeric)
{
  EXPECT_FALSE(dd_table_for_deal::parse_limit("").has_value());
  EXPECT_FALSE(dd_table_for_deal::parse_limit("0").has_value());
  EXPECT_FALSE(dd_table_for_deal::parse_limit("-1").has_value());
  EXPECT_FALSE(dd_table_for_deal::parse_limit("3x").has_value());
  EXPECT_FALSE(dd_table_for_deal::parse_limit("1.5").has_value());
}


TEST(ApplyDealLimit, KeepsPrefixWhenLimited)
{
  const std::vector<std::string> deals{"a", "b", "c"};
  EXPECT_EQ(
      dd_table_for_deal::apply_deal_limit(deals, 2),
      (std::vector<std::string>{"a", "b"}));
  EXPECT_EQ(
      dd_table_for_deal::apply_deal_limit(deals, std::nullopt),
      deals);
  EXPECT_EQ(
      dd_table_for_deal::apply_deal_limit(deals, 10),
      deals);
}


TEST(ShouldReportFailedStreamRead, TrueOnEofOrBad)
{
  std::istringstream empty("");
  empty.get();
  EXPECT_TRUE(empty.eof());
  EXPECT_TRUE(dd_table_for_deal::should_report_failed_stream_read(empty));

  std::istringstream bad_stream("x");
  bad_stream.setstate(std::ios::badbit);
  EXPECT_TRUE(dd_table_for_deal::should_report_failed_stream_read(bad_stream));
}


TEST(ShouldReportFailedStreamRead, FalseWhenStreamStillReadable)
{
  // Oversized reads stop early without setting eof/bad; do not re-report.
  std::istringstream mid("still-readable");
  EXPECT_FALSE(dd_table_for_deal::should_report_failed_stream_read(mid));
}


TEST(ExtractDealTags, FindsAllTags)
{
  const char* text =
      "{Board 1}\n"
      "[Deal \"N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
      "5.A95432.7632.K6 AKJ9842.K.T8.J93\"]\n"
      "\n"
      "{Board 2}\n"
      "[Deal \"N:QJ6.K652.J85.T98 873.J97.AT764.Q4 "
      "K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3\"]\n";

  const auto deals = dd_table_for_deal::extract_deal_tags(text);
  ASSERT_EQ(deals.size(), 2u);
  EXPECT_EQ(
      deals[0],
      "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
      "5.A95432.7632.K6 AKJ9842.K.T8.J93");
  EXPECT_EQ(
      deals[1],
      "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 "
      "K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3");
}


TEST(ExtractDealTags, EmptyWhenNoTags)
{
  EXPECT_TRUE(dd_table_for_deal::extract_deal_tags("{comment only}").empty());
}


TEST(UniqueDeals, PreservesFirstSeenOrderAndDropsDuplicates)
{
  const std::vector<std::string> deals = {
      "deal-a",
      "deal-b",
      "deal-a",
      "deal-c",
      "deal-b",
      "deal-a",
  };
  const auto unique = dd_table_for_deal::unique_deals(deals);
  ASSERT_EQ(unique.size(), 3u);
  EXPECT_EQ(unique[0], "deal-a");
  EXPECT_EQ(unique[1], "deal-b");
  EXPECT_EQ(unique[2], "deal-c");
}


TEST(LooksLikePath, DetectsPathsAndExtensions)
{
  EXPECT_TRUE(dd_table_for_deal::looks_like_path("boards.pbn"));
  EXPECT_TRUE(dd_table_for_deal::looks_like_path("hands/x.pbn"));
  EXPECT_TRUE(dd_table_for_deal::looks_like_path("notes.txt"));
  EXPECT_FALSE(dd_table_for_deal::looks_like_path(
      "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 "
      "5.A95432.7632.K6 AKJ9842.K.T8.J93"));
}


TEST(FormatParLine, SingleSacrifice)
{
  ParResultsMaster sides[2]{};
  sides[0].score = -300;
  sides[0].number = 1;
  sides[0].contracts[0] = make_contract(/*NS*/ 4, 5, /*H*/ 2, 2, 0);
  sides[1].score = 300;
  sides[1].number = 1;
  sides[1].contracts[0] = make_contract(/*NS*/ 4, 5, /*H*/ 2, 2, 0);

  const auto line = dd_table_for_deal::format_par_line(sides);
  ASSERT_TRUE(line.has_value());
  EXPECT_EQ(*line, "Par: NS 5Hx -2 -300");
}


TEST(FormatParLine, SingleMakingUsesEqualsAndDeclaringScore)
{
  ParResultsMaster sides[2]{};
  sides[0].score = -110;
  sides[0].number = 1;
  sides[0].contracts[0] = make_contract(/*EW*/ 5, 2, /*S*/ 1, 0, 0);
  sides[1].score = 110;
  sides[1].number = 1;
  sides[1].contracts[0] = make_contract(/*EW*/ 5, 2, /*S*/ 1, 0, 0);

  const auto line = dd_table_for_deal::format_par_line(sides);
  ASSERT_TRUE(line.has_value());
  EXPECT_EQ(*line, "Par: EW 2S = 110");
}


TEST(FormatParLine, MultipleSacrificesOnOneLine)
{
  ParResultsMaster sides[2]{};
  sides[0].score = 100;
  sides[0].number = 2;
  sides[0].contracts[0] = make_contract(/*EW*/ 5, 3, /*D*/ 3, 1, 0);
  sides[0].contracts[1] = make_contract(/*EW*/ 5, 3, /*C*/ 4, 1, 0);
  sides[1].score = -100;
  sides[1].number = 2;
  sides[1].contracts[0] = make_contract(/*EW*/ 5, 3, /*D*/ 3, 1, 0);
  sides[1].contracts[1] = make_contract(/*EW*/ 5, 3, /*C*/ 4, 1, 0);

  const auto line = dd_table_for_deal::format_par_line(sides);
  ASSERT_TRUE(line.has_value());
  EXPECT_EQ(*line, "Par: EW 3Dx, 3Cx -1 -100");
}


TEST(FormatParLine, OmitsRepeatedDeclaringSideWhenSeatsDiffer)
{
  ParResultsMaster sides[2]{};
  sides[0].score = 100;
  sides[0].number = 2;
  sides[0].contracts[0] = make_contract(/*EW*/ 5, 4, /*H*/ 2, 1, 0);
  sides[0].contracts[1] = make_contract(/*E*/ 1, 5, /*C*/ 4, 1, 0);
  sides[1].score = -100;
  sides[1].number = 2;
  sides[1].contracts[0] = make_contract(/*EW*/ 5, 4, /*H*/ 2, 1, 0);
  sides[1].contracts[1] = make_contract(/*E*/ 1, 5, /*C*/ 4, 1, 0);

  const auto line = dd_table_for_deal::format_par_line(sides);
  ASSERT_TRUE(line.has_value());
  EXPECT_EQ(*line, "Par: EW 4Hx, 5Cx -1 -100");
}


TEST(FormatParLine, PassedOut)
{
  ParResultsMaster sides[2]{};
  sides[0].score = 0;
  sides[0].number = 1;
  sides[1].score = 0;
  sides[1].number = 1;

  const auto line = dd_table_for_deal::format_par_line(sides);
  ASSERT_TRUE(line.has_value());
  EXPECT_EQ(*line, "Par: 0");
}
