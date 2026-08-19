/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include "deal_fanout.hpp"

#include <atomic>

#include <lookup_tables/lookup_tables.hpp>

namespace dds
{
namespace internal
{

namespace
{
std::atomic<int> g_deal_fanout_call_count{0};
}  // namespace

auto deal_fanout_call_count() -> int
{
  return g_deal_fanout_call_count.load(std::memory_order_relaxed);
}

auto deal_fanout(const Deal& dl) -> int
{
  g_deal_fanout_call_count.fetch_add(1, std::memory_order_relaxed);

  // The fanout for a given suit and a given player is the number
  // of bit groups, so KT982 has 3 groups. In a given suit the
  // maximum number over all four players is 13.
  // A void counts as the sum of the other players' groups.

  int fanout = 0;
  int fanout_suit, num_voids, c;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    fanout_suit = 0;
    num_voids = 0;
    for (int s = 0; s < DDS_SUITS; s++)
    {
      c = static_cast<int>(dl.remainCards[h][s] >> 2);
      fanout_suit += group_data[c].last_group_ + 1;
      if (c == 0)
        num_voids++;
    }
    fanout_suit += num_voids * fanout_suit;
    fanout += fanout_suit;
  }

  return fanout;
}

}  // namespace internal
}  // namespace dds
