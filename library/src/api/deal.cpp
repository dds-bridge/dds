/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <api/dll.h>
#include <lookup_tables/lookup_tables.hpp>


auto Deal::fanout() const -> int
{
  // The fanout for a given suit and a given player is the number
  // of bit groups, so KT982 has 3 groups. In a given suit the
  // maximum number over all four players is 13.
  // A void counts as the sum of the other players' groups.

  int fanout = 0;
  int fanoutSuit, numVoids, c;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    fanoutSuit = 0;
    numVoids = 0;
    for (int s = 0; s < DDS_SUITS; s++)
    {
      c = static_cast<int>(remainCards[h][s] >> 2);
      fanoutSuit += group_data[c].last_group_ + 1;
      if (c == 0)
        numVoids++;
    }
    fanoutSuit += numVoids * fanoutSuit;
    fanout += fanoutSuit;
  }

  return fanout;
}
