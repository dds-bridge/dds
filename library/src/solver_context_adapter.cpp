#include <solver_if.hpp>
#include <solver_context/solver_context.hpp>
#include <pbn.hpp>

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

auto solve_board_pbn(
  SolverContext& ctx,
  const DealPBN& dlpbn,
  int target,
  int solutions,
  int mode,
  FutureTricks* futp) -> int
{
  Deal dl;
  if (convert_from_pbn(dlpbn.remainCards, dl.remainCards) != RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  for (int k = 0; k <= 2; k++)
  {
    dl.currentTrickRank[k] = dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k] = dlpbn.currentTrickSuit[k];
  }
  dl.first = dlpbn.first;
  dl.trump = dlpbn.trump;

  return solve_board(ctx, dl, target, solutions, mode, futp);
}
