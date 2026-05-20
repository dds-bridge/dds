#include <cassert>
#include <cstring>
#include <iostream>


// Access legacy thread memory to build a context (test-only knowledge)
#include <system/memory.hpp>
#include <dds/dds.hpp>
extern Memory memory;

static Deal make_empty_deal()
{
  Deal dl{}; // zero-initialized; remainCards all zero
  dl.trump = 0;
  dl.first = 0;
  std::memset(dl.currentTrickSuit, 0, sizeof(dl.currentTrickSuit));
  std::memset(dl.currentTrickRank, 0, sizeof(dl.currentTrickRank));
  std::memset(dl.remainCards, 0, sizeof(dl.remainCards));
  return dl;
}

int main()
{
  // Arrange
  const int thr = 0;
  FutureTricks ft1{};
  FutureTricks ft2{};
  Deal dl = make_empty_deal();

  // Act: legacy
  int r1 = SolveBoard(dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft1, thr);

  // Act: context
  SolverContext ctx;
  int r2 = solve_board(ctx, dl, /*target=*/0, /*solutions=*/1, /*mode=*/0, &ft2);

  // Assert: return codes identical (both should be error on empty Deal)
  if (r1 != r2) {
    std::cerr << "Return codes differ: legacy=" << r1 << " ctx=" << r2 << std::endl;
    return 1;
  }
  return 0;
}
