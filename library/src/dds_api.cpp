/*
   DDS, a bridge double dummy solver.

   Version 3 API implementation - Solver Context Management
   This file ensures the dds_api.hpp functions are compiled into the DLL.
*/

#include <api/dds_api.hpp>
#include <api/calc_par.hpp>
#include <string>

// Creation
DLLEXPORT DDS_SOLVER_CTX dds_create_solvercontext_default()
{
	SolverConfig cfg{};
	return new SolverContext(cfg);
}

DLLEXPORT DDS_SOLVER_CTX dds_create_solvercontext(SolverConfig cfg)
{
	return new SolverContext(cfg);
}

// SolverContext Destruction
DLLEXPORT void dds_destroy_solvercontext(DDS_SOLVER_CTX ctx)
{
	delete ctx;
}

// TT Configuration
DLLEXPORT void dds_configure_tt(DDS_SOLVER_CTX ctx, TTKind kind, int defMB, int maxMB)
{
	ctx->configure_tt(kind, defMB, maxMB);
}

DLLEXPORT void dds_resize_tt(DDS_SOLVER_CTX ctx, int defMB, int maxMB)
{
	ctx->resize_tt(defMB, maxMB);
}

DLLEXPORT void dds_clear_tt(DDS_SOLVER_CTX ctx)
{
	ctx->clear_tt();
}

// Resets
DLLEXPORT void dds_reset_for_solve(DDS_SOLVER_CTX ctx)
{
	ctx->reset_for_solve();
}

DLLEXPORT void dds_reset_best_moves_lite(DDS_SOLVER_CTX ctx)
{
	ctx->reset_best_moves_lite();
}

// Utilities – simple logging passthrough
DLLEXPORT void dds_log_append(DDS_SOLVER_CTX ctx, const char* msg)
{
	ctx->utilities().log_append(std::string(msg ? msg : ""));
}

DLLEXPORT void dds_log_clear(DDS_SOLVER_CTX ctx)
{
	ctx->utilities().log_clear();
}

DLLEXPORT auto dds_solve_board(DDS_SOLVER_CTX ctx, const Deal& dl, int target, int solutions, int mode, FutureTricks* futp) -> int
{
	return SolveBoard(*ctx,
		dl,
		target,
		solutions,
		mode,
		futp);
}

DLLEXPORT auto dds_solve_board_pbn(DDS_SOLVER_CTX ctx, const DealPBN& dlpbn, int target, int solutions, int mode, FutureTricks* futp) -> int
{
	return solve_board_pbn(*ctx, dlpbn, target, solutions, mode, futp);
}

DLLEXPORT auto dds_calc_dd_table(DDS_SOLVER_CTX ctx, const DdTableDeal& table_deal, DdTableResults* table_results) -> int
{
	return calc_dd_table(*ctx, table_deal, table_results);
}

DLLEXPORT auto dds_calc_dd_table_pbn(DDS_SOLVER_CTX ctx, const DdTableDealPBN& table_deal, DdTableResults* table_results) -> int
{

	return calc_dd_table_pbn(*ctx, table_deal, table_results);
}

DLLEXPORT auto dds_calc_par(
	DDS_SOLVER_CTX ctx,
	const DdTableDeal& table_deal,
	int vulnerable,
	DdTableResults* table_results,
	ParResults* par_results) -> int
{
	return calc_par(*ctx, table_deal, vulnerable, table_results, par_results);
}

DLLEXPORT auto dds_calc_par_pbn(
	DDS_SOLVER_CTX ctx,
	const DdTableDealPBN& table_deal_pbn,
	int vulnerable,
	DdTableResults* table_results,
	ParResults* par_results) -> int
{
	return calc_par_pbn(*ctx, table_deal_pbn, vulnerable, table_results, par_results);
}

