/*
   DDS, a bridge double dummy solver.

   Implementation of the pure-C ABI shim. Each function casts the opaque
   void* handle back to SolverContext* and forwards to the reference-taking
   dds_* functions declared in dds_api.hpp. SolverConfig / TTKind stay internal
   to this translation unit and never cross the shim boundary.

   See LICENSE and README.
*/

#include <api/dds_c_api.h>
#include <api/dds_api.hpp>

/* This shim is the boundary between the C++ implementation and pure-C FFI
   consumers (JVM/FFM, .NET, ctypes). Two things must never cross it:

     - C++ exceptions: unwinding through a C ABI frame is undefined behavior,
       so every entry point wraps its body in catch-all handlers.
     - Undefined behavior from NULL handles/arguments: the reference-taking
       dds_* functions dereference their inputs, so we validate pointers up
       front and fail fast with RETURN_UNKNOWN_FAULT instead of crashing the
       caller's process. */

extern "C" {

DLLEXPORT DDS_C_SOLVER_CTX dds_c_create_solvercontext_default(void)
{
    try {
        return static_cast<DDS_C_SOLVER_CTX>(dds_create_solvercontext_default());
    } catch (...) {
        return nullptr;
    }
}

DLLEXPORT void dds_c_destroy_solvercontext(DDS_C_SOLVER_CTX ctx)
{
    if (ctx == nullptr)
        return;

    try {
        dds_destroy_solvercontext(static_cast<SolverContext*>(ctx));
    } catch (...) {
        // A destructor must not let an exception escape the C ABI boundary.
    }
}

DLLEXPORT int dds_c_solve_board(DDS_C_SOLVER_CTX ctx,
                                const struct Deal* dl,
                                int target, int solutions, int mode,
                                struct FutureTricks* futp)
{
    if (ctx == nullptr || dl == nullptr || futp == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return dds_solve_board(static_cast<SolverContext*>(ctx),
            *dl, target, solutions, mode, futp);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_calc_dd_table(DDS_C_SOLVER_CTX ctx,
                                  const struct DdTableDeal* deal,
                                  struct DdTableResults* results)
{
    if (ctx == nullptr || deal == nullptr || results == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return dds_calc_dd_table(static_cast<SolverContext*>(ctx), *deal, results);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_calc_par(DDS_C_SOLVER_CTX ctx,
                             const struct DdTableDeal* deal,
                             int vulnerable,
                             struct DdTableResults* results,
                             struct ParResults* par)
{
    if (ctx == nullptr || deal == nullptr || results == nullptr || par == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return dds_calc_par(static_cast<SolverContext*>(ctx),
            *deal, vulnerable, results, par);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

} // extern "C"
