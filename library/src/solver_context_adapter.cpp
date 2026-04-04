#include <solver_if.hpp>
#include <solver_context/solver_context.hpp>

auto solve_board(
  SolverContext& ctx,
  const Deal& dl,
  int target,
  int solutions,
  int mode,
  FutureTricks* futp) -> int
{
  // Use ThreadData-attached TT so all contexts created in lower layers
  // observe the same table. No ownership adoption to avoid duplication.
  return solve_board_internal(ctx, dl, target, solutions, mode, futp);
}

auto SolveBoard(
  SolverContext& ctx,
  const Deal& dl,
  int target,
  int solutions,
  int mode,
  FutureTricks* futp) -> int
{
  return solve_board(ctx, dl, target, solutions, mode, futp);
}
