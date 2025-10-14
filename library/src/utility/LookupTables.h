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
 * \brief Read-only views of the precomputed lookup tables.
 *
 * These are exposed as const references to fixed-size arrays to provide
 * zero-overhead indexing (e.g., highestRank[i], relRank[i][j]). The
 * underlying storage is initialized once via InitLookupTables().
 */

/** \brief Read-only table: highest absolute rank per aggregate. */
extern const int (&highestRank)[8192];
/** \brief Read-only table: lowest absolute rank per aggregate. */
extern const int (&lowestRank)[8192];
/** \brief Read-only table: count of set bits per aggregate. */
extern const int (&counttable)[8192];
/** \brief Read-only table: relative rank lookup per aggregate. */
extern const char (&relRank)[8192][15];
/** \brief Read-only table: winners mask limited to top N cards. */
extern const unsigned short (&winRanks)[8192][14];
/** \brief Read-only table: run decomposition per aggregate. */
extern const moveGroupType (&groupData)[8192];

#endif
