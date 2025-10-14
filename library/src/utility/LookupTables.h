/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_UTILITY_LOOKUPTABLES_H
#define DDS_UTILITY_LOOKUPTABLES_H

/**
 * \brief Representation of a suit bitmask as runs of adjacent ranks.
 *
 * A 13-bit vector encodes ranks 2..A (LSB = deuce, MSB = ace). This structure
 * decomposes the vector into up to 7 contiguous runs of bits. It is used to
 * generate move sequences and to reason about gaps between runs.
 */
struct moveGroupType
{
  /** \brief Index of the last valid group (run) in this representation.
   *  -1 indicates there are no groups (empty suit).
   */
  int lastGroup;

  /** \brief For each group g, the absolute rank (2..14) of the top card. */
  int rank[7];

  /** \brief For each group g, bitmask of the sequence excluding the top card. */
  int sequence[7];

  /** \brief For each group g, bitmask of the full sequence including top card. */
  int fullseq[7];

  /** \brief For each group g (g>=1), bitmask of the gap between group g and g-1. */
  int gap[7];
};

/**
 * \brief Initialize the lookup tables.
 *
 * Safe to call multiple times. Internally guarded by std::call_once so
 * initialization happens at most once per process. Accessing any table via
 * the read-only proxies also triggers lazy initialization.
 */
void InitLookupTables();

/**
 * \brief Read-only accessors that preserve array-like syntax.
 *
 * These lightweight proxy tables provide `operator[]` to look up values while
 * preventing mutation. They mirror the historical data layout:
 * - Index domain for the first dimension is 0..8191 (13-bit aggregate).
 * - Values are precomputed and initialized once per process.
 */

/** \brief Highest absolute rank in a suit aggregate.
 *
 * For a given aggregate bitmask (0..8191), returns the highest absolute rank
 * present (2..14). Returns 0 for an empty aggregate (0).
 */
struct HighestRankTable {
  /** Lookup by aggregate bitmask. */
  int operator[](int i) const noexcept;
};

/** \brief Lowest absolute rank in a suit aggregate (2..14). */
struct LowestRankTable {
  /** Lookup by aggregate bitmask. */
  int operator[](int i) const noexcept;
};

/** \brief Population count (number of set bits) of an aggregate (0..13). */
struct CountTable {
  /** Lookup by aggregate bitmask. */
  int operator[](int i) const noexcept;
};

/** \brief Row proxy for relative ranks within a given aggregate. */
struct RelRankRow {
  int i; //!< aggregate index (0..8191)
  /** Lookup by absolute rank (2..14) to get the relative rank position. */
  char operator[](int j) const noexcept;
};

/** \brief Relative rank lookup table: relRank[aggr][absRank] -> relative rank.
 *
 * Provides the position (1..13) of an absolute rank within the aggregate when
 * ranks are ordered descending (A..2). Values are 0 if the rank is not present.
 */
struct RelRankTable {
  /** Get a row proxy for a specific aggregate. */
  RelRankRow operator[](int i) const noexcept;
};

/** \brief Row proxy for winners bitmasks limited to top N cards. */
struct WinRanksRow {
  int i; //!< aggregate index (0..8191)
  /** Lookup by leastWin (0..13) to get mask of top N absolute ranks. */
  unsigned short operator[](int j) const noexcept;
};

/** \brief Winners lookup: winRanks[aggr][leastWin] -> bitmask of top N ranks. */
struct WinRanksTable {
  /** Get a row proxy for a specific aggregate. */
  WinRanksRow operator[](int i) const noexcept;
};

/** \brief Group data lookup: groupData[aggr] -> run decomposition. */
struct GroupDataTable {
  /** Access the read-only group decomposition for an aggregate. */
  const moveGroupType& operator[](int i) const noexcept;
};

/** \brief Read-only table: highest absolute rank per aggregate. */
extern const HighestRankTable highestRank;
/** \brief Read-only table: lowest absolute rank per aggregate. */
extern const LowestRankTable lowestRank;
/** \brief Read-only table: count of set bits per aggregate. */
extern const CountTable counttable;
/** \brief Read-only table: relative rank lookup per aggregate. */
extern const RelRankTable relRank;
/** \brief Read-only table: winners mask limited to top N cards. */
extern const WinRanksTable winRanks;
/** \brief Read-only table: run decomposition per aggregate. */
extern const GroupDataTable groupData;

#endif
