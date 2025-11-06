/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <lookup_tables/LookupTables.hpp>
#include <utility/Constants.h>
#include <cstring>
#include <mutex>

namespace {
  // Underlying storage (internal linkage)
  static int dds_lut_highestRank_storage[8192];
  static int dds_lut_lowestRank_storage[8192];
  static int dds_lut_counttable_storage[8192];
  static char dds_lut_relRank_storage[8192][15];
  static unsigned short dds_lut_winRanks_storage[8192][14];
  static MoveGroupType dds_lut_groupData_storage[8192];

  static std::once_flag dds_lut_once_flag;
}

static auto dds_lut_init_impl() -> void
{
  // highestRank[aggr] is the highest absolute rank in the
  // suit represented by aggr. The absolute rank is 2 .. 14.
  // Similarly for lowestRank.
  dds_lut_highestRank_storage[0] = 0;
  dds_lut_lowestRank_storage[0] = 0;
  for (int aggr = 1; aggr < 8192; aggr++)
  {
    for (int r = 14; r >= 2; r--)
    {
      if (aggr & bitMapRank[r])
      {
        dds_lut_highestRank_storage[aggr] = r;
        break;
      }
    }
    for (int r = 2; r <= 14; r++)
    {
      if (aggr & bitMapRank[r])
      {
        dds_lut_lowestRank_storage[aggr] = r;
        break;
      }
    }
  }

  // The use of the counttable to give the number of bits set to
  // one in an integer follows an implementation by Thomas Andrews.

  // counttable[aggr] is the number of '1' bits (binary weight)
  // in aggr.
  for (int aggr = 0; aggr < 8192; aggr++)
  {
    dds_lut_counttable_storage[aggr] = 0;
    for (int r = 0; r < 13; r++)
    {
      if (aggr & (1 << r))
      {
        dds_lut_counttable_storage[aggr]++;
      }
    }
  }

  // relRank[aggr][absolute rank] is the relative rank of
  // that absolute rank in the suit represented by aggr.
  // The relative rank is 2 .. 14.
  memset(dds_lut_relRank_storage[0], 0, 15);
  for (int aggr = 1; aggr < 8192; aggr++)
  {
    char ord = 0;
    for (int r = 14; r >= 2; r--)
    {
      if (aggr & bitMapRank[r])
      {
        ord++;
        dds_lut_relRank_storage[aggr][r] = ord;
      }
    }
  }

  // winRanks[aggr][leastWin] is the absolute suit represented
  // by aggr, but limited to its top "leastWin" bits.
  for (int aggr = 0; aggr < 8192; aggr++)
  {
    dds_lut_winRanks_storage[aggr][0] = 0;
    for (int leastWin = 1; leastWin < 14; leastWin++)
    {
      int res = 0;
      int nextBitNo = 1;
      for (int r = 14; r >= 2; r--)
      {
        if (aggr & bitMapRank[r])
        {
          if (nextBitNo <= leastWin)
          {
            res |= bitMapRank[r];
            nextBitNo++;
          }
          else
            break;
        }
      }
      dds_lut_winRanks_storage[aggr][leastWin] = static_cast<unsigned short>(res);
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

  int topside[15] =
  {
    0x0000, 0x0000, 0x0000, 0x0001, // 2, 3,
    0x0003, 0x0007, 0x000f, 0x001f, // 4, 5, 6, 7,
    0x003f, 0x007f, 0x00ff, 0x01ff, // 8, 9, T, J,
    0x03ff, 0x07ff, 0x0fff          // Q, K, A
  };

  int botside[15] =
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

  dds_lut_groupData_storage[0].last_group_ = -1;

  dds_lut_groupData_storage[1].last_group_ = 0;
  dds_lut_groupData_storage[1].rank_[0] = 2;
  dds_lut_groupData_storage[1].sequence_[0] = 0;
  dds_lut_groupData_storage[1].fullseq_[0] = 1;
  dds_lut_groupData_storage[1].gap_[0] = 0;

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

    dds_lut_groupData_storage[ris] = dds_lut_groupData_storage[ris ^ topBitRank];

    if (ris & nextBitRank) // 11... Extend group
    {
      g = dds_lut_groupData_storage[ris].last_group_;
      dds_lut_groupData_storage[ris].rank_[g]++;
      dds_lut_groupData_storage[ris].sequence_[g] |= nextBitRank;
      dds_lut_groupData_storage[ris].fullseq_[g] |= topBitRank;
    }
    else // 10... New group
    {
      g = ++dds_lut_groupData_storage[ris].last_group_;
      dds_lut_groupData_storage[ris].rank_[g] = topBitNo;
      dds_lut_groupData_storage[ris].sequence_[g] = 0;
      dds_lut_groupData_storage[ris].fullseq_[g] = topBitRank;
      dds_lut_groupData_storage[ris].gap_[g] =
        topside[topBitNo] & botside[ dds_lut_groupData_storage[ris].rank_[g - 1] ];
    }
  }
}

auto init_lookup_tables() -> void
{
  std::call_once(dds_lut_once_flag, dds_lut_init_impl);
}

// Eager initialization at program start (TU load) to avoid any cost on first use.
namespace {
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
const MoveGroupType (&group_data)[8192] = dds_lut_groupData_storage;
const int (&highest_rank)[8192] = dds_lut_highestRank_storage;
const int (&lowest_rank)[8192] = dds_lut_lowestRank_storage;
const int (&count_table)[8192] = dds_lut_counttable_storage;
const char (&rel_rank)[8192][15] = dds_lut_relRank_storage;
const unsigned short (&win_ranks)[8192][14] = dds_lut_winRanks_storage;
