---
capability: wasm-emscripten
owners: [wasm]
last-updated: 2026-07-19
---

# WASM (Emscripten) Build

> **Specs vs. docs.** Build commands and emsdk pinning live in
> `docs/wasm_build.md`. This spec records what the WASM build covers, the
> toolchain contract, and the single-thread assumption.

## Purpose

This capability compiles selected example CLIs to WebAssembly so the solver runs
in a browser or Node without a native install. It is the WASM module layer that
the [web-mvp](web-mvp.md) site and the WASM system tests consume. It exists to prove the
core solver builds and runs correctly under Emscripten.

## Behaviour & invariants

> Per-target detail is in the BUILD file and `docs/wasm_build.md`; these are the
> capability-wide facts.

- **A curated subset of CLIs is ported.** `wasm_cc_binary` wraps three
  [examples-cli](examples-cli.md) binaries: `solve_board_wasm` (← `//examples:solve_board`),
  `analyse_play_bin_wasm` (← `//examples:AnalysePlayBin`), and
  `calc_dd_table_pbn_wasm` (← `//examples:calc_dd_table_pbn`), each emitting a
  `.js` loader + `.wasm`. `all_examples_wasm` groups them. Separately,
  `dtest_wasm` (← `//library/tests:dtest`) ports the hand-list test harness for
  Node (host file access via `NODERAWFS`); it is not part of `all_examples_wasm`.
  `//wasm:run_dtest_wasm` is a `py_binary` that runs that module under Node
  (`bazel run //wasm:run_dtest_wasm -- …`).
- **The toolchain is hermetic and transition-driven.** `wasm_cc_binary` applies an
  Emscripten **platform transition** to the underlying `cc_binary` — no
  `--config=wasm` or `.bazelrc` profile is needed. The emsdk toolchain is
  downloaded/cached by Bazel (pinned in `MODULE.bazel`, currently `emsdk 5.0.7`).
  The transition is what makes the `//:build_wasm` config setting
  (`@platforms//cpu:wasm32`) active — see [build-system](build-system.md).
- **WASM builds are single-threaded.** The Emscripten build does not use the host
  threading in [system-concurrency](system-concurrency.md); solves run on one thread. Link flags come
  from `WASM_LINKOPTS` ([build-system](build-system.md)) — notably an 8 MB stack, because DDS
  search recursion overflows Emscripten's 64 KB default. Example binaries also
  attach those flags via `EXAMPLES_LINKOPTS_WASM` in `examples/BUILD.bazel`.
  `dtest` adds Node-oriented flags (`ENVIRONMENT=node`, `NODERAWFS`,
  `EXIT_RUNTIME`) under the same `build_wasm` select.
- **Correctness is checked two ways.** `calc_dd_table_pbn_test` is a native
  `cc_test` over the same example logic (fast feedback without a JS runtime); the
  `wasm_examples_system_test` py_test runs `calc_dd_table_pbn_wasm` and
  `dtest_wasm` under Node end to end. `run_dtest_wasm_test` covers the Node
  runner helpers. The `all` and `wasm_system_tests`
  suites bundle these.

## Key entry points

- `wasm/BUILD.bazel` — the example `wasm_cc_binary` targets, `dtest_wasm`,
  `run_dtest_wasm`, `all_examples_wasm`, `calc_dd_table_pbn_test`,
  `wasm_examples_system_test`, and the test suites.
- `wasm/run_dtest_wasm.py` — `bazel run` entry that invokes `dtest.js` via Node.
- `wasm/tests/test_wasm_examples_system.py` — the end-to-end runner.
- Consumer guide: `docs/wasm_build.md`. Shared link flags: `WASM_LINKOPTS` in
  `wasm_compat.bzl`.

## Known gaps / non-goals

- **Only three examples are ported**, not the full [examples-cli](examples-cli.md) set.
  `dtest_wasm` is an additional harness port, not an example CLI.
- **No threaded WASM** — single-thread only.
- **No link-time LTO.** `-flto` applies at WASM compile time only
  ([build-system](build-system.md)); enabling it at link time was tried and
  reverted because the `emsdk 5.0.7` toolchain pinned in `MODULE.bazel` ships a
  frozen, pre-built cache containing only non-LTO system libraries. Link-time
  LTO requires LTO-bitcode variants of core sysroot libraries (e.g.
  `libprintf_long_double`) that the frozen cache cannot build on demand inside
  Bazel's hermetic sandbox. Revisit only if the emsdk packaging ships a
  populated LTO cache.
- Browser wiring, the site, MVP post-build JS patches (`web/patch_mvp_wasm.py`),
  and JS/e2e tests belong to [web-mvp](web-mvp.md); this capability provides the example
  modules, not the page.
