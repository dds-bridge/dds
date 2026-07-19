---
capability: system-concurrency
owners: [system]
last-updated: 2026-07-18
---

# System & Concurrency

> **Specs vs. doxygen.** Per-class/method contracts (`System`, `Memory`,
> `Scheduler`, `parallel_all_boards_n`, `Utilities`) are documented inline in the
> `system/` headers. This spec records the cross-cutting concurrency and
> feature-variant facts that no single symbol owns.

## Purpose

This capability is the platform and concurrency layer: it decides how many worker
threads to use, distributes boards across them, owns per-thread scratch memory,
provides file/timer utilities, and carries the logging/stats plumbing
(`Utilities`). It exists to keep system- and threading-dependent behaviour in one
place so the search and API layers stay portable. It is internal — not part of
[dds-public-api](dds-public-api.md).

## Behaviour & invariants

> Signatures live in the headers' doxygen; these are the whole-layer guarantees.

- **Worker count is derived, then clamped.** `resolve_worker_count(max_threads,
  count)`: `max_threads <= 0` means "auto" (hardware concurrency); the result is
  clamped to `[1, count]` when `count > 0`, and to `1` when `count <= 0`. Every
  parallel entry point derives its thread count this way — a request for more
  threads than there are boards never spawns idle workers. Guarded by
  `//library/tests/system:worker_count_test`.
- **Board parallelism is shared-queue dispatch and fail-fast.** 
  `parallel_all_boards_n(count, worker_cap, process_board, order = nullptr)`
  hands out indices via a shared atomic counter (`fetch_add`) — not classic
  per-worker steal queues, despite the header's "work-stealing" wording. Optional
  `order` must be a permutation of `[0, count)` (correct size, in-range, no
  duplicates) to control dispatch priority; a malformed `order` **falls back to
  index order** rather than rejecting. Each `process_board(worker_id, bno)` must
  return `RETURN_NO_FAULT` (1) on success. The function returns the **first
  non-success code** encountered (or `RETURN_NO_FAULT`). Guarded by
  `concurrency_validation_test` and `parallel_boards_test`.
- **Result equivalence across thread counts is an invariant.** Solving the same
  boards single-threaded and multi-threaded must produce identical results;
  `max_threads_equivalence_test` and `context_equivalence_test*` guard this. Thread
  count is a performance knob only, never a correctness input.
- **Per-thread state is context-owned, not centrally pooled.** `Memory` no longer
  holds a central vector of `ThreadData`; each `ThreadData` is owned by its
  [solver-context](solver-context.md) and passed down. `Memory` keeps only lightweight per-thread
  size records for diagnostics. This is the ownership-migration end state.
- **Scheduler timing is build-gated.** The `Scheduler` block/thread timers compile
  to no-ops unless `DDS_SCHEDULER` is defined (the `scheduler` config setting,
  applied via `DDS_SCHEDULER_DEFINE` — see [build-system](build-system.md)). Off by default,
  zero cost.
- **Feature variants share one source set.** `system`, `system_util_log`
  (`DDS_UTILITIES_LOG`), and `system_util_stats` (`DDS_UTILITIES_STATS`) compile
  the same `.cpp`/`.hpp` glob with different defines; the log/stats variants make
  `Utilities` accumulate a log buffer / bump counters. **Link at most one variant
  into a binary.** The [solver-context](solver-context.md) variants pair with these. Guarded by the
  `utilities_log*` / `utilities_stats*` / `utilities_feature_flags*` tests.
- **`Utilities` is the logging/stats sink** surfaced through
  `SolverContext::UtilitiesContext` (`log_append`, `log_buffer`, `log_clear`);
  in the plain `system` build these are cheap no-ops.

## Key entry points

- `library/src/system/system.{hpp,cpp}` — `System`: threading backend selection,
  thread/memory/resource management.
- `library/src/system/parallel_boards.{hpp,cpp}` — `resolve_worker_count`,
  `parallel_all_boards_n`.
- `library/src/system/scheduler.{hpp,cpp}` — board scheduling and (gated) timing.
- `library/src/system/memory.{hpp,cpp}` — per-thread `Memory` manager.
- `library/src/system/file.{hpp,cpp}` and `system/util/utilities.hpp` — file and
  logging/stats utilities.
- Build targets: `//library/src/system:{system,system_util_log,system_util_stats}`.
- Guarded by `//library/tests/system/...` (worker-count, equivalence, concurrency,
  TT-facade, utilities log/stats).

## Known gaps / non-goals

- **WASM builds are single-threaded** — the concurrency here assumes hosted
  threads; the Emscripten build does not use them. See [wasm-emscripten](wasm-emscripten.md).
- This layer does not decide TT sizing or search policy; it schedules work and
  owns scratch memory. TT ownership/config belongs to [solver-context](solver-context.md) /
  [transposition-table](transposition-table.md).
- The log/stats variants are for tests and diagnostics; production binaries link
  the plain `system` variant.
