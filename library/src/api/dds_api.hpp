// File: dds_api.hpp
#pragma once
#include <solver_context/solver_context.hpp>
#include <api/solve_board.hpp>
#include <api/calc_dd_table.hpp>

extern "C" {

	// Opaque handle type for C#/PInvoke
	typedef SolverContext* DDS_SOLVER_CTX;

	// Creation
	EXTERN_C DLLEXPORT DDS_SOLVER_CTX dds_create_solvercontext_default();

	EXTERN_C DLLEXPORT DDS_SOLVER_CTX dds_create_solvercontext(SolverConfig cfg);

	// SolverContext Destruction
	EXTERN_C DLLEXPORT void dds_destroy_solvercontext(DDS_SOLVER_CTX ctx);

	// TT Configuration
	EXTERN_C DLLEXPORT void dds_configure_tt(DDS_SOLVER_CTX ctx, 
											 TTKind kind, 
											 int defMB, int maxMB);

	EXTERN_C DLLEXPORT void dds_resize_tt(DDS_SOLVER_CTX ctx, 
										  int defMB, 
										  int maxMB);

	EXTERN_C DLLEXPORT void dds_clear_tt(DDS_SOLVER_CTX ctx);

	// Resets
	EXTERN_C DLLEXPORT void dds_reset_for_solve(DDS_SOLVER_CTX ctx);

	EXTERN_C DLLEXPORT void dds_reset_best_moves_lite(DDS_SOLVER_CTX ctx);

	// Utilities – simple logging passthrough
	EXTERN_C DLLEXPORT void dds_log_append(DDS_SOLVER_CTX ctx, 
														  const char* msg);

	EXTERN_C DLLEXPORT void dds_log_clear(DDS_SOLVER_CTX ctx);

	EXTERN_C DLLEXPORT 	auto dds_solve_board(DDS_SOLVER_CTX ctx,
										   	 const Deal& dl,
											 int target,
											 int solutions,
											 int mode,
											 FutureTricks* futp) -> int;

	EXTERN_C DLLEXPORT	auto dds_solve_board_pbn(DDS_SOLVER_CTX ctx,
												 const DealPBN& dlpbn,
												 int target,
												 int solutions,
												 int mode,
												 FutureTricks* futp) -> int;

	EXTERN_C DLLEXPORT auto dds_calc_dd_table(
		DDS_SOLVER_CTX  ctx,
		const DdTableDeal& table_deal,
		DdTableResults* table_results) -> int;


	EXTERN_C DLLEXPORT auto dds_calc_dd_table_pbn(
		DDS_SOLVER_CTX  ctx,
		const DdTableDealPBN& table_deal,
		DdTableResults* table_results) -> int;

	EXTERN_C DLLEXPORT	auto dds_calc_par(
		DDS_SOLVER_CTX  ctx,
		const DdTableDeal& table_deal,
		int vulnerable,
		DdTableResults* table_results,
		ParResults* par_results) -> int;

	EXTERN_C DLLEXPORT	auto dds_calc_par_pbn(
		DDS_SOLVER_CTX  ctx,
		const DdTableDealPBN& table_deal_pbn,
		int vulnerable,
		DdTableResults* table_results,
		ParResults* par_results) -> int;


} 
