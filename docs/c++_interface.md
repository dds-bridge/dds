# C++ Interface Documentation

## Overview

DDS provides a modernized C++ interface built around instance-scoped solver state.

- Preferred umbrella include: `#include <dds/dds.hpp>`
- Core modernization concept: `SolverContext`

The modern API is designed to preserve existing solver behavior while improving ownership, lifecycle control, and composability in C++ applications.

## Design Goals

The modern C++ interface emphasizes:

- Instance-scoped state: each `SolverContext` owns or references its own thread state.
- RAII resource management: context lifetime drives solver resource lifetime.
- Explicit transposition-table lifecycle control.
- Snake_case C++ entry points.

## Public C++ Headers

Primary entry points:

- `library/src/dds.hpp` (exported as `<dds/dds.hpp>`)
- `library/src/api/solve_board.hpp`
- `library/src/api/calc_dd_table.hpp`
- `library/src/api/calc_par.hpp`
- `library/src/solver_context/solver_context.hpp`

## Core Types

### SolverConfig

`SolverConfig` controls context-level transposition table configuration.

Fields:

- `tt_kind_`: `TTKind::Small` or `TTKind::Large`
- `tt_mem_default_mb_`: default TT memory in MB
- `tt_mem_maximum_mb_`: maximum TT memory in MB

### SolverContext

`SolverContext` is the primary C++ state holder for solving operations.

Capabilities include:

- Access to thread data via `thread()`
- Solver utility facade via `utilities()`
- Search state facade via `search()`
- TT lifecycle methods:
  - `trans_table()` / `maybe_trans_table()`
  - `reset_for_solve()`
  - `reset_best_moves_lite()`
  - `clear_tt()`
  - `dispose_trans_table()`
  - `configure_tt(...)`

Threading note:

- `SolverContext` is not inherently thread-safe.
- Use one context per worker thread.

## Modern C++ API Functions

### Solve APIs

Preferred context-aware solve function:

- `solve_board(SolverContext& ctx, const Deal& dl, int target, int solutions, int mode, FutureTricks* futp)`

Compatibility wrapper also available in C++:

- `SolveBoard(SolverContext& ctx, const Deal& dl, int target, int solutions, int mode, FutureTricks* futp)`

### DD Table APIs

Temporary-context overloads:

- `calc_dd_table(const DdTableDeal& table_deal, DdTableResults* table_results)`
- `calc_dd_table_pbn(const DdTableDealPBN& table_deal_pbn, DdTableResults* table_results)`

Context-aware overloads:

- `calc_dd_table(SolverContext& ctx, const DdTableDeal& table_deal, DdTableResults* table_results)`
- `calc_dd_table_pbn(SolverContext& ctx, const DdTableDealPBN& table_deal_pbn, DdTableResults* table_results)`

### Par APIs

Par with table calculation:

- `calc_par(const DdTableDeal& table_deal, int vulnerable, DdTableResults* table_results, ParResults* par_results)`
- `calc_par(SolverContext& ctx, const DdTableDeal& table_deal, int vulnerable, DdTableResults* table_results, ParResults* par_results)`

Par from precomputed table:

- `calc_par_from_table(const DdTableResults* table_results, int vulnerable, ParResults* par_results)`

## Data Structures and Status Codes

All core deal/result structs and return codes are defined in `api/dll.h` and exposed via `dds.hpp`.

Examples include:

- `Deal`, `DealPBN`, `Boards`, `BoardsPBN`
- `FutureTricks`, `DdTableDeal`, `DdTableResults`, `ParResults`
- `RETURN_NO_FAULT` and other error/status constants

## Usage Example

```cpp
#include <dds/dds.hpp>
#include <iostream>

int main()
{
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_default_mb_ = 256;
    cfg.tt_mem_maximum_mb_ = 512;

    SolverContext ctx(cfg);

    Deal dl{};
    // Fill dl.trump, dl.first, dl.currentTrickSuit/Rank, dl.remainCards

    FutureTricks fut{};
    const int rc = solve_board(ctx, dl, -1, 3, 0, &fut);

    if (rc != RETURN_NO_FAULT) {
        std::cerr << "solve_board failed: " << rc << "\n";
        return rc;
    }

    std::cout << "cards=" << fut.cards << " nodes=" << fut.nodes << "\n";

    // Optional lifecycle control between independent batches:
    // ctx.reset_for_solve();

    return 0;
}
```

## Build and Verification

Build all targets:

```bash
bazel build //...
```

Run tests:

```bash
bazel test //...
```

Build docs package:

```bash
bazel build //:doxygen_docs
```
