#ifndef DDS_API_SOLVE_BOARD_HPP
#define DDS_API_SOLVE_BOARD_HPP

#include <api/dds.h>
#include <solver_context/solver_context.hpp>

// C++-only overload exposed via <api/solve_board.hpp> for clients managing solver state.
auto SolveBoard(
	SolverContext& ctx,
	const deal& dl,
	int target,
	int solutions,
	int mode,
	futureTricks* futp) -> int;

#endif // DDS_API_SOLVE_BOARD_HPP
