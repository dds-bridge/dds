# Build System Documentation
DDS can be built using Bazel or Rider on supported platforms (macOS, Linux, Windows) 
and Visual Studio on Windows only. 

## C++ Toolchain

DDS uses `bazel-contrib/toolchains_llvm` for C++ compilation on supported
macOS, Linux and Windows hosts. The Bazel module configuration pins LLVM
**21.1.8** for `darwin-aarch64`, `linux-x86_64`, and `windows-x86_64` in
`MODULE.bazel`, and registers the downloaded toolchains via
`@llvm_toolchain//:all`. Other hosts use Bazel's default C++ toolchain
resolution.

Project-specific warning and feature flags remain in `CPPVARIABLES.bzl`, while
toolchain selection lives in `MODULE.bazel` and standard-language settings are
primarily configured in `.bazelrc` (with a fallback default in
`CPPVARIABLES.bzl`).

### Bazel version and MODULE.bazel.lock

Use [bazelisk](https://github.com/bazelbuild/bazelisk) (CI already does). The
repo pins the Bazel version in `.bazelversion`; keep that in sync with the
committed `MODULE.bazel.lock` when upgrading Bazel.

`MODULE.bazel.lock` is checked in. `.bazelrc` sets `--lockfile_mode=error` so
builds fail instead of silently rewriting the lockfile. That keeps resolution
reproducible and avoids ambient lockfile diffs in unrelated PRs.

When you change `MODULE.bazel` (or intentionally refresh deps), update the
lockfile and commit it:

```bash
bazelisk mod deps --lockfile_mode=update
```

Do not hand-edit the lockfile. For merge conflicts, restore it and regenerate
after resolving `MODULE.bazel`, or use Bazel's lockfile merge driver
(`.gitattributes` already declares `merge=bazel-lockfile-merge`). Register the
driver once per machine (requires `jq` 1.5+). Download the merge script to a
file (do not embed it in git config) and pin the URL to the Bazel version in
`.bazelversion`; re-download when you bump that pin:

```bash
mkdir -p "${HOME}/.local/share/bazel"
# Run from the repo root so .bazelversion supplies the pin.
curl -fsSL \
  "https://raw.githubusercontent.com/bazelbuild/bazel/$(cat .bazelversion)/scripts/bazel-lockfile-merge.jq" \
  -o "${HOME}/.local/share/bazel/bazel-lockfile-merge.jq"
# Optionally inspect: less "${HOME}/.local/share/bazel/bazel-lockfile-merge.jq"
git config --global merge.bazel-lockfile-merge.name \
  "Merge driver for the Bazel lockfile (MODULE.bazel.lock)"
git config --global merge.bazel-lockfile-merge.driver \
  "jq -s -f ${HOME}/.local/share/bazel/bazel-lockfile-merge.jq -- %O %A %B > %A.jq_tmp && mv %A.jq_tmp %A"
```

### macOS SDK and Runtime Compatibility

On macOS, binaries built against a newer SDK/runtime than the currently running
OS can fail at startup (for example with `dyld` symbol lookup errors).

If you see runtime loader failures after a toolchain or OS change:

1. Verify host OS and SDK versions (`sw_vers -productVersion`, `xcrun --show-sdk-version`).
2. Re-resolve Bazel toolchains (`bazelisk shutdown`, then `bazelisk clean --expunge`).
3. Re-run `bazelisk test //...` to confirm runtime compatibility.

### AddressSanitizer and ThreadSanitizer (macOS)

Sanitizer builds use `--config=asan` or `--config=tsan` (see `.bazelrc`).
Do not use `--define=asan=true`; that path is not supported.
macOS-specific settings are chained automatically; you do not pass a separate
`asan_macos` or `tsan_macos` flag.

**Clang / Xcode version coupling.** Three places must stay on the same **clang
major** version (currently **21**):

| Location | Role |
|----------|------|
| `MODULE.bazel` → `llvm_versions` | Hermetic LLVM used for normal builds and for TSAN instrumentation |
| Installed Xcode (`xcrun clang++ --version`) | ASAN compiler/runtime; TSAN `compiler-rt` dylib |
| `.bazelrc` → `build:tsan_macos` rpath (`.../lib/clang/21/lib/darwin`) | Lets LLVM-instrumented TSAN binaries load Xcode's TSAN runtime |

**ASAN (`--config=asan`).** macOS builds select the Xcode CC toolchain via
`apple_support` (`build:asan_macos`). LLVM's bundled ASAN runtime hangs at
startup; mixing LLVM-instrumented objects with Xcode's ASAN dylib aborts with
a version mismatch. Using Xcode for the full compile/link avoids that.

**TSAN (`--config=tsan`).** Code is still compiled with the hermetic LLVM
toolchain; only the **runtime** comes from Xcode's `compiler-rt`. LLVM's TSAN
dylib segfaults on current macOS, but Xcode's dylib works with LLVM-instrumented
binaries when the clang major matches.

**When upgrading LLVM or Xcode**, update all coupled paths together:

1. Bump `llvm_versions` in `MODULE.bazel` (and run
   `bazelisk mod deps --lockfile_mode=update` to refresh `MODULE.bazel.lock`).
2. Update the `clang/N` segment in `build:tsan_macos` in `.bazelrc` to match the
   new major version.
3. Confirm Xcode ships that major:  
   `xcrun clang++ --version`  
   `xcrun clang -print-file-name=libclang_rt.tsan_osx_dynamic.dylib`
4. Re-run sanitizer smoke tests, e.g.  
   `bazel test --config=asan //library/tests/system:context_tt_facade_test`  
   `bazel test --config=tsan //library/tests/system:thread_safety_stress_test`

If TSAN fails to start after an Xcode upgrade (dyld cannot load the runtime),
the rpath major is likely out of sync with the installed toolchain.

**CI.** GitHub Actions runs sanitizer jobs on pull requests to `main` and
`develop`:

| Workflow | Job | Command |
|----------|-----|---------|
| `ci_linux.yml` | `asan` | `bazel test --config=asan //library/tests/...` |
| `ci_linux.yml` | `tsan` | `bazel test --config=tsan //library/tests/system/...` |
| `ci_macos.yml` | `sanitizers` | ASAN and TSAN on `//library/tests/system/...` (validates macOS toolchain/rpath wiring) |


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
