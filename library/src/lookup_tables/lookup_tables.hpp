/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/**
 * @brief Representation of a suit bitmask as runs of adjacent ranks.
 *
 * A 13-bit vector encodes ranks 2..A (LSB = deuce, MSB = ace). This structure
 * decomposes the vector into up to 7 contiguous runs of bits. It is used to
 * generate move sequences and to reason about gaps between runs.
 *
 * Example: For bitmask 0x1D62 (binary 1 1101 0110 0010):
 * - Group 0: rank 4 (top card), sequence 0x0002, gap 0x0000
 * - Group 1: rank 6, sequence 0x0000, gap 0x0008  
 * - Group 2: rank 9, sequence 0x0040, gap 0x0020
 * - Group 3: rank 14 (Ace), sequence 0x0C00, gap 0x0300
 * - last_group = 3
 *
 * @note Maximum 7 groups due to 13-bit constraint (alternating runs)
 */
struct MoveGroupType
{
  /**
   * @brief Index of the last valid group (run) in this representation.
   * 
   * Valid range: -1 (empty suit) to 6 (maximum 7 groups).
   * Groups are indexed from 0 to last_group_ (inclusive).
   */
  int last_group_;

  /**
   * @brief For each group g, the absolute rank (2..14) of the top card.
   * 
   * Rank encoding: 2=deuce, ..., 10=ten, 11=Jack, 12=Queen, 13=King, 14=Ace.
   * Only indices 0..last_group_ contain valid data.
   */
  int rank_[7];

  /**
   * @brief For each group g, bitmask of the sequence excluding the top card.
   * 
   * This represents the "tail" of the run below the top card.
   * Example: For AKQ, top=Ace(0x1000), sequence=0x0C00 (K=0x0800 | Q=0x0400).
   * Only indices 0..last_group_ contain valid data.
   */
  int sequence_[7];

  /**
   * @brief For each group g, bitmask of the full sequence including top card.
   * 
   * This is the complete run including the top card: 
   * fullseq[g] = (1 << (rank[g] - 2)) | sequence[g]
   * where rank[g] is the absolute rank (2..14).
   * Equivalently: fullseq[g] = bit_map_rank[rank[g]] | sequence[g].
   * Example: For AKQ, fullseq=0x1C00 (A=0x1000 | K=0x0800 | Q=0x0400).
   * Only indices 0..last_group_ contain valid data.
   */
  int fullseq_[7];

  /**
   * @brief For each group g (g>=1), bitmask of the gap between group g and g-1.
   * 
   * Represents the missing ranks between two consecutive runs.
   * gap[0] is not used (no gap before first group).
   * Only indices 1..last_group_ contain valid gap data.
   */
  int gap_[7];
};

/**
 * @brief Initialize all precomputed lookup tables for suit analysis.
 *
 * Precomputes and initializes all lookup tables used for move generation,
 * rank calculations, and group decomposition. This includes:
 * - highest_rank and lowest_rank tables
 * - count_table for binary weight calculation
 * - rel_rank for relative rank mapping
 * - win_ranks for top-N card selection
 * - group_data for run decomposition
 *
 * The tables are eagerly initialized at program startup via static initialization
 * (DdsLutInitGuard). Explicit calls to this function are safe but redundant;
 * they become no-ops after initial startup initialization.
 *
 * @note Thread-safe via std::call_once synchronization
 * @note Initialization occurs automatically at program startup
 * @note Explicit calls after startup are no-ops with minimal overhead
 * @note Safe to call multiple times from any thread
 *
 * @see highest_rank, lowest_rank, count_table, rel_rank, win_ranks, group_data
 */
auto init_lookup_tables() -> void;

/**
 * @brief Read-only views of the precomputed lookup tables.
 *
 * These are exposed as const references to fixed-size arrays to provide
 * zero-overhead indexing (e.g., highest_rank[i], rel_rank[i][j]). The
 * underlying storage is initialized once via init_lookup_tables().
 *
 * All tables are indexed by an "aggregate" value - a 13-bit suit bitmask
 * where bit 0 = deuce (2), bit 1 = 3, ..., bit 12 = Ace (14).
 * Valid aggregate range: 0..8191 (0x0000 to 0x1FFF).
 */

/**
 * @brief Highest absolute rank in the suit represented by an aggregate value.
 *
 * For a given aggregate (suit bitmask), returns the absolute rank (2..14)
 * of the highest card, or 0 if the aggregate is empty.
 *
 * @note Indexed by aggregate value (0..8191)
 * @note Return value: 0 (empty) or 2..14 (deuce..Ace)
 * @see lowest_rank, count_table
 */
extern const int (&highest_rank)[8192];

/**
 * @brief Lowest absolute rank in the suit represented by an aggregate value.
 *
 * For a given aggregate (suit bitmask), returns the absolute rank (2..14)
 * of the lowest card, or 0 if the aggregate is empty.
 *
 * @note Indexed by aggregate value (0..8191)
 * @note Return value: 0 (empty) or 2..14 (deuce..Ace)
 * @see highest_rank, count_table
 */
extern const int (&lowest_rank)[8192];

/**
 * @brief Count of set bits (card count) in an aggregate value.
 *
 * Returns the number of cards represented by the aggregate bitmask.
 * Also known as the binary weight or population count.
 *
 * @note Indexed by aggregate value (0..8191)
 * @note Return value: 0..13 (number of cards in suit)
 * @see highest_rank, lowest_rank
 */
extern const int (&count_table)[8192];

/**
 * @brief Relative rank lookup table for cards in an aggregate.
 *
 * For a given aggregate and absolute rank, returns the relative rank
 * (ordinal position from top) of that card within the suit.
 * Example: In AKT, Ace has relative rank 1, King has 2, Ten has 3.
 *
 * @note First index: aggregate value (0..8191)
 * @note Second index: absolute rank (2..14)
 * @note Return value: 0 (not present) or 1..13 (relative rank from top)
 * @see highest_rank, count_table
 */
extern const char (&rel_rank)[8192][15];

/**
 * @brief Winners mask representing top N cards of an aggregate.
 *
 * For a given aggregate and count N, returns a bitmask containing only
 * the top N cards from the original aggregate.
 * Example: For AKQJT with N=3, returns AKQ (masking out JT).
 *
 * @note First index: aggregate value (0..8191)
 * @note Second index: N = number of top cards to keep (0..13)
 * @note Return value: bitmask with top N cards only
 * @see highest_rank, count_table
 */
extern const unsigned short (&win_ranks)[8192][14];

/**
 * @brief Run decomposition data for each aggregate value.
 *
 * For a given aggregate (suit bitmask), provides a decomposition into
 * runs of consecutive ranks, including gap information between runs.
 * This is essential for move generation and suit analysis.
 *
 * @note Indexed by aggregate value (0..8191)
 * @note Contains up to 7 groups (runs) per aggregate
 * @see MoveGroupType for structure details
 */
extern const MoveGroupType (&group_data)[8192];
