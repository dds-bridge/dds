---
capability: wasm-emscripten
owners: [wasm]
last-updated: 2026-07-16
related-plans: [write_specs]
---

# WASM (Emscripten) Build

> **Specs vs. docs.** Build commands, emsdk pinning, and the post-build patch are
> in `docs/wasm_build.md`. This spec records what the WASM build covers, the
> toolchain contract, and the single-thread assumption.

## Purpose

This capability compiles selected example CLIs to WebAssembly so the solver runs
in a browser or Node without a native install. It is the WASM module layer that
the [[web-mvp]] site and the WASM system tests consume. It exists to prove the
core solver builds and runs correctly under Emscripten.

## Behaviour & invariants

> Per-target detail is in the BUILD file and `docs/wasm_build.md`; these are the
> capability-wide facts.

- **A curated subset of examples is ported.** `wasm_cc_binary` wraps three
  [[examples-cli]] binaries: `solve_board_wasm` (← `//examples:solve_board`),
  `analyse_play_bin_wasm` (← `//examples:AnalysePlayBin`), and
  `calc_dd_table_pbn_wasm` (← `//examples:calc_dd_table_pbn`), each emitting a
  `.js` loader + `.wasm`. `all_examples_wasm` groups them.
- **The toolchain is hermetic and transition-driven.** `wasm_cc_binary` applies an
  Emscripten **platform transition** to the underlying `cc_binary` — no
  `--config=wasm` or `.bazelrc` profile is needed. The emsdk toolchain is
  downloaded/cached by Bazel (pinned in `MODULE.bazel`, currently `emsdk 5.0.7`).
  The transition is what makes the `//:build_wasm` config setting
  (`@platforms//cpu:wasm32`) active — see [[build-system]].
- **WASM builds are single-threaded.** The Emscripten build does not use the host
  threading in [[system-concurrency]]; solves run on one thread. Link flags come
  from `WASM_LINKOPTS` ([[build-system]]) — notably an 8 MB stack, because DDS
  search recursion overflows Emscripten's 64 KB default.
- **Correctness is checked two ways.** `calc_dd_table_pbn_test` is a native
  `cc_test` over the same example logic (fast feedback without a JS runtime); the
  `wasm_examples_system_test` py_test actually runs the built
  `calc_dd_table_pbn_wasm` module end to end. The `all` and `wasm_system_tests`
  suites bundle these.
- **A generated helper is patched post-build.** `web/patch_mvp_wasm.py` fixes one
  Emscripten-generated `isFileURI` helper for browser/`file://` safety; if an
  emsdk upgrade moves that line, the regex (and the note in `docs/wasm_build.md`)
  must be updated. Regenerate with `./web/update_wasm.sh` after an emsdk bump.

## Key entry points

- `wasm/BUILD.bazel` — the three `wasm_cc_binary` targets, `all_examples_wasm`,
  `calc_dd_table_pbn_test`, `wasm_examples_system_test`, and the test suites.
- `wasm/tests/test_wasm_examples_system.py` — the end-to-end runner.
- Consumer guide: `docs/wasm_build.md`. Shared link flags: `WASM_LINKOPTS` in
  `wasm_compat.bzl`.

## Known gaps / non-goals

- **Only three examples are ported**, not the full [[examples-cli]] set.
- **No threaded WASM** — single-thread only.
- Browser wiring, the site, and its JS/e2e tests belong to [[web-mvp]]; this
  capability provides the modules, not the page.
