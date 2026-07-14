---
capability: shared_library
last-updated: 2026-07-14
related-plans:
  - ship_shared_lib
---

# Capability: Native shared library & JVM (FFM) consumption

Produces a single native shared library exporting the stable DDS C ABI, and
supports calling it from the JVM via the Foreign Function & Memory API. This is
the artifact non-C++ consumers (Java, .NET, ctypes) bind against.

## Behaviour & invariants

- **Artifact.** `//jni:dds_shared` builds one self-contained shared library —
  `libdds.so` (Linux), `libdds.dylib` (macOS), `dds.dll` (Windows) — with the
  entire solver (all internal sub-libraries) linked in statically, so the JVM's
  `System.loadLibrary`/`SymbolLookup` needs only this one file.
- **Export control.** The library exports **exactly** the public C API and no
  more: the flat legacy C functions declared in `library/src/api/dll.h` plus the
  `dds_c_*` shim functions in `library/src/api/dds_c_api.h`. No internal C++
  (name-mangled) symbol is exported. On Linux/macOS this is enforced at link
  time by `jni/version_script.lds` / `jni/exported_symbols.lds`; on Windows by
  `__declspec(dllexport)`. The `.lds` files are generated from the headers by
  `jni/gen_export_lists.py` and the invariant is checked by
  `//jni/tests:export_set_test`.
- **Pure-C shim contract.** `dds_c_api.h` exposes the modern SolverContext API
  with a C-valid, pointer-only, POD-only surface: the solver handle is opaque
  (`typedef void* DDS_C_SOLVER_CTX`), every struct is passed by pointer, and no
  non-POD C++ type (`SolverConfig`, `TTKind`, C++ references) crosses the
  boundary. Each shim function forwards to the corresponding reference-taking
  `dds_*` function.
- **Per-context lifecycle.** A solver context owns its own state and
  transposition table and is not thread-safe: one context per thread. Contexts
  are created with `dds_c_create_solvercontext_default` and released with
  `dds_c_destroy_solvercontext`. Solving through a context requires no prior
  global `InitializeStaticMemory` call.
- **FFM consumption.** `//jni:dds_ffm` (package `org.dds.ffm`) provides
  hand-written `java.lang.foreign` bindings — struct `MemoryLayout`s and
  `Linker` downcall handles — requiring JDK 22+ (FFM is stable there; the build
  pins the hermetic `remotejdk_25`). Libraries are loaded via
  `SymbolLookup.libraryLookup`; FFM downcalls require
  `--enable-native-access`. `jextract` is not used or required.
- **Parity.** The double dummy results obtained through the FFM path match the
  other bindings; `//jni:dds_ffm_smoke_test` cross-checks a known deal against
  the value produced by the Python binding.

## Non-goals

- Hand-written JNI (`native`-method) convenience classes.
- Native-in-JAR packaging / Maven distribution.
- Switching the whole build to `-fvisibility=hidden` (exports are constrained at
  link time instead).
