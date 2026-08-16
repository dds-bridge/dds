# DDS 3.1.0 — Release Notes (draft)

DDS 3.1.0 builds on the modernised 3.0 core. The headline changes for anyone
consuming the solver are new **.NET** and **Java** bindings, a much larger
**Python** surface, batch solving that is parallel by default, and a set of
double-dummy correctness fixes. Existing 3.0 code continues to work unchanged.

## Highlights

### New language bindings

- **.NET (`DDS_Core`)** — a type-safe .NET 8+ wrapper covering both the legacy C
  API (`SolveBoard`, `CalcDDtable`, `Par`, `AnalysePlay`, …) and the modern
  `SolverContext` API. One AnyCPU assembly runs on macOS, Linux, and Windows;
  the native library is located automatically or via `DDS_LIBRARY_PATH`.
  See [dotnet_interface.md](../dotnet_interface.md).
- **Java (FFM / Project Panama)** — call DDS from JDK 22+ with no hand-written
  JNI glue. `//jni:dds_shared` produces one self-contained native library per
  OS, packaged with the native binary inside the jar.
  See [jni_interface.md](../jni_interface.md).
- **A pure-C ABI shim (`dds_c_api.h`)** underpins both. It takes only pointers
  and plain-old-data, so FFM, P/Invoke, and `ctypes` all bind against the same
  stable surface instead of each inventing its own.

### Python

`dds3` now exposes the rest of the solver, not just the single-board entry
points:

- `analyse_play_pbn`, `analyse_all_plays_pbn` — trick-by-trick play analysis.
- `solve_all_boards_pbn`, `solve_all_boards_bin` — batch solving, parallelised
  across hardware threads inside the library.
- `dealer_par` — par contracts from the dealer's perspective.
- `initialise_static_memory` (replaces the deprecated `set_max_threads`).

All of these release the GIL around the native call and validate their inputs.
Batch entry points accept an optional `max_threads`.

### Performance

- **Batch work is parallel inside the library.** `SolveAllBoards*` and
  `CalcAllTables*` share one work-stealing dispatcher backed by a persistent
  worker pool, each worker holding its own reusable `SolverContext`. Hardest
  boards are dispatched first to shorten the tail.
- **Search efficiency is back at 2.9 parity.** 3.0 carried move-ordering
  regressions from the refactor — signed/unsigned truncation in the heuristic
  and quick-tricks paths, and a trump-void ordering bug — that made the search
  explore materially more nodes than 2.9. Those are fixed; results are
  unchanged, the search just does less work to reach them.
- **Warm solver context across calls.** Reusing a `SolverContext` between
  related solves keeps its per-thread search state and its transposition table
  alive. The TT survives as long as the next call is on the same or a similar
  deal with the same trump; a genuinely new deal or a new trump resets it, as in
  2.9. This is now the documented pattern and is used internally by the batch
  paths. `clear_tt()` remains the way to discard the table explicitly — it
  disposes the instance and keeps only the configuration, which rebuilds an
  empty table on next use.
- Thin LTO on macOS builds, inlined hot accessors, and native WASM exception
  handling.

### Correctness fixes

- **`AnalysePlay` under-counted tricks** (#156): each card was analysed against
  a cold transposition table, so the hint-bounded search settled on the wrong
  bound. The play path now reuses the caller's context, matching `SolveBoard`.
- **Move ordering and pruning corruption** from signed→unsigned casts (see
  above) — search behaviour only, but it cost significant time.
- **Heap-use-after-free in `clear_tt`** and a null dereference in
  `TransTableS::reset_memory` after memory release.
- **Par output now names the declaring seat** when successive par contracts
  differ, in both the C++ and .NET paths.
- Worker exceptions in parallel board solving are propagated rather than
  terminating the process.

### New public C API

Thread-count-aware and sequential variants were added alongside the existing
entry points, so callers can size or opt out of the library's parallelism:

`CalcDDtableN`, `CalcDDtablePBNN`, `CalcAllTablesN`, `CalcAllTablesPBNN`,
`CalcAllTablesX`, `CalcAllTablesPBNX`, `SolveAllBoardsN`, `SolveAllBoardsBinN`,
`SolveAllBoardsSeq`, `SolveAllBoardsBinSeq`.

`SetMaxThreads` is deprecated in favour of `InitializeStaticMemory`; the old
name still works and no longer influences batch parallelism.

### Platforms and build

- **Windows/MSVC** is a first-class target again: Visual Studio project files
  under `solution/`, plus Windows CI for the native build and the .NET bindings.
- **WebAssembly** builds hermetically via Bazel's Emscripten toolchain — no
  manual `emsdk` install — with multithreading, a heap-budgeted worker cap, and
  a browser demo (`web/`) that runs the solver entirely client-side.
- CI now runs Linux, macOS, Windows, and WASM, under ASan, TSan, UBSan, and
  MSan.
- Use `bazelisk`; the Bazel version is pinned in `.bazelversion` and the
  committed `MODULE.bazel.lock` expects it.

### Tooling

- `//benchmarks:dds_replay` replays a recorded real-world DDS workload
  (154,370 solved deals), timing it and verifying every answer — a benchmark and
  a regression test in one. `//benchmarks:warm_tt_benchmark` isolates the
  transposition-table reuse win.
- `dd_table_for_deal` CLI in Python, C++, and .NET; `create_list_for_dtest` for
  generating hand lists; regenerated, de-duplicated `hands/` lists.
- Capability specs under `specs/` document the public API, bindings, threading,
  and build system.

## Compatibility

No breaking changes for 3.0 consumers. The only deprecation is `SetMaxThreads`,
which remains available as an alias.

## Contributors

@BonyJordan, @BSalita, @ed2k, @jdh8, @mortensp, @tameware, @ThorvaldAagaard,
@tzimnoch, @wopdevries, @zzcgumn
