/// @file next_suit_test.cpp
/// @brief Unit tests for QuickTricks suit-iteration advancement.
///
/// Pins the trump-first / skip-trump ordering used throughout QuickTricks
/// so the shared helper can replace the many copy-pasted advance blocks.

#include <gtest/gtest.h>

#include <utility/constants.h>
#include <quick_tricks.hpp>

namespace {

/// Reference copy of the duplicated QuickTricks advance logic.
auto reference_next_suit(int suit, int trump) -> int
{
  if ((trump != DDS_NOTRUMP) && (suit == trump))
  {
    if (trump == 0)
      return 1;
    return 0;
  }
  suit++;
  if ((trump != DDS_NOTRUMP) && (suit == trump))
    suit++;
  return suit;
}

}  // namespace

TEST(NextQuickTrickSuit, NoTrumpAdvancesSequentially)
{
  EXPECT_EQ(next_quick_trick_suit(0, DDS_NOTRUMP), 1);
  EXPECT_EQ(next_quick_trick_suit(1, DDS_NOTRUMP), 2);
  EXPECT_EQ(next_quick_trick_suit(2, DDS_NOTRUMP), 3);
  EXPECT_EQ(next_quick_trick_suit(3, DDS_NOTRUMP), 4);
}

TEST(NextQuickTrickSuit, AfterTrumpJumpsToSuitZeroOrOne)
{
  // Spades trump: after visiting trump (0), continue at hearts (1).
  EXPECT_EQ(next_quick_trick_suit(0, 0), 1);

  // Hearts trump: after visiting trump (1), restart at spades (0).
  EXPECT_EQ(next_quick_trick_suit(1, 1), 0);

  // Diamonds / clubs trump likewise restart at spades.
  EXPECT_EQ(next_quick_trick_suit(2, 2), 0);
  EXPECT_EQ(next_quick_trick_suit(3, 3), 0);
}

TEST(NextQuickTrickSuit, SkipsTrumpWhenAdvancingPastIt)
{
  // Hearts trump: 0 -> 2 (skip 1), 2 -> 3, 3 -> 4.
  EXPECT_EQ(next_quick_trick_suit(0, 1), 2);
  EXPECT_EQ(next_quick_trick_suit(2, 1), 3);
  EXPECT_EQ(next_quick_trick_suit(3, 1), 4);

  // Diamonds trump: 0 -> 1, 1 -> 3 (skip 2), 3 -> 4.
  EXPECT_EQ(next_quick_trick_suit(0, 2), 1);
  EXPECT_EQ(next_quick_trick_suit(1, 2), 3);
  EXPECT_EQ(next_quick_trick_suit(3, 2), 4);
}

TEST(NextQuickTrickSuit, MatchesReferenceForAllSuitTrumpPairs)
{
  for (int trump = 0; trump <= DDS_NOTRUMP; ++trump)
  {
    for (int suit = 0; suit < DDS_SUITS; ++suit)
    {
      EXPECT_EQ(next_quick_trick_suit(suit, trump),
                reference_next_suit(suit, trump))
          << "suit=" << suit << " trump=" << trump;
    }
  }
}

TEST(NextQuickTrickSuit, FullTrumpFirstIterationOrder)
{
  // Hearts trump iteration: 1, then 0, 2, 3.
  int suit = 1;
  EXPECT_EQ((suit = next_quick_trick_suit(suit, 1)), 0);
  EXPECT_EQ((suit = next_quick_trick_suit(suit, 1)), 2);
  EXPECT_EQ((suit = next_quick_trick_suit(suit, 1)), 3);
  EXPECT_EQ((suit = next_quick_trick_suit(suit, 1)), 4);
}
