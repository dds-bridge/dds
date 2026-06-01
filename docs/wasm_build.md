# WebAssembly (WASM) Build Guide

This document explains how to build DDS examples for WebAssembly using Bazel.

## Prerequisites

### Bazel

Bazel 7.x or later is required. Install using your package manager or download from:
https://bazel.build/install

The Emscripten SDK (emsdk) does NOT need to be manually installed. Bazel downloads and caches a hermetic Emscripten toolchain when you build a `wasm_cc_binary` target.

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

### Web browser (not yet supported)

There is no checked-in HTML runner yet. To experiment manually, copy the built `.js` and `.wasm` files from `bazel-bin/examples/wasm/` to a static file server and load the `.js` from a page that provides the Emscripten `Module` hooks.

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

## Development notes

- The `__WASM__` preprocessor constant is defined for WASM builds (`CPPVARIABLES.bzl`). It was added to work around platform-specific code paths; revisit whether it can be narrowed or removed as WASM support matures.
- Some threading and platform-specific features are disabled or stubbed when `__WASM__` is set.
- A reusable `cc_library` WASM artifact (not only example binaries) is not yet provided; today only `wasm_cc_binary` example targets are wired up.
- Browser/HTML packaging (`.html` shell, assets) is not yet documented or automated; Node.js is the supported smoke-test runner for now.

## Next steps

To integrate WASM builds into CI/CD:

1. See `.github/workflows/ci_wasm.yml`
2. Store WASM artifacts (`*.js`, `*.wasm`) for download or release as needed

## Related documentation

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Bazel Build System](https://bazel.build/docs)
- [DDS C++ API](c++_interface.md)
- [Build System Overview](BUILD_SYSTEM.md)
