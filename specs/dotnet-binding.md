---
capability: dotnet-binding
owners: [dotnet]
last-updated: 2026-07-19
---

# .NET Binding (DDS_Core)

> **Specs vs. doxygen / docs.** How to call the wrapper — the type-by-type API,
> legacy-vs-modern guidance, worked examples — is in `docs/dotnet_interface.md`
> and the XML doc comments on the types themselves. This spec records the
> cross-cutting contracts: which ABI layer is bound, how the native library is
> found, and the invariants the tests enforce.

## Purpose

This capability lets .NET consumers call the solver through an idiomatic managed
API — blittable structs, `SafeHandle` lifetimes, method overloading — on macOS,
Linux, and Windows from the same assembly. It exists so a .NET application gets
the solver without building the C++ or writing marshalling code. Like the JVM
binding it targets the pure-C shim from [dds-public-api](dds-public-api.md), not
the C++ API.

## Behaviour & invariants

> Per-type and per-method detail is in `DDS_Core`'s doc comments and
> `docs/dotnet_interface.md`. These are the whole-binding guarantees.

- **Two ABI layers, one library.** The modern context entry points bind the
  `dds_c_*` shim; the legacy flat API (`SolveBoard`, `CalcDDtable`, `Par`,
  `Analyse*`, …) binds `dll.h` directly. Both come from the single native
  library built by `//jni:dds_shared`. The binding does **not** use the
  reference-taking `dds_*` symbols in `dds_api.hpp`: those are not exported on
  Linux or macOS, so binding them made the wrapper Windows-only.
- **Managed names are decoupled from ABI names.** The shim is reached via
  `EntryPoint` on each `DllImport`, so `DdsNative`'s method names — and every
  call site in `DDS.cs` / `SolverContext.cs` — are independent of the C symbol
  names. Renaming a shim export touches one attribute.
- **No struct crosses the boundary by value.** `SolverConfig` is unpacked into
  scalar arguments at the P/Invoke and `TTKind` marshals as its underlying
  `int`, matching the shim's pointer-only, POD-only contract.
- **One native library name: `dds`.** .NET's probing supplies the `lib` prefix
  and per-OS extension, so a single `DllName` resolves `libdds.dylib`,
  `libdds.so`, and `dds.dll`.
- **Library resolution is overridable.** A resolver registered from
  `DdsNative`'s static constructor — which the runtime guarantees runs before
  that type's first P/Invoke — honours the `DDS_LIBRARY_PATH` environment
  variable, falling back to default probing when it is unset. When it is set but
  unusable the failure names the attempted path rather than silently falling
  through. This is the counterpart of the JVM binding's `-Ddds.library.path` and
  is intended for tests and development, not deployment.
- **Struct layouts are pinned by tests, not by convention.** The managed structs
  must match the C structs in `dll.h` byte for byte; a mismatch on SysV or
  AArch64 corrupts results *silently* rather than throwing. `LayoutTests`
  asserts sizes and field offsets against values derived from the C headers, and
  `FourHands` indexing against the `hand * 4 + suit` row-major layout the other
  bindings assume.
- **Solver contexts are single-threaded and deterministically released.**
  `SolverContext` owns a `SolverContextHandle` (`SafeHandle`), so the native
  context is freed on `Dispose` or finalization; disposal is idempotent. One
  context per thread, as with every binding (see
  [solver-context](solver-context.md)).
- **Integer status returns.** Entry points return `RETURN_*` codes as elsewhere
  in the API; the wrapper converts failures to exceptions at its public surface,
  in every build configuration. The check must not be made conditional on
  `DEBUG` — that silently downgrades Release consumers to unchecked return
  codes, which is the opposite of what this bullet promises.
- **All three ABIs are covered by CI.** The managed tests run on Linux (SysV
  x86-64), Windows (Win64), and macOS (AArch64), each against the Bazel-built
  shared library located via `bazel info bazel-bin`. Windows matters most: it is
  the platform the shim retarget could regress, and the Bazel-built `dds.dll`
  path is covered by no other test, since the JNI tests are
  `target_compatible_with`-excluded there.

## Key entry points

- `dotnet/DDS_Core/Native/DdsNative.cs` — every P/Invoke; `DllName` and the
  `EntryPoint` mapping onto `dds_c_*`.
- `dotnet/DDS_Core/Native/DdsNativeResolver.cs` — `DDS_LIBRARY_PATH` resolution.
- `dotnet/DDS_Core/DataModel/SolverContext.cs` — the modern managed API;
  `Helpers/SolverContextHandle.cs` — `SafeHandle` ownership.
- `dotnet/DDS_Core/DataModel/`, `Helpers/` — the blittable structs and inline
  array helpers whose layouts the tests pin.
- Native artifact: `//jni:dds_shared` (see [jni-ffm-binding](jni-ffm-binding.md)).
- Consumer guide: `docs/dotnet_interface.md`.
- Guarded by `dotnet/DDS_Core.Tests/` (`LayoutTests`, `SmokeTests`,
  `ContextLifecycleTests`) and, on the native side,
  `//library/tests:dds_c_api_test`.

## Known gaps / non-goals

- **No NuGet package yet.** Consumers build the project and supply the native
  library themselves. RID-specific packaging (`runtimes/<rid>/native/…`) and a
  multi-platform package are a deliberate follow-up, mirroring how the jar
  followed the JVM shared library.
- **`solution/dds_native.vcxproj` is no longer the shipped native artifact** but
  is still present, and `DDS_Core.slnx` still build-depends on it. It builds a
  differently-named DLL that the binding no longer loads; retiring it is
  deferred. As a consequence `DDS_Core.slnx` does not build under `dotnet
  build` — it includes a C++ project needing Visual Studio's MSBuild. The
  individual projects build fine, which is what CI uses.
- **The test project targets `net8.0` with `RollForward` set to `Major`**, so it
  also runs where only a newer major runtime is installed. CI pins an 8.0 SDK,
  where the property has no effect.
- Per-type API documentation is intentionally not duplicated here; it lives in
  `docs/dotnet_interface.md` and the types' doc comments.
