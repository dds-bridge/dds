/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>


/**
 * @brief Validate a caller-supplied double dummy table before par calculation.
 *
 * Every entry of DdTableResults::res_table is a trick count and must lie in
 * [0, 13]. The par calculation derives contract levels directly from these
 * values and formats them into fixed-size character buffers, so an entry far
 * outside the legal range overflows those buffers. CalcDDtable() always
 * produces legal tables, but the par entry points are exported and a caller
 * may hand-build or deserialise a table, so the range is checked here rather
 * than assumed.
 *
 * @param tablep Table to validate. May be nullptr.
 * @return RETURN_NO_FAULT when every entry is in range,
 *         RETURN_PAR_TABLE_FAULT otherwise (including a nullptr table).
 */
inline auto par_table_checks(DdTableResults const * tablep) -> int
{
  if (tablep == nullptr)
    return RETURN_PAR_TABLE_FAULT;

  for (int d = 0; d < DDS_STRAINS; d++)
    for (int h = 0; h < DDS_HANDS; h++)
      if (tablep->res_table[d][h] < 0 || tablep->res_table[d][h] > 13)
        return RETURN_PAR_TABLE_FAULT;

  return RETURN_NO_FAULT;
}
