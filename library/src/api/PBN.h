/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>


/**
 * @brief Convert a PBN (Portable Bridge Notation) deal string to DDS card array.
 *
 * Parses a PBN-format deal string and fills the DDS card array with the remaining cards for each hand and suit.
 *
 * @param dealBuff PBN-format deal string.
 * @param remainCards Output array for remaining cards per hand and suit.
 * @return 1 if successful, 0 otherwise.
 */
auto ConvertFromPBN(
  char const * dealBuff,
  unsigned int remainCards[DDS_HANDS][DDS_SUITS]) -> int;

/**
 * @brief Convert a PBN-format play trace to binary play trace.
 *
 * Converts a play trace from PBN format to the binary playTraceBin structure for analysis.
 *
 * @param playPBN Play trace in PBN format.
 * @param playBin Output binary play trace structure.
 * @return 1 if successful, 0 otherwise.
 */
auto ConvertPlayFromPBN(
  const playTracePBN& playPBN,
  playTraceBin& playBin) -> int;
