---
capability: transposition-table
owners: [trans_table]
last-updated: 2026-07-18
related-plans: []
---

# Transposition Table

> **Specs vs. doxygen.** The `TransTable` interface, `NodeCards` layout, and each
> method's contract are documented inline in `trans_table.hpp`. This spec records
> the capability-wide facts: the two implementations, the memory/reset model, and
> how the table relates to the owning context.

## Purpose

The transposition table caches double-dummy search results (bounds, best move,
and move-ordering hints for a position) so the alpha-beta search avoids
re-solving positions it has already seen. It is the single biggest memory
consumer in a solve and the main reason reusing a [[solver-context]] across
solves is worthwhile. This capability provides the abstract table interface and
its two concrete strategies, trading memory against speed.

## Behaviour & invariants

> Per-method signatures live in the header doxygen; these are the whole-table
> guarantees.

- **One interface, two implementations.** `TransTable` is an abstract base;
  `TransTableL` (large) is the full-featured, faster, paged-memory table with
  harvesting, and `TransTableS` (small) is the pool-based, lower-memory, somewhat
  slower table. Which one a context uses is chosen by `TTKind::{Large,Small}` in
  `SolverConfig` (default `Large`) — see [[solver-context]].
- **Not thread-safe.** A table instance must be accessed from a single solver
  thread. Concurrency comes from one table per context/worker, never a shared
  table under a lock.
- **`NodeCards` is a tightly packed 8-byte record** (upper/lower trick bounds,
  best-move suit/rank, per-suit `least_win` encoding). The compactness is
  deliberate — it caps the per-entry footprint of large tables. A pointer
  returned by `lookup()` is valid only until the next `add()` or reset.
- **Two-tier memory limits.** `set_memory_default` is a soft limit the table
  tries to stay under (may briefly exceed, triggering cleanup/harvesting);
  `set_memory_maximum` is a hard cap — allocations that would exceed it are
  refused and trigger a reset. On the raw `TransTable` interface, `0` default
  means unlimited; the owning [[solver-context]] replaces `<= 0` config values
  with `THREADMEM_*` constants before construct, so context-owned tables never
  start as unlimited. Env overrides: `DDS_TT_DEFAULT_MB` / `DDS_TT_LIMIT_MB`.
- **Resets are reason-tagged and tiered.** `reset_memory(ResetReason)` clears
  cached positions and statistics but retains the allocated structures for reuse;
  `return_all_memory()` deallocates everything (requires `make_tt()` / in-table
  reallocation before reuse). `ResetReason` (`TooManyNodes`, `NewDeal`,
  `NewTrump`, `MemoryExhausted`, `FreeMemory`, …) records *why* a reset happened
  for diagnostics.
- **Allocation failures throw `std::bad_alloc`** (post-modernization); non-critical
  allocations may fall back rather than throw.
- **Lifecycle on the production context path:** first TT access does
  `set_memory_*` → `make_tt()`; per-deal `init(hand_lookup)` runs later in
  `init.cpp` before search. Standalone/header examples may show `init` first —
  that is not what `SearchContext::trans_table()` does.
- **Build-flag-gated diagnostics.** `tt_reset_debug` (`DDS_DEBUG_TT_RESET`) enables
  reset-tracking diagnostics in `TransTableL` (wired through [[build-system]], off
  by default). `tt_context_ownership` / `DDS_TT_CONTEXT_OWNERSHIP` remains a Bazel
  `--define` in `CPPVARIABLES.bzl` but is **unused in library sources** today —
  ownership is always instance-scoped via `SearchContext`.
- **`testable_trans_table`** is the same sources exposed to
  `//library/tests/trans_table` for white-box testing; behaviour is identical to
  `trans_table`.

## Key entry points

- `library/src/trans_table/trans_table.hpp` — `TransTable` abstract interface,
  `NodeCards`, `ResetReason`. Doxygen documents every method.
- `library/src/trans_table/trans_table_l.{hpp,cpp}` — `TransTableL` (large/paged).
- `library/src/trans_table/trans_table_s.{hpp,cpp}` — `TransTableS` (small/pool).
- Build targets: `//library/src/trans_table:{trans_table,testable_trans_table}`.
- Guarded by `//library/tests/trans_table` and, for context wiring,
  `//library/tests/system:{tt_sharing_test,configure_tt_api_test}`.

## Known gaps / non-goals

- No cross-thread or cross-process sharing of a table; in-memory, single-thread
  only.
- The table does not choose its own size strategy — kind and limits are dictated
  by the owning [[solver-context]] / config, not decided internally.
- Print/diagnostic methods are for offline analysis and emit only under the
  relevant debug builds; they are not part of the hot path.
