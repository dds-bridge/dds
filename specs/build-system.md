---
capability: build-system
owners: [//, CPPVARIABLES.bzl, wasm_compat.bzl]
last-updated: 2026-08-07
---

# Build System

> **Specs vs. doxygen / docs.** Step-by-step "how to build X" instructions live
> in `docs/BUILD_SYSTEM.md`. This spec records the config-setting matrix and the
> shared option sets — the facts every other capability's BUILD file depends on
> — not a command reference.

## Purpose

The project is a single Bazel module (`MODULE.bazel`, module name `dds`) that
builds the C++ core, the Java/Python/WASM/web bindings, the example CLIs, and the
tests. This capability owns the pieces that make those builds *compose*: the
platform and feature `config_setting`s in the root `BUILD.bazel`, and the shared
compile/link option sets in `CPPVARIABLES.bzl` / `wasm_compat.bzl` that every
`cc_library` pulls in. It exists so that per-module BUILD files stay thin and
consistent — they select a platform's flags and opt into features by name rather
than re-encoding toolchain knowledge.

## Behaviour & invariants

> Exact flag lists live in `CPPVARIABLES.bzl`; the point here is the contract, not
> the literal strings.

- **Platform selection is by OS constraint, plus a `dbg` compilation-mode twin.**
  `build_{macos,linux,windows}` match the OS — `build_windows` additionally
  requires `@platforms//cpu:x86_64`, so a Windows-arm64 build matches no
  `build_*` setting and falls through to `//conditions:default`. Each has a
  `debug_build_*` sibling
  gated on `compilation_mode = dbg`. `build_wasm` matches
  `@platforms//cpu:wasm32` (set by the `wasm_cc_binary` platform transition, not
  the host OS). Any target that wants per-platform flags does so through the
  `select()`s in `CPPVARIABLES.bzl` — it should not hand-roll `-O3`/`/O2`.
- **Optimised builds are strict.** Non-debug macOS/Linux/WASM use `-O3` with
  `-Wall -Wpedantic -Werror`; Windows uses `/O2 /W4 /WX /permissive-`. macOS adds
  LTO at both compile and link (`-flto=thin`); WASM adds `-flto` at compile time
  only — link-time LTO is not currently possible under the pinned hermetic emsdk
  toolchain (see [wasm-emscripten](wasm-emscripten.md)). C++20 is the baseline
  (`--cxxopt=-std=c++20` in `.bazelrc` for macOS/Linux; Windows `/std:c++20` in
  `DDS_CPPOPTS`). Treat `-Werror` as a standing invariant: warnings break the
  build.
- **Feature flags are `--define`-driven `config_setting`s**, surfaced through
  `DDS_LOCAL_DEFINES` / `DDS_SCHEDULER_DEFINE`:
  | config setting | `--define` | preprocessor define | capability |
  |---|---|---|---|
  | `debug_all` | `debug_all=true` | `DDS_DEBUG_ALL` | [constants-and-debug](constants-and-debug.md) |
  | `ab_stats` | `ab_stats=true` | `DDS_AB_STATS` | [ab-stats](ab-stats.md) |
  | `scheduler` | `scheduler=true` | `DDS_SCHEDULER` | [system-concurrency](system-concurrency.md) |
  | `tt_context_ownership` | `tt_context_ownership=true` | `DDS_TT_CONTEXT_OWNERSHIP` | [transposition-table](transposition-table.md) (define wired but inert — no source `#ifdef`s it) |
  | `tt_reset_debug` | `tt_reset_debug=true` | `DDS_DEBUG_TT_RESET` | [transposition-table](transposition-table.md) |
- **`DDS_LOCAL_DEFINES` is the standard `local_defines` for every core
  `cc_library`; `DDS_SCHEDULER_DEFINE` is appended only where scheduler timing is
  wanted** (`local_defines = DDS_LOCAL_DEFINES + DDS_SCHEDULER_DEFINE`). Off by
  default, these add zero cost.
- **WASM link flags are centralised** in `wasm_compat.bzl` (`WASM_LINKOPTS`):
  memory growth, 256 MB initial memory, an 8 MB stack (DDS search recursion
  overflows Emscripten's 64 KB default), and `PTHREAD_POOL_SIZE=8`. WASM
  `wasm_cc_binary` targets also set `threads = "emscripten"` — see
  [wasm-emscripten](wasm-emscripten.md).
- **The dependency graph is pinned in `MODULE.bazel`** (rules_cc, platforms,
  googletest, pybind11_bazel, rules_python, toolchains_llvm, apple_support, emsdk,
  plus the JVM rules for [jni-ffm-binding](jni-ffm-binding.md)). Toolchains are hermetic
  (LLVM via `toolchains_llvm`, Emscripten via `emsdk`). Exact versions live in the
  file and drift; do not hard-code them elsewhere.
- **`//:dds`** is the public façade library (re-exports `//library/src:dds`);
  **`//:testable_dds`** re-exports the test-visible variant to the test packages
  only. **`//:doxygen_docs`** is a manual, local-only developer target that needs
  `doxygen` + `zip` on `PATH`.

## Key entry points

- `BUILD.bazel` (root) — all `config_setting`s, the `//:dds` / `//:testable_dds`
  façades, and the `doxygen_docs` genrule.
- `.bazelrc` — C++20 cxxopts, hermetic JDK, default test tag filters (e.g. `-e2e`),
  sanitizer configs (`asan` / `tsan` / `ubsan` / Linux-only `msan`).
- `CPPVARIABLES.bzl` — `DDS_CPPOPTS`, `DDS_LINKOPTS`, `DDS_LOCAL_DEFINES`,
  `DDS_SCHEDULER_DEFINE`.
- `wasm_compat.bzl` — `WASM_LINKOPTS`.
- `MODULE.bazel` — the Bazel module and its `bazel_dep` graph (including
  `@llvm_toolchain_msan` for MemorySanitizer).
- Consumer guide: `docs/BUILD_SYSTEM.md`.

## Known gaps / non-goals

- **.NET is not built by Bazel** — `dotnet/` uses MSBuild (`*.csproj`) and is out
  of scope for this capability.
- No CMake / Makefile build path is maintained; Bazel is the single source of
  truth.
- This capability does not document per-target build commands (that is
  `docs/BUILD_SYSTEM.md`) or the semantics of each feature flag (those live with
  the capability the flag belongs to).
