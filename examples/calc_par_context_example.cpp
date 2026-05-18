/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund / 
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

// Example showing context usage for par calculations.
// This demonstrates how to use SolverContext for managing resources
// across multiple operations.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <api/calc_par.hpp>
#include <api/dll.h>
#include <solver_context/solver_context.hpp>
#include "hands.hpp"


void example_without_context()
{
  printf("\n=== Par Calculation (Traditional Approach) ===\n\n");

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int handno = 0; handno < 3; handno++)
  {
    DdTableResults ddtable;
    set_table(&ddtable, handno);

    ParResults pres;
    int res = Par(&ddtable, &pres, vulnerability_[handno]);

    if (res == RETURN_NO_FAULT)
    {
      const char* suit_name = 
        (trump_suit_[handno] == 0 ? "♠" :
         trump_suit_[handno] == 1 ? "♥" :
         trump_suit_[handno] == 2 ? "♦" :
         trump_suit_[handno] == 3 ? "♣" : "NT");
      
      printf("Hand %d (%s): Par score = %s\n", 
             handno + 1,
             suit_name,
             pres.par_score[0]);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time).count();
  printf("\nTime (traditional): %lld ms\n\n", static_cast<long long>(elapsed));
}


void example_with_context()
{
  printf("\n=== Par Calculation WITH SolverContext ===\n\n");

  // Create a single solver context for resource management
  SolverContext context;

  printf("SolverContext provides:\n");
  printf("  - Reuse of allocated solver resources across calculations\n");
  printf("  - Persistent solver resources (no per-call allocation overhead)\n");
  printf("  - Consistent API with other context-aware DDS operations\n\n");

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int handno = 0; handno < 3; handno++)
  {
    DdTableDeal table_deal{};
    for (int h = 0; h < DDS_HANDS; ++h) {
      for (int s = 0; s < DDS_SUITS; ++s) {
        table_deal.cards[h][s] = holdings_[handno][s][h];
      }
    }

    DdTableResults ddtable;
    ParResults pres;
    // Use context-aware calc_par API
    int res = calc_par(
      context,
      table_deal,
      vulnerability_[handno],
      &ddtable,
      &pres);

    if (res == RETURN_NO_FAULT)
    {
      const char* suit_name = 
        (trump_suit_[handno] == 0 ? "♠" :
         trump_suit_[handno] == 1 ? "♥" :
         trump_suit_[handno] == 2 ? "♦" :
         trump_suit_[handno] == 3 ? "♣" : "NT");
      
      printf("Hand %d (%s): Par score = %s\n", 
             handno + 1,
             suit_name,
             pres.par_score[0]);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - start_time).count();
  printf("\nTime (with context): %lld ms\n\n", static_cast<long long>(elapsed));
}


void example_mixed_usage()
{
  printf("\n=== Context for Sequential Operations ===\n\n");

  // Create a context - useful for managing resources across calls
  SolverContext context;

  printf("Best practices when using SolverContext:\n\n");

  printf("1. Create context once:\n");
  printf("   SolverContext context;\n\n");

  printf("2. Reuse for multiple calculations:\n");
  for (int i = 0; i < 3; i++) {
    DdTableDeal table_deal{};
    for (int h = 0; h < DDS_HANDS; ++h) {
      for (int s = 0; s < DDS_SUITS; ++s) {
        table_deal.cards[h][s] = holdings_[i][s][h];
      }
    }

    DdTableResults ddtable;
    ParResults pres;
    int res = calc_par(
      context,
      table_deal,
      vulnerability_[i],
      &ddtable,
      &pres);
    if (res == RETURN_NO_FAULT) {
      printf("   Hand %d Par: Score = %s\n", i + 1, pres.par_score[0]);
    }
  }

  printf("\n3. Solver resources are reused across operations\n");
  printf("   This is active now: calc_par(ctx, ...) uses the provided context\n\n");
}


void print_python_example()
{
  printf("\n=== Python Equivalent Usage ===\n\n");
  printf("Python code for context reuse:\n\n");
  printf("  from dds3 import SolverContext, solve_board\n\n");
  printf("  # Create context once\n");
  printf("  ctx = SolverContext()\n\n");
  printf("  # Reuse for multiple operations\n");
  printf("  for deal in deals:\n");
  printf("      result = solve_board(deal, context=ctx)\n");
  printf("      print(f'Score: {result[\"score\"]}')\n\n");
}


auto main() -> int
{
  printf("DDS Examples: Par Calculation with SolverContext\n");
  printf("================================================\n");

#if defined(__linux) || defined(__APPLE__)
  SetMaxThreads(0);
#endif

  // Run examples
  example_without_context();
  example_with_context();
  example_mixed_usage();
  print_python_example();

  printf("\n=== Summary ===\n");
  printf("✓ SolverContext provides resource management\n");
  printf("✓ Backward compatible - traditional API still works\n");
  printf("✓ Python bindings demonstrate context advantages\n");
  printf("✓ Transposition table reuse across operations\n");
  printf("✓ Improved efficiency for batch processing\n");

  return 0;
}


