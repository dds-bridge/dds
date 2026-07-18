---
capability: dds-public-api
owners: [api]
last-updated: 2026-07-18
related-plans: []
---

# DDS Public API

> **Specs vs. doxygen / docs.** Per-function signatures, parameters, and status
> codes live in the header doxygen and in `docs/c++_interface.md` /
> `docs/legacy_c_api.md`. This spec records the *shape* of the public surface —
> its layers, what is stable, and the cross-cutting contracts every binding relies
> on — not a function reference.

## Purpose

This is the public entry surface of the solver: the symbols every binding
(Java/FFM, .NET, Python, WASM) and every example CLI ultimately calls. It exists
in three deliberately distinct layers so that both legacy consumers and modern
context-aware consumers are served from one library without duplicating the
solver. Everything below the API (search, TT, scheduler) is private; this
capability defines what crosses the boundary and promises to stay stable.

## Behaviour & invariants

> The exact per-function contracts are in doxygen. These are the whole-surface
> facts.

- **Three layers, one library:**
  1. **Flat legacy C API** — `dll.h` (`SolveBoard`, `SolveBoardPBN`,
     `CalcDDtable(PBN/N)`, `CalcAllTables*`, `SolveAllBoards*`, `CalcPar`,
     `CalcParPBN`, `Par`, `DealerPar*`, `AnalysePlay*`, `SetMaxThreads`,
     `SetResources`, `FreeMemory`, `GetDDSInfo`, `ErrorMessage`, …). Declared
     `EXTERN_C DLLEXPORT auto STDCALL … -> int`; this is the historical
     Haglund/Hein ABI kept for backward compatibility.
  2. **Modern context C++ API** — `dds_api.hpp`: context-handle entry points
     (`dds_solve_board`, `dds_calc_dd_table` / `_pbn`, `dds_calc_par`, lifecycle,
     TT controls, and related helpers). These take an explicit `DDS_SOLVER_CTX`
     (= `SolverContext*`) and use C++ types (`const Deal&`, `SolverConfig`,
     `TTKind`). See [[solver-context]] and the header for the full set.
  3. **Pure-C ABI shim** — `dds_c_api.h` (`dds_c_*`). Pointer-only, POD-only,
     opaque `void*` handle (`DDS_C_SOLVER_CTX`); no C++ types cross the boundary.
     It forwards to layer 2 and is the surface non-C++ languages bind against.
     Today the shim is a thin subset (`dds_c_solve_board`, `dds_c_calc_dd_table`,
     `dds_c_calc_par` plus context lifecycle) — **no `*PBN` twins and no TT
     configure** in the shim.
- **The shim is the stable binding surface, not the C++ API.** Java (FFM), .NET,
  and ctypes bind to the `dds_c_*` symbols. The header is C-ABI but not
  C-includable (it pulls in `dll.h`, which uses C++ trailing-return syntax) — bind
  to the compiled symbols or parse in C++ mode (jextract).
- **Handles are single-threaded.** One `DDS_SOLVER_CTX` / `DDS_C_SOLVER_CTX` per
  thread; the handle owns per-context solver state and its transposition table.
  Create → use → destroy. The legacy flat API manages global/threaded state via
  `SetMaxThreads`/`SetResources`/`FreeMemory` instead.
- **Integer status returns.** Solver entry points return `RETURN_*` status codes
  (success is positive/`RETURN_NO_FAULT`); `ErrorMessage` maps a code to text.
- **PBN and binary variants are paired on the legacy / modern C++ layers.** Most
  flat `dll.h` and many `dds_api.hpp` entry points have a `*PBN` twin; both
  compute identical results from the same deal. That pairing does **not** extend
  to the C shim.
- **The pinned binding export set is `dll.h` + `dds_c_api.h`.** On Linux/macOS
  the JNI shared library exports are constrained by `jni/version_script.lds` /
  `exported_symbols.lds` and checked by the export-set test. That is the *stable
  binding ABI*, not every `DLLEXPORT` symbol in the tree: `dds_api.hpp` also
  marks modern `dds_*` symbols `DLLEXPORT`, and on Windows (no `.lds`) the DLL
  exports that broader `DLLEXPORT` set. Details in [[jni-ffm-binding]].

## Key entry points

- `library/src/api/dll.h` — flat legacy C ABI. Doxygen: the `dll.h` page;
  narrative in `docs/legacy_c_api.md`.
- `library/src/api/dds_api.hpp` — modern context C++ API. Narrative in
  `docs/c++_interface.md`; migration in `docs/api_migration.md`.
- `library/src/api/dds_c_api.h` — pure-C ABI shim (the binding surface).
- `library/src/api/{solve_board,calc_dd_table,calc_par}.hpp`, `PBN.h`,
  `portab.h`, `dds.h` — supporting public headers (`api_definitions`).
- Build targets: `//library/src/api:dds_c_api`, `:api_definitions`, `//:dds`
  (façade), `//:testable_dds`.

## Known gaps / non-goals

- The API does not expose the private search/TT internals; consumers get results,
  not solver state (beyond the opaque handle).
- The flat legacy API is frozen for compatibility — new functionality is added on
  the modern context API and its shim, not by extending `dll.h` semantics.
- Per-function parameter/return documentation is intentionally *not* duplicated
  here; it lives in doxygen and `docs/`.
