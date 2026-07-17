/// @file deal_fanout_test.cpp
/// @brief Unit tests for Deal::fanout (structural difficulty estimate).

#include <gtest/gtest.h>

#include <api/dll.h>
#include <lookup_tables/lookup_tables.hpp>
#include <utility/constants.h>

namespace
{

/// Pack suit holding bits into the remainCards encoding (aggregate << 2).
auto holding(const unsigned aggregate) -> unsigned
{
  return aggregate << 2;
}

auto empty_deal() -> Deal
{
  Deal dl{};
  return dl;
}

}  // namespace

TEST(DealFanout, EmptyDealIsZero)
{
  // Arrange
  const Deal dl = empty_deal();

  // Act / Assert
  EXPECT_EQ(dl.fanout(), 0);
}

TEST(DealFanout, SingleCardCountsAsOneGroupWithVoidBonus)
{
  // Arrange: North holds only the deuce of spades; all other holdings empty.
  // That hand: 1 group + 3 voids * 1 = 4. Other hands: all voids → 0.
  Deal dl = empty_deal();
  dl.remainCards[0][0] = holding(bit_map_rank[2]);

  // Act / Assert
  EXPECT_EQ(dl.fanout(), 4);
}

TEST(DealFanout, ConsecutiveRanksAreOneGroup)
{
  // Arrange: North holds KT982 of spades (K | T98 | 2 → 3 groups).
  // That hand: 3 groups + 3 voids * 3 = 12. Other hands: 0.
  const unsigned kt982 =
    bit_map_rank[13] |  // K
    bit_map_rank[10] |  // T
    bit_map_rank[9] |   // 9
    bit_map_rank[8] |   // 8
    bit_map_rank[2];    // 2

  Deal dl = empty_deal();
  dl.remainCards[0][0] = holding(kt982);

  // Act / Assert
  EXPECT_EQ(group_data[kt982].last_group_ + 1, 3);
  EXPECT_EQ(dl.fanout(), 12);
}

TEST(DealFanout, SolidSuitIsOneGroup)
{
  // Arrange: North holds AKQ of hearts (one consecutive group), voids elsewhere.
  // That hand: 1 + 3*1 = 4.
  const unsigned akq =
    bit_map_rank[14] | bit_map_rank[13] | bit_map_rank[12];

  Deal dl = empty_deal();
  dl.remainCards[0][1] = holding(akq);

  // Act / Assert
  EXPECT_EQ(group_data[akq].last_group_ + 1, 1);
  EXPECT_EQ(dl.fanout(), 4);
}
