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

**Xcode 27 / MacOSX 27 SDK TBD files.** The macOS 27 SDK lists `arm64e.x1` in
system `.tbd` stubs. LLVM 21's `ld64.lld` rejects those files (`unknown
architecture arm64e.x1`) and then reports a cascade of missing libc++ /
libSystem symbols. Native Darwin links therefore use Apple's `/usr/bin/ld`
via `llvm.toolchain.extra_link_flags` in `MODULE.bazel`, and `.bazelrc`
disables Bazel's `supports_start_end_lib` feature (Apple ld does not accept
`--start-lib` / `--end-lib`). That `--ld-path` override is toolchain-scoped
on purpose: putting it in `build:macos --linkopt` would leak into wasm
transitions and replace `wasm-ld`. Drop both workarounds when LLVM's Mach-O
linker can parse `arm64e.x1` TBD targets.

Switching the macOS link driver away from `ld64.lld` risks the
`-flto=thin` invariant (see specs/build-system.md): classic `ld64` needed an
explicit `-lto_library` plugin flag to perform LTO, which nothing here
supplies. Verified on Xcode 27.0 / macOS 26.6.2 with
`bazelisk build --linkopt=-Wl,-v //path:target` on a real target — plain
`bazelisk build -s`/`-v` only print Bazel's own subcommand/log output and
never reach the linker's own banner, so `-Wl,-v` is needed to make clang
forward `-v` to the linker itself: Apple's linker reports `ld-27037.1` with
`LTO support using: LLVM version 21.0.0` built in, so ThinLTO still runs
through `/usr/bin/ld` without `-lto_library` on this linker generation.
`python/tests/ci_macos_native_linker_test.py` only guards that `-flto=thin`
stays requested in `CPPVARIABLES.bzl`; it cannot detect a future Xcode linker
that keeps accepting the flag but silently stops doing cross-TU optimization
with it (the build would still succeed; only the optimization would quietly
go away). Re-check with `bazelisk build --linkopt=-Wl,-v //path:target` on a
link action and look for `LTO support using:` in the linker's own stderr
banner after any Xcode upgrade that touches the macOS linker.

### Linux linker (GNU ld)

The upstream `LLVM-<version>-Linux-X64` archive that `toolchains_llvm`
downloads ships an `ld.lld` that is dynamically linked against
`libxml2.so.2`. libxml2 2.14 changed its soname to `libxml2.so.16`, and
distributions on it (Ubuntu 26.04, Arch, Fedora rawhide, ...) no longer
provide `.so.2`, so `ld.lld` fails to start with `error while loading shared
libraries: libxml2.so.2`. Native Linux links therefore use the host's GNU
`ld` (`/usr/bin/ld`) through the same toolchain-scoped
`llvm.toolchain.extra_link_flags` mechanism as macOS, and `.bazelrc`
disables `supports_start_end_lib` on Linux because GNU ld does not accept
`--start-lib` / `--end-lib`. Compilation is unaffected; clang and libc++ still
come from the LLVM archive.

GNU ld also resolves static archives strictly in command-line order, unlike
lld. `toolchains_llvm` places `-l:libunwind.a` among the early link flags,
before the `libc++abi.a` that needs `_Unwind_*`, so `MODULE.bazel` repeats
`libc++.a`, `libc++abi.a` and `libunwind.a` (in that order) via
`extra_link_libs` at the end of every Linux link.

Consequences:

- Linux hosts need **binutils** installed (`/usr/bin/ld`). It is present
  wherever `build-essential` / a C development toolchain is; minimal
  containers may need `apt-get install binutils` or `dnf install binutils`.
- `--config=msan` uses its own toolchain (`llvm_toolchain_msan`) and keeps
  `ld.lld`; its CI job is pinned to Ubuntu 22.04, which still ships
  `libxml2.so.2`.

Drop the Linux override once the LLVM Linux archive no longer needs a host
`libxml2.so.2` (or the project moves to a statically linked LLVM).

### Linux host requirements and distro CI

Because no sysroot is configured, Linux builds compile against the host's
glibc headers and link with the host's GNU ld. The downloaded clang also needs
glibc ≥ 2.34 and libstdc++ from gcc ≥ 12 (`GLIBCXX_3.4.30`) just to start,
so RHEL 8/9 and their clones are not supported. RHEL 10 and current Fedora
are tested on every PR, and rolling Fedora, Arch, openSUSE and Debian weekly;
Debian 12+ and Ubuntu 22.04+ are expected to work (inferred from the same
symbol versions, not tested).

