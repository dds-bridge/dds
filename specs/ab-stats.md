---
capability: ab-stats
owners: [library/src]
last-updated: 2026-07-16
related-plans: [write_specs]
---

# Alpha-Beta Statistics

> **Specs vs. doxygen.** The `ABstats` class and its methods are documented inline
> in `ab_stats.hpp`. This spec records what the instrumentation is for, how it is
> gated to zero cost, and where it plugs into the search.

## Purpose

`ABstats` is the alpha-beta search's self-profiler: it counts nodes and where in
the search alpha-beta terminations happen, broken down by position type, side, and
depth. It exists purely for performance tuning and debugging of the solver — it
has no effect on results — and is compiled to nothing in normal builds so it never
costs the hot path.

## Behaviour & invariants

> Method contracts are in the header doxygen; these are the capability-wide facts.

- **Counting is gated to zero cost.** The `AB_COUNT(a, b, c)` macro expands to
  `thrp->ABStats.IncrPos(a, b, c)` only when `DDS_AB_STATS` is defined; otherwise
  it expands to nothing. So call sites in `ab_search.cpp` / `solver_if.cpp` carry
  no overhead unless stats are enabled. This is the central invariant: **stats-off
  builds must remain free of AB counting cost.**
- **Enabled through the build system, not source edits.** The supported switch is
  `--define=ab_stats=true` → the `ab_stats` config setting → `DDS_AB_STATS` in
  `DDS_LOCAL_DEFINES` (see [[build-system]]). Uncommenting `DDS_AB_STATS` in
  `debug.h` or enabling `DDS_DEBUG_ALL` also works (see [[constants-and-debug]]).
- **The accumulator lives on `ThreadData`.** Each solving thread's `ABStats`
  member is incremented via the macro, so counts are per-thread; results are
  written to per-thread `ABstats`-prefixed diagnostic files on teardown.
- **What it measures.** `ABCountType` enumerates the termination sites
  (`AB_TARGET_REACHED`, `AB_DEPTH_ZERO`, `AB_QUICKTRICKS`, `AB_QUICKTRICKS_2ND`,
  `AB_LATERTRICKS`, `AB_MAIN_LOOKUP`, `AB_SIDE_LOOKUP`, `AB_MOVE_LOOP`); counts are
  tracked per side and per depth (up to `DDS_MAXDEPTH = 49`), with cumulative
  variants across solves. `IncrNode` counts generated nodes; `PrintStats` renders
  the breakdown.
- **The `ab_stats` library is always linked, the counting is not.** The
  `//library/src:ab_stats` `cc_library` is a normal dependency of
  [[system-concurrency]]; whether its counters actually run is decided by the
  define, not by linkage. This mirrors the `testable_dds_util_stats` /
  `system_util_stats` stats variants but is a distinct, search-specific flag.

## Key entry points

- `library/src/ab_stats.hpp` — `ABstats`, `ABCountType`, `ABtracker`, and the
  `AB_COUNT` macro. Doxygen documents the class.
- `library/src/ab_stats.cpp` — accumulation and reporting.
- Call sites: `library/src/ab_search.cpp`, `solver_if.cpp` (via `AB_COUNT`);
  storage on `system/thread_data.hpp`.
- Build target / flag: `//library/src:ab_stats`; config setting `ab_stats`
  (`--define=ab_stats=true`).

## Known gaps / non-goals

- Not a public API and not a result-affecting feature — enabling it never changes
  what the solver returns, only what it reports.
- Distinct from the `DDS_UTILITIES_STATS` logging/stats variants in
  [[system-concurrency]] / [[solver-context]]; those track utilities/TT events,
  this tracks alpha-beta search internals.
- Output is offline diagnostic files, not a queryable runtime API.
