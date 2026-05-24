# Build System Documentation
DDS can be built using Bazel or Rider on supported platforms (macOS, Linux, Windows) 
and Visual Studio on Windows only. 

## C++ Toolchain

DDS uses `bazel-contrib/toolchains_llvm` for C++ compilation on supported
macOS, Linux and Windows hosts. The Bazel module configuration currently pins
host-specific LLVM versions in `MODULE.bazel`, including LLVM 20.1.8 for
`darwin-aarch64` and LLVM 21.1.8 for `linux-x86_64`, and registers the
downloaded toolchains via `@llvm_toolchain//:all`. Other hosts use Bazel's
default C++ toolchain resolution.

Project-specific warning and feature flags remain in `CPPVARIABLES.bzl`, while
toolchain selection lives in `MODULE.bazel` and standard-language settings are
primarily configured in `.bazelrc` (with a fallback default in
`CPPVARIABLES.bzl`).

### macOS SDK and Runtime Compatibility

On macOS, binaries built against a newer SDK/runtime than the currently running
OS can fail at startup (for example with `dyld` symbol lookup errors).

If you see runtime loader failures after a toolchain or OS change:

1. Verify host OS and SDK versions (`sw_vers -productVersion`, `xcrun --show-sdk-version`).
2. Re-resolve Bazel toolchains (`bazelisk shutdown`, then `bazelisk clean --expunge`).
3. Re-run `bazelisk test //...` to confirm runtime compatibility.


## Visual Studio and Rider Build
The top-level `solution` folder contains a Visual Studio solution file `solution.slnx` and 
project files for the dds and all the samples. It also contains a `Directory.Build.props` 
file which defines the common properties for all the projects. 

Note this line in the `Directory.Build.props` file: `<BuildDir>$(MSBuildThisFileDirectory)\..\Build\</BuildDir>` 
defining the output directory for all the projects. 



## API Layers

The library is structured into three API layers:

1. **Core Solver** (`library/src/ab_search.cpp`, `library/src/solve_board.cpp`, etc.)
2. **Modern C++ API** (`library/src/api/solve_board.hpp`)
    - `SolverContext` wrapper
    - Per-instance resource management
3. **Legacy C API** (`library/src/api/dll.h`)
    - C-compatible exports
    - Global state management
    - Backward compatibility layer

When building applications:
- Link against `//library/src:dds`
- Include either `<dds/dds.hpp>` (modern) or `<api/dll.h>` (legacy)
