/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dds_data_types.hpp>


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


/**
 * @brief Validate the vulnerability argument shared by the par entry points.
 *
 * DealerPar() indexes VUL_LOOKUP with this value, so it must be range-checked
 * there. SidesParBin() only compares against it, which is memory-safe but
 * silently treats any out-of-range value as "none vulnerable" -- so Par() and
 * SidesPar() would return RETURN_NO_FAULT with a result computed under the
 * wrong vulnerability while DealerPar() rejected the same input. Both use this
 * helper so the entry points agree.
 *
 * @param vulnerable 0 = None, 1 = Both, 2 = NS, 3 = EW.
 * @return RETURN_NO_FAULT when in range, RETURN_UNKNOWN_FAULT otherwise.
 */
inline auto par_vulnerable_checks(int const vulnerable) -> int
{
  if (vulnerable < 0 || vulnerable > 3)
    return RETURN_UNKNOWN_FAULT;

  return RETURN_NO_FAULT;
}
