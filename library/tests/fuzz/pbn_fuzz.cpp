/*
   DDS, a bridge double dummy solver.

   See LICENSE and README.
*/

/// @file pbn_fuzz.cpp
/// @brief Fuzz harness for the PBN deal-string parser.
///
/// convert_from_pbn() is the library's main text-parsing surface and the one
/// path that routinely sees externally authored data (PBN files). It takes a
/// NUL-terminated string, so the harness terminates the input itself rather
/// than handing the parser a non-terminated buffer, which would report a
/// harness bug as a library bug.

#include <cstdint>
#include <cstddef>
#include <string>

#include <api/PBN.h>
#include <api/dll.h>

extern "C" auto LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/) -> int
{
  // Nothing to configure; defined so the corpus-replay driver links.
  return 0;
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int
{
  // PBN deal strings are bounded in practice; keep inputs in that range so
  // the fuzzer spends its budget on parser states rather than on length.
  if (size > 4096)
    return 0;

  std::string const deal(reinterpret_cast<char const *>(data), size);

  unsigned int remain_cards[DDS_HANDS][DDS_SUITS];
  convert_from_pbn(deal.c_str(), remain_cards);

  return 0;
}
