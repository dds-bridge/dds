---
capability: solver-context
owners: [solver_context]
last-updated: 2026-07-18
related-plans: []
---

# Solver Context

> **Specs vs. doxygen.** The per-method contracts of `SolverContext` and its
> facades are documented inline in `solver_context.hpp`. This spec records why the
> context exists and the lifecycle/ownership invariants that span the whole class.

## Purpose

`SolverContext` is the instance-scoped home for everything a solve needs that was
historically global mutable state: the per-thread `ThreadData`, the transposition
table, search state, move generation, and logging/stats. It exists so a caller can
create one context at the top of a call stack, drive multiple solves through it,
and reuse the (expensive) transposition table across them — instead of relying on
process-global state that forced serialised, non-reentrant use. It is the object
the modern C++ API (`dds_api.hpp`) and its C shim (`dds_c_api.h`) hand around as
the opaque handle. See [[dds-public-api]].

## Behaviour & invariants

> Method signatures live in the header's doxygen; these are the whole-context
> guarantees.

- **One context per thread.** `SolverContext` is not thread-safe. Each thread
  that solves owns its own context (and thus its own `ThreadData` and TT).
- **`ThreadData` is `shared_ptr`-owned.** A context either creates and owns its
  `ThreadData` (`SolverContext(SolverConfig)`) or wraps an externally-owned one
  (`SolverContext(shared_ptr<ThreadData>, cfg)`). Raw-`ThreadData*` constructors
  were removed in the ownership migration; non-owning wrappers use a
  no-op-deleter `shared_ptr`. The destructor closes debug files only when it is
  the last holder of that `ThreadData`.
- **The TT is owned per context, created lazily.** Each context's
  `SearchContext` owns its `TransTable` via a `unique_ptr`, allocated on first
  access. There is **no global TT registry and no `ThreadData`-owned TT** — TT
  reuse comes from reusing the *same context* across solves, not from a shared
  global. See [[transposition-table]].
- **TT configuration is `SolverConfig` + optional env overrides.** `SolverConfig`
  carries `tt_kind_` (`TTKind::{Small,Large}`, default `Large`) and default/max MB.
  `configure_tt(kind, defMB, maxMB)` persists a new config and applies it to an
  existing TT (resize in place, or recreate if the kind changes). Env overrides:
  `DDS_TT_DEFAULT_MB` (raise default) and `DDS_TT_LIMIT_MB` (cap maximum) when > 0.
- **Explicit, tiered reset hooks** (no-ops when no TT exists yet):
  `reset_for_solve()` clears a subset of search state and resets TT memory
  (`ResetReason::FreeMemory`) while preserving the allocation for reuse;
  `reset_best_moves_lite()` clears only best-move ranks (hot per-iteration path);
  `clear_tt()` calls `return_all_memory()` on the existing TT object (it does not
  destroy the `unique_ptr`; the next deal's `init` / use reallocates inside the
  same table); `dispose_trans_table()` destroys the TT immediately.
- **Hot-path facades are value-typed and inline-friendly, with different holds.**
  `MoveGenContext` holds a raw `ThreadData*` so `move_gen()` can return a
  value-typed facade without an atomic `shared_ptr` bump on every call.
  `SearchContext` holds `shared_ptr<ThreadData>` (and owns the TT); its trivial
  accessors are header-defined so call sites inline direct field accesses.
  `UtilitiesContext` wraps a `dds::Utilities*`, not `ThreadData`. Treat the
  header-inlined `SearchContext` accessors as a performance contract.
- **Log/stats are build-time variants.** `solver_context` (plain),
  `solver_context_log` (`DDS_UTILITIES_LOG`), and `solver_context_stats`
  (`DDS_UTILITIES_STATS`) compile the same source with different defines; TT
  lifecycle events emit log entries / bump counters only in those variants. Link
  at most one variant into a binary. See [[system-concurrency]] and [[build-system]].

## Key entry points

- `library/src/solver_context/solver_context.hpp` — `SolverContext`,
  `SolverConfig`, `TTKind`, and the `SearchContext` / `MoveGenContext` /
  `UtilitiesContext` facades. Doxygen documents each method.
- `library/src/solver_context/solver_context.cpp` — out-of-line construction,
  TT lifecycle, and reset implementations.
- `library/src/solver_context_adapter.cpp` — `solve_board(SolverContext&, …)`
  adapter that drives the internal solver from a context.
- Build targets: `//library/src/solver_context:{solver_context,solver_context_log,
  solver_context_stats}`.

## Known gaps / non-goals

- Not thread-safe by design — concurrency is achieved with one context per
  worker, coordinated by [[system-concurrency]], not by locking a shared context.
- The context does not persist across processes; the TT is in-memory only.
- The class header still describes itself as "scaffolding … no-behavior-change
  adapter" from the migration; the instance-scoped ownership model above is the
  current, authoritative behaviour. (A stale comment in
  `solver_context_adapter.cpp` still refers to a "ThreadData-attached" shared TT;
  the per-`SearchContext` ownership described here is what the code does.)
