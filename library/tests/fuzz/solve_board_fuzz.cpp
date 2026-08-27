/*
   DDS, a bridge double dummy solver.

   See LICENSE and README.
*/

/// @file solve_board_fuzz.cpp
/// @brief Fuzz harness for SolveBoard(), the main solver entry point.
///
/// SolveBoard() validates its input thoroughly (see solver_if.cpp) before
/// handing control to the search. This harness drives both halves: arbitrary
/// bytes mostly exercise the validation layer, while the seed corpus starts
/// the fuzzer from legal deals so coverage feedback can reach the search
/// itself.

#include <cstdint>
#include <cstddef>
#include <cstring>

#include <api/dll.h>

extern "C" auto LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/) -> int
{
  // SetMaxThreads() is a deprecated alias of InitializeStaticMemory() whose
  // thread argument is ignored, so it never capped anything here. Worker
  // counts come from each call's explicit maxThreads instead.
  InitializeStaticMemory();
  return 0;
}

extern "C" auto LLVMFuzzerTestOneInput(const uint8_t * data, size_t size) -> int
{
  // trump, first, currentTrickSuit[3], currentTrickRank[3], remainCards[4][4],
  // plus one selector byte for target/solutions/mode.
  Deal deal;
  if (size < sizeof(deal) + 1)
    return 0;

  std::memcpy(&deal, data, sizeof(deal));
  uint8_t const selector = data[sizeof(deal)];

  // Cover the documented ranges and a little either side of them, so the
  // parameter validation is exercised as well as the search.
  int const target    = static_cast<int>(selector % 16) - 1;
  int const solutions = static_cast<int>((selector / 16) % 5) - 1;
  int const mode      = static_cast<int>((selector / 80) % 4) - 1;

  FutureTricks fut;
  std::memset(&fut, 0, sizeof(fut));

  SolveBoard(deal, target, solutions, mode, &fut, 0);

  return 0;
}