`.github/workflows/ci_linux_distros.yml` builds and tests `//library/...`
and `//python/...` in `fedora:latest` (newest glibc) and `almalinux:10`
(oldest supported RHEL) containers on every PR. Its Bazel disk cache is off:
host headers are not declared action inputs, so a cache hit could hide a
header change.

`.github/workflows/ci_linux_distros_rolling.yml` runs the same job every
Monday at 06:00 UTC against `fedora:rawhide`, `archlinux:latest`,
`opensuse/tumbleweed` and `debian:testing`. These rolling distributions take
new glibc and binutils releases first, so a failure there is an early warning
for the next Fedora or Ubuntu release, not a regression in recent PRs; it never
runs on pull requests. GitHub notifies only the user who last changed its
`cron` line (or who last re-enabled it), and disables scheduled workflows
after 60 days without repository activity until someone re-enables them, so
check the Actions tab after a quiet spell. Both workflows can also be started
by hand, and `ci_linux_distros.yml` accepts an `images` input (a JSON array
such as `["debian:stable"]`) to try another image that meets the floor above
(glibc ≥ 2.34 and `GLIBCXX_3.4.30`) from a family that
`install_distro_deps.sh` supports (dnf, apt, pacman or zypper).

The packages a minimal container needs are listed per distribution in
`.github/scripts/install_distro_deps.sh`; to reproduce a failure locally:

```bash
# --platform: there is no linux-aarch64 LLVM toolchain, so arm64 hosts (Apple
# silicon) must run the amd64 image under emulation, which is slow.
docker run --rm -it --platform linux/amd64 -v "$PWD:/src" -w /src fedora:latest bash
# inside the container (bazelisk pinned to the version CI uses):
bash .github/scripts/install_distro_deps.sh
curl -fsSL -o /usr/local/bin/bazelisk \
  https://github.com/bazelbuild/bazelisk/releases/download/v1.29.0/bazelisk-linux-amd64
chmod +x /usr/local/bin/bazelisk
# --symlink_prefix=/ stops Bazel replacing the host checkout's bazel-*
# symlinks with paths that only exist inside the container.
bazelisk test --symlink_prefix=/ //library/... //python/...
```

### AddressSanitizer, ThreadSanitizer, UndefinedBehaviorSanitizer, and MemorySanitizer

Sanitizer builds use `--config=asan`, `--config=tsan`, `--config=ubsan`, or
`--config=msan` (see `.bazelrc`). Do not use `--define=asan=true`; that path is
not supported. macOS-specific settings for ASAN/TSAN/UBSAN are chained
automatically; you do not pass a separate `asan_macos`, `tsan_macos`, or
`ubsan_macos` flag. MSAN is Linux x86_64 only.

**Clang / Xcode version coupling (macOS ASAN/TSAN/UBSAN).** Three places must
stay on the same **clang major** version (currently **21**):

| Location | Role |
|----------|------|
| `MODULE.bazel` → `llvm_versions` | Hermetic LLVM used for normal builds and for TSAN/UBSAN instrumentation |
| Installed Xcode (`xcrun clang++ --version`) | ASAN compiler/runtime; TSAN/UBSAN `compiler-rt` dylibs |
| `.bazelrc` → `build:tsan_macos` / `build:ubsan_macos` rpath (`.../lib/clang/21/lib/darwin`) | Lets LLVM-instrumented TSAN/UBSAN binaries load Xcode's runtimes |

