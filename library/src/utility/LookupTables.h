/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_UTILITY_LOOKUPTABLES_H
#define DDS_UTILITY_LOOKUPTABLES_H

// Definition of moveGroupType
struct moveGroupType
{
  // There are at most 7 groups of bit "runs" in a 13-bit vector
  int lastGroup;
  int rank[7];
  int sequence[7];
  int fullseq[7];
  int gap[7];
};

// Initialization function (safe to call multiple times)
// Ensures the lookup tables are initialized once per process.
void InitLookupTables();

// Read-only accessors implemented as lightweight proxy tables that
// preserve existing "table[index]" syntax while prohibiting mutation.

struct HighestRankTable {
  int operator[](int i) const noexcept;
};

struct LowestRankTable {
  int operator[](int i) const noexcept;
};

struct CountTable {
  int operator[](int i) const noexcept;
};

struct RelRankRow {
  int i; // aggregate index
  char operator[](int j) const noexcept;
};

struct RelRankTable {
  RelRankRow operator[](int i) const noexcept;
};

struct WinRanksRow {
  int i; // aggregate index
  unsigned short operator[](int j) const noexcept;
};

struct WinRanksTable {
  WinRanksRow operator[](int i) const noexcept;
};

struct GroupDataTable {
  const moveGroupType& operator[](int i) const noexcept;
};

// Externally visible, read-only table objects
extern const HighestRankTable highestRank;
extern const LowestRankTable lowestRank;
extern const CountTable counttable;
extern const RelRankTable relRank;
extern const WinRanksTable winRanks;
extern const GroupDataTable groupData;

#endif
