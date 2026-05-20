# WebAssembly (WASM) Build Guide

This document explains how to build the DDS library and examples for WebAssembly using Bazel.

## Prerequisites

### 1. Emscripten SDK

You must have the Emscripten SDK installed and configured. Follow the official installation guide:
https://emscripten.org/docs/getting_started/downloads.html

After installation, ensure `em++` is available on your PATH:

```bash
which em++
```

### 2. Bazel

Bazel 7.x or later is required. Install using your package manager or download from:
https://bazel.build/install

## Building WASM Examples

### Build All Examples

```bash
cd /workspaces/dds
bazel build --config=wasm //examples:all
```

### Build Specific Example

```bash
bazel build --config=wasm //examples:solve_board
```

### Output Files

When building with `--config=wasm`, the output files will be located in:
```
bazel-bin/examples/
```

Depending on the target, you'll get:
- `target.js` - JavaScript bindings
- `target.wasm` - WebAssembly binary
- `target.html` - HTML shell (for some targets like `solve_board`)

## Available WASM Targets

The following example targets are available for WASM builds:

### Core Examples
- `solve_board` - Solves a single board (produces .html)
- `solve_board_pbn` - Solves a board from PBN notation (produces .html)
- `solve_all_boards` - Solves multiple boards (produces .html)
- `dds` - Main DDS example (produces .html)

### Analysis Examples
- `analyse_play_bin` - Analyze play from binary format
- `analyse_play_pbn` - Analyze play from PBN format
- `analyse_all_plays_bin` - Analyze all plays from binary format
- `analyse_all_plays_pbn` - Analyze all plays from PBN format

### Calculation Examples
- `calc_dd_table` - Calculate double dummy table
- `calc_dd_table_pbn` - Calculate DD table from PBN
- `calc_all_tables` - Calculate all tables
- `calc_all_tables_pbn` - Calculate all tables from PBN
- `dealer_par` - Calculate dealer par scores
- `par` - Calculate par scores
- `calc_par_context_example` - Par context example

## WASM Build Configuration

The WASM build is configured through:

1. **BUILD.bazel** - Defines the WASM config_setting:
   ```
   config_setting(
       name = "build_wasm",
       constraint_values = ["@platforms//os:emscripten"],
   )
   ```

2. **CPPVARIABLES.bzl** - Specifies WASM-specific compiler flags:
   - Optimization: `-O3 -flto -mtune=generic`
   - Define: `-D__WASM__`

3. **.bazelrc** - Contains the `wasm` profile:
   ```
   build:wasm --compiler=emscripten
   build:wasm --cpu=wasm
   build:wasm --cxxopt=-std=c++20
   ```

## Running WASM Examples

After building, you can run the examples:

### Node.js (requires Node.js installed)

```bash
node bazel-bin/examples/solve_board.js
```

### Web Browser

For HTML targets, open the generated HTML file in a web browser:
```bash
# Copy the output files to a web-accessible location
cp bazel-bin/examples/solve_board.* /path/to/webserver/

# Then open in browser at http://localhost:8000/solve_board.html
```

## Troubleshooting

### Issue: `em++` not found

**Solution:** Ensure Emscripten SDK is installed and sourced:
```bash
source /path/to/emsdk/emsdk_env.sh
```

### Issue: Build fails with undefined reference errors

**Solution:** This may indicate:
1. Missing dependencies in the `deps` field of BUILD.bazel targets
2. Emscripten-specific link flags needed

### Issue: WASM binary is too large

**Solution:** Try building with aggressive optimization:
```bash
bazel build --config=wasm -c opt //examples:solve_board
```

Use `--define=opt_level=3` for maximum optimization if available.

## Compilation Flags

The WASM build uses the following key flags:

| Flag | Purpose |
|------|---------|
| `-O3` | Aggressive optimization |
| `-flto` | Link-time optimization |
| `-mtune=generic` | Generic CPU tuning |
| `-D__WASM__` | Defines `__WASM__` preprocessor constant |
| `-sWASM=1` | Emscripten WASM output (link flag) |

## C++ Standard

The WASM build uses C++20 as specified in `.bazelrc`:
```
build:wasm --cxxopt=-std=c++20
```

## Related Documentation

- [Emscripten Documentation](https://emscripten.org/docs/)
- [Bazel Build System](https://bazel.build/docs)
- [DDS C++ API](c++_interface.md)
- [Build System Overview](BUILD_SYSTEM.md)

## Next Steps

To integrate WASM builds into CI/CD:
1. See `.github/workflows/ci_linux.yml` and `.github/workflows/ci_macos.yml`
2. Add a WASM build job with `--config=wasm` and `em++` available
3. Store WASM artifacts for download/release

## Development Notes

- The `__WASM__` preprocessor constant is automatically defined for WASM builds
- Some threading and platform-specific features are disabled for WASM
- The build produces both `.js` and `.wasm` files
- For HTML targets, em++ produces `.html`, `.js`, and `.wasm` files together
