# WebAssembly (WASM) Build Guide

This document explains how to build DDS examples for WebAssembly using Bazel.

## Prerequisites

### Bazel

Bazel 7.x or later is required. Install using your package manager or download from:
https://bazel.build/install

The Emscripten SDK (emsdk) does NOT need to be manually installed. Bazel downloads and caches a hermetic Emscripten toolchain when you build a `wasm_cc_binary` target.

### Emscripten / emsdk version

WASM builds use the Bazel Central Registry package pinned in `MODULE.bazel`:

- `bazel_dep(name = "emsdk", version = "5.0.7")` (Emscripten 3.1.x toolchain)

If you upgrade `emsdk`, rebuild WASM targets and the web MVP (`./web/update_wasm.sh`). The post-build script `web/patch_mvp_wasm.py` patches one generated `isFileURI` helper for browser/file URL safety; if Emscripten changes that line, update the regex in that script and this note.

## Building WASM Examples

WASM targets use `wasm_cc_binary`, which applies an Emscripten **platform transition** to the underlying `cc_binary`. You do **not** need `--config=wasm` or any other `.bazelrc` profile.

### Build all WASM examples

```bash
bazel build //examples/wasm:all_examples_wasm
```

The alias `//examples:all_examples_wasm` points at the same filegroup.

### Build a specific example

```bash
bazel build //examples/wasm:solve_board_wasm
```

### Output files

Outputs are under `bazel-bin/examples/wasm/`:

- `solve_board.js` / `solve_board.wasm`
- `AnalysePlayBin.js` / `AnalysePlayBin.wasm`
- `calc_dd_table_pbn.js` / `calc_dd_table_pbn.wasm`

## Available WASM targets

Rules in `examples/wasm/BUILD.bazel` wrap native examples in `examples/`:

- `solve_board_wasm` — solves a single board
- `analyse_play_bin_wasm` — analyze play from binary format
- `calc_dd_table_pbn_wasm` — double-dummy table from PBN

## How it works

1. **`wasm_cc_binary`** (from `@emsdk`) transitions its `cc_target` to `@emsdk//:platform_wasm` and sets `--cpu=wasm`.
2. **`//:build_wasm`** in the root `BUILD.bazel` matches that CPU for `select()` in `CPPVARIABLES.bzl` and example link flags.
3. **`wasm_compat.bzl`** — shared Emscripten link flags (`WASM_LINKOPTS`) on the WASM-capable `cc_binary` targets.

Native builds (`bazel build //...`, `bazel test //library/tests/...`, Python bindings) are unchanged and use the host LLVM toolchain.

## Running WASM examples

### Node.js

```bash
node bazel-bin/examples/wasm/solve_board.js
```

### Web browser (DDS MVP)

The `web/` demo calls `CalcDDtablePBN` in the browser via `//web:dds_mvp_wasm`:

```bash
./web/update_wasm.sh
python3 -m http.server 8080 --directory web
# open http://localhost:8080/dds_mvp.html
```

The MVP loads wasm from `dds_mvp_wasm_bin.js` (base64, no network fetch), so `file://` and HTTP both work. Run `./web/update_wasm.sh` to refresh `dds_mvp_wasm.{js,wasm,bin.js}` (includes a small post-process step for Emscripten `isFileURI`; see **Emscripten / emsdk version** above).

For other experiments, copy built `.js` / `.wasm` files from `bazel-bin/examples/wasm/` to any static file server.

## Compilation flags

| Flag | Purpose |
|------|---------|
| `-O3` | Aggressive optimization |
| `-flto` | Link-time optimization |
| `-fexceptions` | Enable C++ exceptions |
| `-D__WASM__` | Preprocessor constant for WASM builds |
| `-sWASM=1` | Emscripten WASM output (link flag) |
| `-sALLOW_MEMORY_GROWTH=1` | Allow heap growth at runtime |
| `-sINITIAL_MEMORY=268435456` | 256MB initial memory |
| `-sSTACK_SIZE=8388608` | 8MB stack (default 64KB is too small for DDS search) |

## C++ standard

WASM example binaries are built as C++20. The Emscripten toolchain (via `wasm_cc_binary`) compiles the transitioned `cc_target`; project flags for that configuration come from `CPPVARIABLES.bzl` when `//:build_wasm` matches (`--cpu=wasm` set by the transition).

Host-side Bazel actions still use the normal platform flags in `.bazelrc`:

```
build:macos --cxxopt=-std=c++20
build:linux --cxxopt=-std=c++20
```

There is no separate `build:wasm` profile in `.bazelrc`; WASM builds are selected by targeting `//examples/wasm:*`.

## Tests

Unit and system tests (Node.js required for system tests; skipped if `node` is not on `PATH`):

```bash
bazel test //web:web_tests //web:web_system_tests //web:web_e2e_tests
bazel test //examples/wasm:all
```

- **`//web:dds_mvp_wasm_system_test`** — builds `//web:dds_mvp_wasm`, runs `patch_mvp_wasm` / `gen_wasm_bin_js` / `verify_wasm_js`, then calls `dds_mvp_calc_table` via Node (`web/tests/dds_mvp_wasm_node.mjs`).
- **`//web:dds_mvp_e2e_test`** — Playwright tests for `dds_mvp.html` over `file://` and HTTP (part-score deal table, validation error). Requires Node, network (Chromium download on first run), and `tags = ["no-sandbox"]`.
- **`//examples/wasm:wasm_examples_system_test`** — runs `calc_dd_table_pbn.js` under Node and checks for `OK` on all three example hands.

The MVP link flags include `-sENVIRONMENT=web,node` so the same `.js` / `.wasm` artifacts work in the browser and in Node system tests.

## Development notes

- The `__WASM__` preprocessor constant is defined for WASM builds (`CPPVARIABLES.bzl`). It was added to work around platform-specific code paths; revisit whether it can be narrowed or removed as WASM support matures.
- Some threading and platform-specific features are disabled or stubbed when `__WASM__` is set.
- A reusable `cc_library` WASM artifact (not only example binaries) is not yet provided; today only `wasm_cc_binary` example targets are wired up.
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