**ASAN (`--config=asan`).** macOS builds select the Xcode CC toolchain via
`apple_support` (`build:asan_macos`). LLVM's bundled ASAN runtime hangs at
startup; mixing LLVM-instrumented objects with Xcode's ASAN dylib aborts with
a version mismatch. Using Xcode for the full compile/link avoids that.
`MODULE.bazel` patches `apple_support` 1.24.2 so its crosstool helpers build
with `-mmacosx-version-min=11.0` (Xcode 27's libc++ warns below that) and
without a dead `hasher(file)` nodiscard call in `libtool.cc`. Drop the patches
when bumping to a release that includes those fixes (`>= 2.8.4` for the min-OS
change).

**TSAN (`--config=tsan`).** Code is still compiled with the hermetic LLVM
toolchain; only the **runtime** comes from Xcode's `compiler-rt`. LLVM's TSAN
dylib segfaults on current macOS, but Xcode's dylib works with LLVM-instrumented
binaries when the clang major matches.

**UBSAN (`--config=ubsan`).** Standalone config (not combined with ASAN) so
failure attribution stays clear and ASAN's Xcode-toolchain coupling is not
shared. Like TSAN, instrumentation uses hermetic LLVM and the runtime is loaded
from Xcode via `build:ubsan_macos`. `-fno-sanitize-recover=undefined` makes the
first UB report abort the process (required for CI).

**MSAN (`--config=msan`).** Linux x86_64 only. See the
[Clang MemorySanitizer documentation](https://clang.llvm.org/docs/MemorySanitizer.html)
for what MSAN detects and its limits (it is not a complete memory-safety proof).
MemorySanitizer reports false positives unless the C++ standard library is also
instrumented, so `--config=msan` selects `@llvm_toolchain_msan` (not registered
globally) and enables `--features=msan`. That toolchain ships an instrumented
libc++ overlay (`libcxx_url` / `libcxx_sha256` in `MODULE.bazel`) built against
the same Ubuntu 22.04 LLVM release as `MSAN_LLVM_VERSION`. Keep those pins
aligned when bumping LLVM. Do not combine MSAN with ASAN/TSAN/UBSAN.

`MODULE.bazel` currently `archive_override`s `toolchains_llvm` past BCR 1.8.0
to include [#791](https://github.com/bazel-contrib/toolchains_llvm/pull/791)
(drops unused `-stdlib=libc++` that breaks Linux `-Werror` builds). Drop the
override when BCR publishes a release that includes that fix.

**When upgrading LLVM or Xcode**, update all coupled paths together:

1. Bump `llvm_versions` in `MODULE.bazel` (and run
   `bazelisk mod deps --lockfile_mode=update` to refresh `MODULE.bazel.lock`).
2. For MSAN, bump `MSAN_LLVM_VERSION` / `MSAN_DISTRIBUTION` / `libcxx_*` to a
   matching instrumented-libc++ release (see toolchains_llvm's
   `tests/MODULE.bazel` for the overlay hosting pattern).
3. Update the `clang/N` segment in `build:tsan_macos` and `build:ubsan_macos` in
   `.bazelrc` to match the new major version.
4. Confirm Xcode ships that major:  
   `xcrun clang++ --version`  
   `xcrun clang -print-file-name=libclang_rt.tsan_osx_dynamic.dylib`  
   `xcrun clang -print-file-name=libclang_rt.ubsan_osx_dynamic.dylib`
5. Re-run sanitizer smoke tests, e.g.  
   `bazelisk test --config=asan //library/tests/system:context_tt_facade_test`  
   `bazelisk test --config=tsan //library/tests/system:thread_safety_stress_test`  
   `bazelisk test --config=ubsan //library/tests/system:context_tt_facade_test`  
   `bazelisk test --config=msan //library/tests/system:context_tt_facade_test` (Linux)

If TSAN or UBSAN fails to start after an Xcode upgrade (dyld cannot load the
runtime), the rpath major is likely out of sync with the installed toolchain.

**CI.** GitHub Actions runs sanitizer jobs on pull requests to `main` and
`develop`:

| Workflow | Job | Command |
|----------|-----|---------|
| `ci_linux.yml` | `build_and_test` | also `bazelisk build --define=ab_stats=true //library/src:dds`, `bazelisk test --define=ab_stats=true //library/tests/ab_search:ab_stats_test //library/tests/ab_search:tt_lookup_test`, and `bazelisk build --define=scheduler=true //library/src:dds` |
| `ci_linux.yml` | `asan` | `bazelisk test --config=asan //library/tests/...` |
| `ci_linux.yml` | `tsan` | `bazelisk test --config=tsan //library/tests/system/...` |
| `ci_linux.yml` | `ubsan` | `bazelisk test --config=ubsan //library/tests/...` |
| `ci_linux.yml` | `msan` | `bazelisk test --config=msan //library/tests/...` on `ubuntu-22.04` (matches LLVM/libcxx overlay) |
| `ci_macos.yml` | `sanitizers` | ASAN, TSAN, and UBSAN on `//library/tests/system/...` (validates macOS toolchain/rpath wiring) |


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
