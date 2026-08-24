/*
   DDS, a bridge double dummy solver.

   See LICENSE and README.
*/

/// @file calc_dd_table_pbn_fuzz.cpp
/// @brief Fuzz harness for CalcDDtablePBN().
///
/// This is the path a PBN file takes into the solver: text parsing followed by
/// a full double dummy table calculation. DdTableDealPBN::cards is a fixed
/// char[80], so the harness copies at most 79 bytes and terminates the buffer
/// itself; handing the library a non-terminated array would be a harness bug
/// rather than a library one.

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <api/dll.h>

extern "C" auto LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/) -> int
{
  // SetMaxThreads() is a deprecated alias of InitializeStaticMemory() whose
  // thread argument is ignored, so it never capped anything here. Worker
  // counts come from each call's explicit maxThreads instead.
  InitializeStaticMemory();
  return 0;
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int
{
  DdTableDealPBN table_deal;
  std::memset(&table_deal, 0, sizeof(table_deal));

  size_t const n = size < sizeof(table_deal.cards) - 1
                     ? size
                     : sizeof(table_deal.cards) - 1;
  // libFuzzer may pass (nullptr, 0), and memcpy's source is declared nonnull,
  // so an empty copy from a null pointer is undefined even though it moves
  // nothing. UBSan on glibc reports it; guard rather than rely on the libc.
  if (n > 0)
    std::memcpy(table_deal.cards, data, n);
  table_deal.cards[n] = '\0';

  DdTableResults table;
  std::memset(&table, 0, sizeof(table));

  // The non-N entry point delegates with maxThreads = 0, which selects
  // hardware concurrency; call the N variant so one input uses one worker.
  CalcDDtablePBNN(table_deal, &table, 1);

  return 0;
}
