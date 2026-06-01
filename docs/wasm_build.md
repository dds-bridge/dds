# WebAssembly (WASM) Build Guide

This document explains how to build the DDS library and examples for WebAssembly using Bazel.

## Prerequisites

### Bazel

Bazel 7.x or later is required. Install using your package manager or download from:
https://bazel.build/install

The Emscripten SDK (emsdk) does NOT need to be manually installed. Bazel will automatically download, configure, and cache the appropriate hermetic Emscripten toolchain for your host platform as part of the build process.

## Building WASM Examples

### Build all WASM-compatible targets

Native-only targets (Python bindings, C++ tests, most examples) are skipped automatically:

```bash
bazel build --config=wasm //...
```

### Build WASM examples only

```bash
bazel build --config=wasm //examples/wasm:all_examples_wasm
```

The alias `//examples:all_examples_wasm` points at the same filegroup.

### Build a specific example

```bash
bazel build --config=wasm //examples/wasm:solve_board_wasm
```

### Output Files

Output files are under `bazel-bin/examples/wasm/` (wasm_cc_binary output layout):

- `solve_board.js` / `solve_board.wasm`
- `AnalysePlayBin.js` / `AnalysePlayBin.wasm`

## Available WASM Targets

WASM rules live in `examples/wasm/BUILD.bazel` and wrap native `cc_binary` targets in `examples/`:

- `solve_board_wasm` — solves a single board
- `analyse_play_bin_wasm` — analyze play from binary format

## WASM Build Configuration

1. **`BUILD.bazel`** — `//:build_wasm` matches `--cpu=wasm` from the `wasm` config
2. **`wasm_compat.bzl`** — shared Emscripten link flags (`WASM_LINKOPTS`)
3. **`CPPVARIABLES.bzl`** — WASM compiler flags and `__WASM__` define
4. **`.bazelrc`** — `wasm` config (platform, `--build_tag_filters=-native-only`)

Targets tagged `native-only` are excluded from `bazel build --config=wasm //...` (most examples, Doxygen). The `python` and `library/tests` packages are omitted entirely via `--deleted_packages`.

## Running WASM Examples

### Node.js

```bash
node bazel-bin/examples/wasm/solve_board.js
```

## Compilation Flags

| Flag | Purpose |
|------|---------|
| `-O3` | Aggressive optimization |
| `-flto` | Link-time optimization |
| `-fexceptions` | Enable C++ exceptions |
| `-D__WASM__` | Preprocessor constant for WASM builds |
| `-sWASM=1` | Emscripten WASM output (link flag) |
| `-sALLOW_MEMORY_GROWTH=1` | Allow heap growth at runtime |
| `-sINITIAL_MEMORY=268435456` | 256MB initial memory |

## Related Documentation

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Bazel Build System](https://bazel.build/docs)
- [DDS C++ API](c++_interface.md)
- [Build System Overview](BUILD_SYSTEM.md)
