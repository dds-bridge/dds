# WebAssembly (WASM) Build Guide

This document explains how to build the DDS library and examples for WebAssembly using Bazel.

## Prerequisites

### Bazel

Bazel 7.x or later is required. Install using your package manager or download from:
https://bazel.build/install

The Emscripten SDK (emsdk) does NOT need to be manually installed. Bazel will automatically download, configure, and cache the appropriate hermetic Emscripten toolchain for your host platform as part of the build process.

## Building WASM Examples

### Build All Examples

```bash
cd /workspaces/dds
bazel build //examples:all_examples_wasm
```

### Build Specific Example

```bash
bazel build //examples:solve_board_wasm
```

### Output Files

the output files will be located in:
```
bazel-bin/examples/
```

Depending on the target, you'll get:
- `target.js` - JavaScript bindings
- `target.wasm` - WebAssembly binary


## Available WASM Targets

The following example targets are available for WASM builds:

### Implemented Examples
- `solve_board` - Solves a single board (produces .js and .wasm)

- `analyse_play_bin` - Analyze play from binary format


## WASM Build Configuration

The WASM build is configured through:

1. **BUILD.bazel** - Defines the WASM config_setting:
   ```python
   config_setting(
       name = "build_wasm",
       values = {"cpu": "wasm"},
   )
   ```

2. **CPPVARIABLES.bzl** - Specifies WASM-specific compiler flags:
   - Optimization: `-O3 -flto`
   - Exceptions: `-fexceptions`
   - Define: `-D__WASM__`

3. **.bazelrc** - Contains the `wasm` profile:
   ```
   build:wasm --platforms=@emsdk//:platform_wasm
   build:wasm --cpu=wasm
   build:wasm --cxxopt=-std=c++20
   build:wasm --host_cxxopt=-std=c++20
   build:wasm --compilation_mode=opt
   ```

## Running WASM Examples

After building, you can run the examples:

### Node.js (requires Node.js installed)

```bash
node bazel-bin/examples/solve_board.js
```

### Web Browser (TODO)

For HTML targets, open the generated HTML file in a web browser:
```bash
# Copy the output files to a web-accessible location
cp bazel-bin/examples/solve_board.* /path/to/webserver/

# Then open in browser at http://localhost:8000/solve_board.html
```

## Compilation Flags

The WASM build uses the following key flags:

| Flag | Purpose |
|------|---------|
| `-O3` | Aggressive optimization |
| `-flto` | Link-time optimization |
| `-fexceptions` | Enable C++ exceptions |
| `-D__WASM__` | Defines `__WASM__` preprocessor constant |
| `-sWASM=1` | Emscripten WASM output (link flag) |
| `-sALLOW_MEMORY_GROWTH=1` | Allow heap growth at runtime |
| `-sINITIAL_MEMORY=268435456` | Configure 256MB initial memory |

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
2. Store WASM artifacts for download/release

## Development Notes

- The `__WASM__` preprocessor constant is added initially to bypass error, need to check again whether we can remove
- Some threading and platform-specific features are disabled for WASM
- The build flow for a reusable library wasm 
- A good HTML example
