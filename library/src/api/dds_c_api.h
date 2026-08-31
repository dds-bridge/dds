/*
   DDS, a bridge double dummy solver.

   C-ABI shim over the modern SolverContext API (dds_api.hpp).

   Every function here is C-callable and takes only pointers and plain-old-data
   by value: the opaque handle is a void*, and non-POD C++ types (SolverConfig,
   TTKind, C++ references) never appear. This gives Java (FFM/jextract), .NET,
   and ctypes a single clean, stable ABI to bind against.

   NOTE: the *exported symbols* are a pure C ABI, but this header is not itself
   compilable by a C front-end: it pulls in <api/dds_c_data_types.h>, which in
   turn includes <api/dds_constants.hpp>, where the shared constants are C++
   `constexpr` (and other declarations use C++-only syntax). Consume the ABI by
   binding to the compiled library's symbols (FFM/ctypes/.NET) or by parsing
   the headers with a C++ mode (jextract); do not include this from a C
   translation unit.

   See LICENSE and README.
*/

#pragma once

#include <api/dds_c_data_types.h>   /* struct Deal, FutureTricks, DdTableDeal, DdTableDealPBN, DdTableResults, ParResults; DLLEXPORT */

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque solver-context handle. Owns per-context solver state and the
   transposition table. Create with dds_c_create_solvercontext_default and
   release with dds_c_destroy_solvercontext. Not thread-safe: use one handle
   per thread. */
typedef void* DDS_C_SOLVER_CTX;

/* Creation / destruction. */
DLLEXPORT DDS_C_SOLVER_CTX dds_c_create_solvercontext_default(void);
DLLEXPORT void             dds_c_destroy_solvercontext(DDS_C_SOLVER_CTX ctx);

/* Solve a single board. Returns a RETURN_* status code. */
DLLEXPORT int dds_c_solve_board(DDS_C_SOLVER_CTX ctx,
                                const struct Deal* dl,
                                int target, int solutions, int mode,
                                struct FutureTricks* futp);

/* Solve a single board in PBN format. Returns a RETURN_* status code. */
DLLEXPORT int dds_c_solve_board_pbn(DDS_C_SOLVER_CTX ctx,
                                    const struct DealPBN* dlpbn,
                                    int target, int solutions, int mode,
                                    struct FutureTricks* futp);

/* Compute the double dummy table for a deal. */
DLLEXPORT int dds_c_calc_dd_table(DDS_C_SOLVER_CTX ctx,
                                  const struct DdTableDeal* deal,
                                  struct DdTableResults* results);

/* Compute the par result for a deal (computes the DD table internally). */
DLLEXPORT int dds_c_calc_par(DDS_C_SOLVER_CTX ctx,
                             const struct DdTableDeal* deal,
                             int vulnerable,
                             struct DdTableResults* results,
                             struct ParResults* par);

/* Compute the par result for a PBN-format deal (computes the DD table
   internally). */
DLLEXPORT int dds_c_calc_par_pbn(DDS_C_SOLVER_CTX ctx,
                                 const struct DdTableDealPBN* deal,
                                 int vulnerable,
                                 struct DdTableResults* results,
                                 struct ParResults* par);

/* Creation with explicit transposition-table configuration. The C++ SolverConfig
   is decomposed into scalars rather than mirrored as a struct: passing a struct
   by value is exactly the ABI question this shim exists to avoid, and a mirror
   type would be a second definition to keep in sync. tt_kind: 0 = Small,
   1 = Large (matching enum class TTKind). Returns NULL on failure. */
DLLEXPORT DDS_C_SOLVER_CTX dds_c_create_solvercontext(int tt_kind,
                                                      int def_mb, int max_mb);

/* Compute the double dummy table from a PBN-format deal. */
DLLEXPORT int dds_c_calc_dd_table_pbn(DDS_C_SOLVER_CTX ctx,
                                      const struct DdTableDealPBN* deal,
                                      struct DdTableResults* results);

/* Transposition-table configuration. */
DLLEXPORT void dds_c_configure_tt(DDS_C_SOLVER_CTX ctx, int tt_kind,
                                  int def_mb, int max_mb);
DLLEXPORT void dds_c_resize_tt(DDS_C_SOLVER_CTX ctx, int def_mb, int max_mb);
DLLEXPORT void dds_c_clear_tt(DDS_C_SOLVER_CTX ctx);

/* Per-solve state resets. */
DLLEXPORT void dds_c_reset_for_solve(DDS_C_SOLVER_CTX ctx);
DLLEXPORT void dds_c_reset_best_moves_lite(DDS_C_SOLVER_CTX ctx);

/* Logging passthrough. msg is a NUL-terminated UTF-8 string. */
DLLEXPORT void dds_c_log_append(DDS_C_SOLVER_CTX ctx, const char* msg);
DLLEXPORT void dds_c_log_clear(DDS_C_SOLVER_CTX ctx);

/* Context-free utilities. These operate on already-produced data (a
   DdTableResults/ParResultsMaster) or static library info; they need no
   SolverContext and so take no handle. */

/* Compute par from an already-computed double dummy table. */
DLLEXPORT int dds_c_par_from_table(const struct DdTableResults* table,
                                   int vulnerable,
                                   struct ParResults* par);

/* Compute par from both the NS and EW dealing sides' viewpoints. */
DLLEXPORT int dds_c_sides_par(const struct DdTableResults* table,
                              struct ParResultsDealer sides_res[2],
                              int vulnerable);

/* Compute par for a specific dealer. */
DLLEXPORT int dds_c_dealer_par(const struct DdTableResults* table,
                               struct ParResultsDealer* par,
                               int dealer, int vulnerable);

/* Binary (ContractType) variant of dds_c_dealer_par. */
DLLEXPORT int dds_c_dealer_par_bin(const struct DdTableResults* table,
                                   struct ParResultsMaster* par,
                                   int dealer, int vulnerable);

/* Binary (ContractType) variant of dds_c_sides_par. */
DLLEXPORT int dds_c_sides_par_bin(const struct DdTableResults* table,
                                  struct ParResultsMaster sides_res[2],
                                  int vulnerable);

/* Format a dds_c_dealer_par_bin() result as dealer-oriented text. */
DLLEXPORT int dds_c_convert_to_dealer_text_format(const struct ParResultsMaster* par,
                                                  char* resp);

/* Format a dds_c_sides_par_bin() result (both sides) as sides-oriented text.
   par must point to a 2-element array, one entry per side, matching
   dds_c_sides_par_bin's output -- not a single dds_c_dealer_par_bin() result. */
DLLEXPORT int dds_c_convert_to_sides_text_format(const struct ParResultsMaster par[2],
                                                 struct ParTextResults* resp);

/* Query library version/build information. */
DLLEXPORT void dds_c_get_dds_info(struct DDSInfo* info);

/* Map a RETURN_* status code to its human-readable text. */
DLLEXPORT void dds_c_error_message(int code, char line[80]);

#ifdef __cplusplus
}
#endif
