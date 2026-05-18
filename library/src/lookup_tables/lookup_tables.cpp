/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <cstring>
#include <mutex>

#include <lookup_tables/lookup_tables.hpp>
#include <utility/constants.h>

namespace
{
  // Underlying storage (internal linkage)
  static int highest_rank_storage[8192];
  static int lowest_rank_storage[8192];
  static int count_table_storage[8192];
  static char rel_rank_storage[8192][15];
  static unsigned short win_ranks_storage[8192][14];
  static MoveGroupType group_data_storage[8192];

  static std::once_flag lookup_tables_init_flag;
}

static auto init_lookup_tables_impl() -> void
{
  // highestRank[aggregate] is the highest absolute rank in the
  // suit represented by aggr. The absolute rank is 2 .. 14.
  // Similarly for lowestRank.
  highest_rank_storage[0] = 0;
  lowest_rank_storage[0] = 0;
  for (int aggregate = 1; aggregate < 8192; aggregate++)
  {
    for (int rank = 14; rank >= 2; rank--)
    {
      if (aggregate & bit_map_rank[rank])
      {
        highest_rank_storage[aggregate] = rank;
        break;
      }
    }
    for (int rank = 2; rank <= 14; rank++)
    {
      if (aggregate & bit_map_rank[rank])
      {
        lowest_rank_storage[aggregate] = rank;
        break;
      }
    }
  }

  // The use of the counttable to give the number of bits set to
  // one in an integer follows an implementation by Thomas Andrews.

  // counttable[aggregate] is the number of '1' bits (binary weight)
  // in aggr.
  for (int aggregate = 0; aggregate < 8192; aggregate++)
  {
    count_table_storage[aggregate] = 0;
    for (int rank = 0; rank < 13; rank++)
    {
      if (aggregate & (1 << rank))
      {
        count_table_storage[aggregate]++;
      }
    }
  }

  // relRank[aggregate][absolute rank] is the relative rank of
  // that absolute rank in the suit represented by aggr.
  // The relative rank is 2 .. 14.
  memset(rel_rank_storage[0], 0, 15);
  for (int aggregate = 1; aggregate < 8192; aggregate++)
  {
    char ordinal = 0;
    for (int rank = 14; rank >= 2; rank--)
    {
      if (aggregate & bit_map_rank[rank])
      {
        ordinal++;
        rel_rank_storage[aggregate][rank] = ordinal;
      }
    }
  }

  // win_ranks[aggregate][least_win] is the absolute suit represented
  // by aggr, but limited to its top "leastWin" bits.
  for (int aggregate = 0; aggregate < 8192; aggregate++)
  {
    win_ranks_storage[aggregate][0] = 0;
    for (int least_win = 1; least_win < 14; least_win++)
    {
      int result = 0;
      int next_bit_position = 1;
      for (int rank = 14; rank >= 2; rank--)
      {
        if (aggregate & bit_map_rank[rank])
        {
          if (next_bit_position <= least_win)
          {
            result |= bit_map_rank[rank];
            next_bit_position++;
          }
          else
            break;
        }
      }
      win_ranks_storage[aggregate][least_win] = static_cast<unsigned short>(result);
    }
  }

  // groupData[ris] is a representation of the suit (ris is
  // "rank in suit") in terms of runs of adjacent bits.
  // 1 1100 1101 0110
  // has 4 runs, so last_group_ is 3, and the entries are
  // 0: 4 and 0x0002, gap 0x0000 (lowest gap unused, though)
  // 1: 6 and 0x0000, gap 0x0008
  // 2: 9 and 0x0040, gap 0x0020
  // 3: 14 and 0x0c00, gap 0x0300

  static const int topside[15] =
  {
    0x0000, 0x0000, 0x0000, 0x0001, // 2, 3,
    0x0003, 0x0007, 0x000f, 0x001f, // 4, 5, 6, 7,
    0x003f, 0x007f, 0x00ff, 0x01ff, // 8, 9, T, J,
    0x03ff, 0x07ff, 0x0fff          // Q, K, A
  };

  static const int botside[15] =
  {
    0xffff, 0xffff, 0x1ffe, 0x1ffc, // 2, 3,
    0x1ff8, 0x1ff0, 0x1fe0, 0x1fc0, // 4, 5, 6, 7,
    0x1f80, 0x1f00, 0x1e00, 0x1c00, // 8, 9, T, J,
    0x1800, 0x1000, 0x0000          // Q, K, A
  };

  // So the bit vector in the gap between a top card of K
  // and a bottom card of T is
  // topside[K] = 0x07ff &
  // botside[T] = 0x1e00
  // which is 0x0600, the binary code for QJ.

  group_data_storage[0].last_group_ = -1;

  group_data_storage[1].last_group_ = 0;
  group_data_storage[1].rank_[0] = 2;
  group_data_storage[1].sequence_[0] = 0;
  group_data_storage[1].fullseq_[0] = 1;
  group_data_storage[1].gap_[0] = 0;

  int topBitRank = 1;
  int nextBitRank = 0;
  int topBitNo = 2;
  int g;

  for (int ris = 2; ris < 8192; ris++)
  {
    if (ris >= (topBitRank << 1))
    {
      // Next top bit
      nextBitRank = topBitRank;
      topBitRank <<= 1;
      topBitNo++;
    }

    group_data_storage[ris] = group_data_storage[ris ^ topBitRank];

    if (ris & nextBitRank) // 11... Extend group
    {
      g = group_data_storage[ris].last_group_;
      group_data_storage[ris].rank_[g]++;
      group_data_storage[ris].sequence_[g] |= nextBitRank;
      group_data_storage[ris].fullseq_[g] |= topBitRank;
    }
    else // 10... New group
    {
      g = ++group_data_storage[ris].last_group_;
      group_data_storage[ris].rank_[g] = topBitNo;
      group_data_storage[ris].sequence_[g] = 0;
      group_data_storage[ris].fullseq_[g] = topBitRank;
      // gap_[g] is the gap between the current group g and the previous group (g-1).
      // Since g was just incremented and g >= 1 here, rank_[g-1] is always valid.
      // The first group (g == 0) is handled separately via explicit initialization above.
      group_data_storage[ris].gap_[g] =
        topside[topBitNo] & botside[ group_data_storage[ris].rank_[g - 1] ];
    }
  }
}

auto init_lookup_tables() -> void
{
  std::call_once(lookup_tables_init_flag, init_lookup_tables_impl);
}

// Eager initialization at program start (TU load) to avoid any cost on first use.
namespace
{
  struct DdsLutInitGuard
  {
    DdsLutInitGuard() noexcept
    {
      init_lookup_tables();
    }
  };
  static const DdsLutInitGuard dds_lut_init_guard;
}

// Bind const references to internal storage for zero-overhead access
const MoveGroupType (&group_data)[8192] = group_data_storage;
const int (&highest_rank)[8192] = highest_rank_storage;
const int (&lowest_rank)[8192] = lowest_rank_storage;
const int (&count_table)[8192] = count_table_storage;
const char (&rel_rank)[8192][15] = rel_rank_storage;
const unsigned short (&win_ranks)[8192][14] = win_ranks_storage;
