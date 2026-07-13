/*
   DDS, a bridge double dummy solver.

   Pure-C ABI shim over the modern SolverContext API (dds_api.hpp).

   Every function here is C-callable and takes only pointers and plain-old-data
   by value: the opaque handle is a void*, and non-POD C++ types (SolverConfig,
   TTKind, C++ references) never appear. This gives Java (FFM/jextract), .NET,
   and ctypes a single clean, stable ABI to bind against.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>   /* struct Deal, FutureTricks, DdTableDeal, DdTableResults, ParResults */

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

/* Solve a single board. Returns an RETURN_* status code. */
DLLEXPORT int dds_c_solve_board(DDS_C_SOLVER_CTX ctx,
                                const struct Deal* dl,
                                int target, int solutions, int mode,
                                struct FutureTricks* futp);

/* Compute the double dummy table for a deal. */
DLLEXPORT int dds_c_calc_dd_table(DDS_C_SOLVER_CTX ctx,
                                  const struct DdTableDeal* deal,
                                  struct DdTableResults* results);

/* Compute the par result for a deal's double dummy table. */
DLLEXPORT int dds_c_calc_par(DDS_C_SOLVER_CTX ctx,
                             const struct DdTableDeal* deal,
                             int vulnerable,
                             struct DdTableResults* results,
                             struct ParResults* par);

#ifdef __cplusplus
}
#endif
