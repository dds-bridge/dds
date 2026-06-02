/*
   Browser entry point for CalcDDtablePBN.

   Copyright 2020-2026 Adam Wildavsky
   Use of this source code is governed by the MIT license.
*/

#include <cstring>

#include <api/dll.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" {

// Fills out_table[20] with res_table[strain][hand] (strain 0..4 = S,H,D,C,N).
// Returns RETURN_NO_FAULT (1) on success, or a DDS error code.
EMSCRIPTEN_KEEPALIVE
auto dds_mvp_calc_table(const char* pbn, int* out_table) -> int {
  if (pbn == nullptr || out_table == nullptr) {
    return RETURN_UNKNOWN_FAULT;
  }

  DdTableDealPBN deal{};
  const size_t pbn_len = std::strlen(pbn);
  if (pbn_len >= sizeof(deal.cards)) {
    return RETURN_PBN_FAULT;
  }
  std::memcpy(deal.cards, pbn, pbn_len + 1);

  DdTableResults table{};
  const int res = CalcDDtablePBN(deal, &table);
  if (res != RETURN_NO_FAULT) {
    return res;
  }

  int k = 0;
  for (int strain = 0; strain < DDS_STRAINS; ++strain) {
    for (int hand = 0; hand < DDS_HANDS; ++hand) {
      out_table[k++] = table.res_table[strain][hand];
    }
  }
  return RETURN_NO_FAULT;
}

}  // extern "C"

#if !defined(__EMSCRIPTEN__) && !defined(DDS_MVP_WASM_NO_MAIN)
auto main() -> int { return 0; }
#endif
