/*
   DDS, a bridge double dummy solver.

   Implementation of the pure-C ABI shim. Most functions cast the opaque
   void* handle back to SolverContext* and forward to the reference-taking
   dds_* functions declared in dds_api.hpp. SolverConfig / TTKind stay internal
   to this translation unit and never cross the shim boundary.

   A second, smaller group of functions needs no SolverContext at all: they
   operate on already-produced data (a DdTableResults/ParResultsMaster) or
   static library info. Those take no handle and forward straight to the
   corresponding legacy dll.h function, which is already POD-only and
   extern "C" — there is no C++ type to keep off the ABI boundary, so no
   dds_api.hpp intermediate is needed for them.

   See LICENSE and README.
*/

#include <api/dds_c_api.h>
#include <api/dds_api.hpp>
#include <api/dll.h>   /* legacy Par/SidesPar/DealerPar/.../GetDDSInfo/ErrorMessage */

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

DLLEXPORT int dds_c_solve_board_pbn(DDS_C_SOLVER_CTX ctx,
                                    const struct DealPBN* dlpbn,
                                    int target, int solutions, int mode,
                                    struct FutureTricks* futp)
{
    if (ctx == nullptr || dlpbn == nullptr || futp == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return dds_solve_board_pbn(static_cast<SolverContext*>(ctx),
            *dlpbn, target, solutions, mode, futp);
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

DLLEXPORT int dds_c_calc_par_pbn(DDS_C_SOLVER_CTX ctx,
                                 const struct DdTableDealPBN* deal,
                                 int vulnerable,
                                 struct DdTableResults* results,
                                 struct ParResults* par)
{
    if (ctx == nullptr || deal == nullptr || results == nullptr || par == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return dds_calc_par_pbn(static_cast<SolverContext*>(ctx),
            *deal, vulnerable, results, par);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT DDS_C_SOLVER_CTX dds_c_create_solvercontext(int tt_kind,
                                                      int def_mb, int max_mb)
{
    try {
        SolverConfig cfg;
        cfg.tt_kind_ = static_cast<TTKind>(tt_kind);
        cfg.tt_mem_default_mb_ = def_mb;
        cfg.tt_mem_maximum_mb_ = max_mb;
        return static_cast<DDS_C_SOLVER_CTX>(dds_create_solvercontext(cfg));
    } catch (...) {
        return nullptr;
    }
}

DLLEXPORT int dds_c_calc_dd_table_pbn(DDS_C_SOLVER_CTX ctx,
                                      const struct DdTableDealPBN* deal,
                                      struct DdTableResults* results)
{
    if (ctx == nullptr || deal == nullptr || results == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return dds_calc_dd_table_pbn(static_cast<SolverContext*>(ctx),
            *deal, results);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT void dds_c_configure_tt(DDS_C_SOLVER_CTX ctx, int tt_kind,
                                  int def_mb, int max_mb)
{
    if (ctx == nullptr)
        return;

    try {
        dds_configure_tt(static_cast<SolverContext*>(ctx),
            static_cast<TTKind>(tt_kind), def_mb, max_mb);
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_resize_tt(DDS_C_SOLVER_CTX ctx, int def_mb, int max_mb)
{
    if (ctx == nullptr)
        return;

    try {
        dds_resize_tt(static_cast<SolverContext*>(ctx), def_mb, max_mb);
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_clear_tt(DDS_C_SOLVER_CTX ctx)
{
    if (ctx == nullptr)
        return;

    try {
        dds_clear_tt(static_cast<SolverContext*>(ctx));
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_reset_for_solve(DDS_C_SOLVER_CTX ctx)
{
    if (ctx == nullptr)
        return;

    try {
        dds_reset_for_solve(static_cast<SolverContext*>(ctx));
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_reset_best_moves_lite(DDS_C_SOLVER_CTX ctx)
{
    if (ctx == nullptr)
        return;

    try {
        dds_reset_best_moves_lite(static_cast<SolverContext*>(ctx));
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_log_append(DDS_C_SOLVER_CTX ctx, const char* msg)
{
    if (ctx == nullptr || msg == nullptr)
        return;

    try {
        dds_log_append(static_cast<SolverContext*>(ctx), msg);
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_log_clear(DDS_C_SOLVER_CTX ctx)
{
    if (ctx == nullptr)
        return;

    try {
        dds_log_clear(static_cast<SolverContext*>(ctx));
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

/* --- Context-free utilities: no SolverContext, forward straight to the
   legacy dll.h function. --- */

DLLEXPORT int dds_c_par_from_table(const struct DdTableResults* table,
                                   int vulnerable,
                                   struct ParResults* par)
{
    if (table == nullptr || par == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return Par(table, par, vulnerable);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_sides_par(const struct DdTableResults* table,
                              struct ParResultsDealer sides_res[2],
                              int vulnerable)
{
    if (table == nullptr || sides_res == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return SidesPar(table, sides_res, vulnerable);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_dealer_par(const struct DdTableResults* table,
                               struct ParResultsDealer* par,
                               int dealer, int vulnerable)
{
    if (table == nullptr || par == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return DealerPar(table, par, dealer, vulnerable);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_dealer_par_bin(const struct DdTableResults* table,
                                   struct ParResultsMaster* par,
                                   int dealer, int vulnerable)
{
    if (table == nullptr || par == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return DealerParBin(table, par, dealer, vulnerable);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_sides_par_bin(const struct DdTableResults* table,
                                  struct ParResultsMaster sides_res[2],
                                  int vulnerable)
{
    if (table == nullptr || sides_res == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return SidesParBin(table, sides_res, vulnerable);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_convert_to_dealer_text_format(const struct ParResultsMaster* par,
                                                  char* resp)
{
    if (par == nullptr || resp == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return ConvertToDealerTextFormat(par, resp);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT int dds_c_convert_to_sides_text_format(const struct ParResultsMaster par[2],
                                                 struct ParTextResults* resp)
{
    if (par == nullptr || resp == nullptr)
        return RETURN_UNKNOWN_FAULT;

    try {
        return ConvertToSidesTextFormat(par, resp);
    } catch (...) {
        return RETURN_UNKNOWN_FAULT;
    }
}

DLLEXPORT void dds_c_get_dds_info(struct DDSInfo* info)
{
    if (info == nullptr)
        return;

    try {
        GetDDSInfo(info);
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

DLLEXPORT void dds_c_error_message(int code, char line[80])
{
    if (line == nullptr)
        return;

    try {
        ErrorMessage(code, line);
    } catch (...) {
        // Must not unwind through the C ABI boundary.
    }
}

} // extern "C"
