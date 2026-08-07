# WebAssembly (WASM) Build Guide

This document explains how to build DDS examples for WebAssembly using Bazel.

## Prerequisites

### Bazel

The Bazel version is pinned in [`.bazelversion`](../.bazelversion). Use
[bazelisk](https://github.com/bazelbuild/bazelisk), which reads that file and
fetches the matching Bazel automatically:
https://github.com/bazelbuild/bazelisk

The Emscripten SDK (emsdk) does NOT need to be manually installed. Bazel downloads and caches a hermetic Emscripten toolchain when you build a `wasm_cc_binary` target.

### Emscripten / emsdk version

WASM builds use the Bazel Central Registry package pinned in `MODULE.bazel`:

- `bazel_dep(name = "emsdk", version = "5.0.7")` (Emscripten 5.0.x toolchain)

If you upgrade `emsdk`, rebuild WASM targets and the web MVP (`./web/update_wasm.sh`). The post-build script `web/patch_mvp_wasm.py` patches one generated `isFileURI` helper for browser/file URL safety; if Emscripten changes that line, update the regex in that script and this note.

## Building WASM Examples

WASM targets use `wasm_cc_binary`, which applies an Emscripten **platform transition** to the underlying `cc_binary`. You do **not** need `--config=wasm` or any other `.bazelrc` profile.

### Build all WASM examples

```bash
bazelisk build //wasm:all_examples_wasm
```

The alias `//examples:all_examples_wasm` points at the same filegroup.

### Build a specific example

```bash
bazelisk build //wasm:solve_board_wasm
```

### Output files

Outputs are under `bazel-bin/wasm/`:

- `solve_board.js` / `solve_board.wasm`
- `AnalysePlayBin.js` / `AnalysePlayBin.wasm`
- `calc_dd_table_pbn.js` / `calc_dd_table_pbn.wasm`
- `dtest.js` / `dtest.wasm` (from `//wasm:dtest_wasm`)

## Available WASM targets

Rules in `wasm/BUILD.bazel` wrap native binaries:

- `solve_board_wasm` — solves a single board (`//examples:solve_board`)
- `analyse_play_bin_wasm` — analyze play from binary format
- `calc_dd_table_pbn_wasm` — double-dummy table from PBN
- `dtest_wasm` — the `dtest` hand-list harness for Node (`//library/tests:dtest`)

`dtest_wasm` is linked with `NODERAWFS` so Node can pass a real host path to
`-f` (for example `hands/list1.txt`). Multi-threading is enabled (`-pthread`);
use `-n N` for N workers (or omit / `0` for auto).

```bash
bazelisk build //wasm:dtest_wasm
node bazel-bin/wasm/dtest.js -f hands/list2.txt -s solve -n 2

# Or via Bazel (forwards args after -- to dtest; cwd is your shell's directory):
bazelisk run //wasm:run_dtest_wasm -- -f hands/list2.txt -s solve -n 2
```
## How it works

1. **`wasm_cc_binary`** (from `@emsdk`) transitions its `cc_target` to `@emsdk//:platform_wasm` and sets `--cpu=wasm`.
2. **`//:build_wasm`** in the root `BUILD.bazel` matches `@platforms//cpu:wasm32` for `select()` in `CPPVARIABLES.bzl` and example link flags.
3. **`wasm_compat.bzl`** — shared Emscripten link flags (`WASM_LINKOPTS`) on the WASM-capable `cc_binary` targets.

Native builds (`bazelisk build //...`, `bazelisk test //library/tests/...`, Python bindings) are unchanged and use the host LLVM toolchain.

## Running WASM examples

### Node.js

```bash
node bazel-bin/wasm/solve_board.js
```

### Web browser (DDS MVP)

The `web/` demo calls `CalcDDtablePBN` in the browser via `//web:dds_mvp_wasm`:

```bash
./web/update_wasm.sh
python3 web/serve_mvp.py
# open http://127.0.0.1:8080/dds_mvp.html
```

Pthread WASM needs `SharedArrayBuffer`, which requires cross-origin isolation
(`Cross-Origin-Opener-Policy: same-origin` and
`Cross-Origin-Embedder-Policy: require-corp`). `web/serve_mvp.py` sets those
headers; plain `python3 -m http.server` does not, so solving will fail there.
Any other host must send the same response headers. Without them the page can
load but solving reports that `SharedArrayBuffer` is unavailable.
`file://` is still fine for UI-only browsing; run a solve over the isolated HTTP
server.

The MVP loads wasm from `dds_mvp_wasm_bin.js` (base64, no network fetch). Run
`./web/update_wasm.sh` to refresh `dds_mvp_wasm.{js,wasm,bin.js}` (includes a
small post-process step for Emscripten `isFileURI`; see **Emscripten / emsdk
version** above).

`bazelisk clean` does not delete those copied files under `web/` (they live outside `bazel-out`). Use either:

```bash
./clean.sh              # bazelisk clean + remove web/dds_mvp_wasm.*
./clean.sh --expunge
./web/clean_wasm.sh     # web artifacts only
```

For other experiments, copy built `.js` / `.wasm` files from `bazel-bin/wasm/` to any static file server.

## Compilation flags

| Flag | Purpose |
|------|---------|
| `-O3` | Aggressive optimization |
| `-flto` | Link-time optimization at compile time only (see note below) |
| `-fwasm-exceptions` | Native WebAssembly exception handling (use together with `-fexceptions`; replaces the slower JS-trampoline EH lowering used when linking with `-fexceptions` alone) |
| `-pthread` / `USE_PTHREADS` | Enable SharedArrayBuffer atomics and `std::thread` (via `wasm_cc_binary(threads = "emscripten")`) |
| `-sWASM=1` | Emscripten WASM output (link flag) |
| `-sALLOW_MEMORY_GROWTH=1` | Allow heap growth at runtime (with `-pthread`; warning silenced via `-Wno-pthreads-mem-growth`) |
| `-Wno-pthreads-mem-growth` | Keep growth+pthreads without the emcc advisory on slower JS paths after grow |
| `-sINITIAL_MEMORY=268435456` | 256MB initial memory |
| `-sSTACK_SIZE=8388608` | 8MB stack (default 64KB is too small for DDS search) |
| `-sPTHREAD_POOL_SIZE=8` | Pre-create a modest pthread worker pool (more threads allocate on demand; Node `dtest` shuts the pool down cleanly before exit) |

`-flto` is applied at compile time (`DDS_CPPOPTS`) but deliberately **not** at
link time. Passing `-flto` to the wasm link step was tried and reverted: the
`emsdk 5.0.7` package pinned in `MODULE.bazel` ships a frozen, pre-built cache
containing only non-LTO system libraries, and requesting link-time LTO makes
Emscripten require LTO-bitcode variants of core sysroot libraries (e.g.
`libprintf_long_double`) that the frozen cache can't build on demand inside
Bazel's hermetic sandbox. Revisit only if the emsdk packaging changes to ship
a populated LTO cache.

## C++ standard

WASM example binaries are built as C++20. The Emscripten toolchain (via `wasm_cc_binary`) compiles the transitioned `cc_target`; project flags for that configuration come from `CPPVARIABLES.bzl` when `//:build_wasm` matches (the Emscripten platform transition sets `wasm32`).

Host-side Bazel actions still use the normal platform flags in `.bazelrc`:

```
build:macos --cxxopt=-std=c++20
build:linux --cxxopt=-std=c++20
```

There is no separate `build:wasm` profile in `.bazelrc`; WASM builds are selected by targeting `//wasm:*`.

## Tests

Unit and system tests (Node.js required for system tests; skipped if `node` is not on `PATH`):

```bash
bazelisk test //web:web_tests //web:web_system_tests //web:web_e2e_tests
bazelisk test //wasm:all
```

`bazelisk test //...` skips targets tagged `e2e` by default (see `.bazelrc`). Run Playwright tests explicitly, e.g. `bazelisk test //web:web_e2e_tests` or `bazelisk test --test_tag_filters=e2e //web:dds_mvp_e2e_test`. To run all tests, including the Playwright tests: `bazelisk test --test_tag_filters= //...`. The WASM CI workflow (`.github/workflows/ci_wasm.yml`) passes empty `--test_tag_filters=` so `//web:web_system_tests` includes Playwright.

- **`//web:dds_mvp_wasm_system_test`** — builds `//web:dds_mvp_wasm`, runs `patch_mvp_wasm` / `gen_wasm_bin_js` / `verify_wasm_js`, then calls `dds_mvp_calc_table` via Node (`web/tests/dds_mvp_wasm_node.mjs`).
- **`//web:dds_mvp_e2e_test`** — Playwright tests for `dds_mvp.html` over `file://` (UI) and isolated HTTP (part-score solve, COOP/COEP). Requires Node, network (Chromium download on first run), and `tags = ["no-sandbox"]`.
- **`//wasm:wasm_examples_system_test`** — runs `calc_dd_table_pbn.js` under Node (expects `OK` on all three example hands) and `dtest.js` on `hands/list1.txt` (`-s solve -n 1`) plus `hands/list2.txt` (`-n 2`).

The MVP link flags include `-sENVIRONMENT=web,worker,node` so the same `.js` / `.wasm` artifacts work in the browser (with pthread workers) and in Node system tests.

## Development notes

- A reusable `cc_library` WASM artifact (not only CLI binaries) is not yet provided; today `wasm_cc_binary` wraps selected examples plus `dtest`.
- The browser MVP lives under `web/`; see **Web browser (DDS MVP)** above and `//web:web_system_tests`.

## Next steps

To integrate WASM builds into CI/CD:

1. See `.github/workflows/ci_wasm.yml`
2. Store WASM artifacts (`*.js`, `*.wasm`) for download or release as needed

## Related documentation

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Bazel Build System](https://bazel.build/docs)
- [DDS C++ API](c++_interface.md)
- [Build System Overview](BUILD_SYSTEM.md)
