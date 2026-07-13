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

extern "C" {

DLLEXPORT DDS_C_SOLVER_CTX dds_c_create_solvercontext_default(void)
{
	return static_cast<DDS_C_SOLVER_CTX>(dds_create_solvercontext_default());
}

DLLEXPORT void dds_c_destroy_solvercontext(DDS_C_SOLVER_CTX ctx)
{
	dds_destroy_solvercontext(static_cast<SolverContext*>(ctx));
}

DLLEXPORT int dds_c_solve_board(DDS_C_SOLVER_CTX ctx,
                                const struct Deal* dl,
                                int target, int solutions, int mode,
                                struct FutureTricks* futp)
{
	return dds_solve_board(static_cast<SolverContext*>(ctx),
		*dl, target, solutions, mode, futp);
}

DLLEXPORT int dds_c_calc_dd_table(DDS_C_SOLVER_CTX ctx,
                                  const struct DdTableDeal* deal,
                                  struct DdTableResults* results)
{
	return dds_calc_dd_table(static_cast<SolverContext*>(ctx), *deal, results);
}

DLLEXPORT int dds_c_calc_par(DDS_C_SOLVER_CTX ctx,
                             const struct DdTableDeal* deal,
                             int vulnerable,
                             struct DdTableResults* results,
                             struct ParResults* par)
{
	return dds_calc_par(static_cast<SolverContext*>(ctx),
		*deal, vulnerable, results, par);
}

} // extern "C"
