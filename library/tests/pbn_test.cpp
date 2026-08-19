/// @file pbn_test.cpp
/// @brief Unit tests for convert_from_pbn deal-string parsing.

#include <gtest/gtest.h>

#include <string>

#include <api/PBN.h>
#include <api/dll.h>

namespace
{

constexpr char kNorthFirst[] =
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3";

constexpr char kEastFirst[] =
    "E:QJT5432.T.6.QJ82 .J97543.K7532.94 87.A62.QJT4.AT75 AK96.KQ8.A98.K63";

auto convert(const char* pbn) -> int
{
  unsigned int remain[DDS_HANDS][DDS_SUITS]{};
  return convert_from_pbn(pbn, remain);
}

}  // namespace

TEST(ConvertFromPbn, AcceptsNorthFirstWithoutLaterDirections)
{
  unsigned int remain[DDS_HANDS][DDS_SUITS]{};
  EXPECT_EQ(convert_from_pbn(kNorthFirst, remain), RETURN_NO_FAULT);
  EXPECT_NE(remain[0][0], 0u);
}

TEST(ConvertFromPbn, AcceptsEastFirstWithoutLaterDirections)
{
  EXPECT_EQ(convert(kEastFirst), RETURN_NO_FAULT);
}

TEST(ConvertFromPbn, AcceptsLowercaseFirstHandDirection)
{
  EXPECT_EQ(
      convert(
          "n:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"),
      RETURN_NO_FAULT);
}

TEST(ConvertFromPbn, RejectsClockwiseSeatLettersOnLaterHands)
{
  EXPECT_EQ(
      convert(
          "N:QJ6.K652.J85.T98 E:873.J97.AT764.Q4 S:K5.T83.KQ9.A7652 "
          "W:AT942.AQ4.32.KJ3"),
      0);
}

TEST(ConvertFromPbn, RejectsASingleExtraSeatLetter)
{
  EXPECT_EQ(
      convert(
          "N:QJ6.K652.J85.T98 W:873.J97.AT764.Q4 K5.T83.KQ9.A7652 "
          "AT942.AQ4.32.KJ3"),
      0);
}

TEST(ConvertFromPbn, RejectsLowercaseExtraSeatLetter)
{
  EXPECT_EQ(
      convert(
          "N:QJ6.K652.J85.T98 e:873.J97.AT764.Q4 K5.T83.KQ9.A7652 "
          "AT942.AQ4.32.KJ3"),
      0);
}

TEST(ConvertFromPbn, RejectsNullPointerDealBuffer)
{
  unsigned int remain[DDS_HANDS][DDS_SUITS]{};
  EXPECT_EQ(convert_from_pbn(nullptr, remain), 0);
}

TEST(ConvertFromPbn, ClearsOutputOnNullDealBuffer)
{
  unsigned int remain[DDS_HANDS][DDS_SUITS]{};
  remain[0][0] = 0xFFFF;
  EXPECT_EQ(convert_from_pbn(nullptr, remain), 0);
  EXPECT_EQ(remain[0][0], 0u);
}

TEST(ConvertFromPbn, ClearsOutputOnInvalidDeal)
{
  unsigned int remain[DDS_HANDS][DDS_SUITS]{};
  remain[0][0] = 0xFFFF;
  EXPECT_EQ(convert_from_pbn("xx", remain), 0);
  EXPECT_EQ(remain[0][0], 0u);
}

TEST(ConvertFromPbn, RejectsNullOutputBuffer)
{
  EXPECT_EQ(convert_from_pbn(kNorthFirst, nullptr), 0);
}

TEST(ConvertFromPbn, RejectsEmptyAndMissingSeatPrefixInputs)
{
  EXPECT_EQ(convert(""), 0);
  EXPECT_EQ(convert("N"), 0);
  EXPECT_EQ(convert("xx"), 0);
}

TEST(ConvertFromPbn, RejectsTooManySuitsInHand)
{
  EXPECT_EQ(convert("N:AK.K.K.K.A"), 0);
}

TEST(ConvertFromPbn, RejectsTooManyHands)
{
  unsigned int remain[DDS_HANDS][DDS_SUITS]{};
  EXPECT_EQ(convert_from_pbn("N:AK.K.K.K    A", remain), 0);
}

TEST(ConvertFromPbn, RejectsTruncatedDealWithFewerThanFourHands)
{
  EXPECT_EQ(convert("N:AK.QJ.T9.876"), 0);         // 1 hand
  EXPECT_EQ(convert("N:AK.QJ.T9.876 AK.QJ.T9.876"), 0); // 2 hands
  EXPECT_EQ(convert("N:AK.QJ.T9.876 AK.QJ.T9.876 AK.QJ.T9.876"), 0); // 3 hands
}

TEST(ConvertFromPbn, RejectsInputLongerThanRemainCardsBuffer)
{
  constexpr auto kBufSize = sizeof(DealPBN::remainCards);
  std::string pbn = "N:";
  pbn.append(kBufSize, 'A');
  ASSERT_GT(pbn.size(), kBufSize);
  EXPECT_EQ(convert(pbn.c_str()), 0);
}

TEST(ConvertFromPbn, RejectsInputExactlyAtRemainCardsBufferLimit)
{
  constexpr auto kBufSize = sizeof(DealPBN::remainCards);
  std::string pbn = "N:";
  pbn.append(kBufSize - 2, 'A');
  ASSERT_EQ(pbn.size(), kBufSize);
  EXPECT_EQ(convert(pbn.c_str()), 0);
}

TEST(ConvertFromPbn, AcceptsInputThatFitsRemainCardsBuffer)
{
  constexpr auto kBufSize = sizeof(DealPBN::remainCards);
  ASSERT_LT(std::char_traits<char>::length(kNorthFirst), kBufSize);
  EXPECT_EQ(convert(kNorthFirst), RETURN_NO_FAULT);
}
