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

## Available WASM targets

Rules in `examples/wasm/BUILD.bazel` wrap native examples in `examples/`:

- `solve_board_wasm` — solves a single board
- `analyse_play_bin_wasm` — analyze play from binary format

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

## Related documentation

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Bazel Build System](https://bazel.build/docs)
- [DDS C++ API](c++_interface.md)
- [Build System Overview](BUILD_SYSTEM.md)
