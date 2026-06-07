// File: dds_api.hpp
#pragma once
#include <solver_context/solver_context.hpp>
#include <api/solve_board.hpp>
#include <api/solve_board.hpp>

extern "C" {

	// Opaque handle type for C#/PInvoke
	typedef SolverContext* DDS_SOLVER_CTX;

	// Creation
	EXTERN_C DLLEXPORT DDS_SOLVER_CTX dds_create_solvercontext_default()
	{
		SolverConfig cfg{};
		return new SolverContext(cfg);
	}

	EXTERN_C DLLEXPORT DDS_SOLVER_CTX dds_create_solvercontext(SolverConfig cfg)
	{
		return new SolverContext(cfg);
	}

	// SolverContext Destruction
	EXTERN_C DLLEXPORT void dds_destroy_solvercontext(DDS_SOLVER_CTX ctx)
	{
		delete ctx;
	}

	// TT Configuration
	EXTERN_C DLLEXPORT void dds_configure_tt(DDS_SOLVER_CTX ctx, 
											 TTKind kind, 
											 int defMB, int maxMB)
	{
		ctx->configure_tt(kind, defMB, maxMB);
	}

	EXTERN_C DLLEXPORT void dds_resize_tt(DDS_SOLVER_CTX ctx, 
										  int defMB, 
										  int maxMB)
	{
		ctx->resize_tt(defMB, maxMB);
	}

	EXTERN_C DLLEXPORT void dds_clear_tt(DDS_SOLVER_CTX ctx)
	{
		ctx->clear_tt();
	}

	// Resets
	EXTERN_C DLLEXPORT void dds_reset_for_solve(DDS_SOLVER_CTX ctx)
	{
		ctx->reset_for_solve();
	}

	EXTERN_C DLLEXPORT void dds_reset_best_moves_lite(DDS_SOLVER_CTX ctx)
	{
		ctx->reset_best_moves_lite();
	}

	// Utilities – simple logging passthrough
	EXTERN_C DLLEXPORT void dds_log_append(DDS_SOLVER_CTX ctx, 
														  const char* msg)
	{
		ctx->utilities().log_append(std::string(msg));
	}

	EXTERN_C DLLEXPORT void dds_log_clear(DDS_SOLVER_CTX ctx)
	{
		ctx->utilities().log_clear();
	}

	EXTERN_C DLLEXPORT 	auto dds_solve_board(DDS_SOLVER_CTX ctx,
										   	 const Deal& dl,
											 int target,
											 int solutions,
											 int mode,
											 FutureTricks* futp) -> int
	{
		return SolveBoard(*ctx,
						   dl,
						   target,
						   solutions,
						   mode,
						   futp) ;
	}


} 
