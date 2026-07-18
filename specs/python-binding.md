---
capability: python-binding
owners: [python]
last-updated: 2026-07-18
---

# Python Binding

> **Specs vs. doxygen / docs.** How to build, import, and call the Python API is
> in `docs/python_interface.md`; per-function behaviour matches the underlying
> C++ API. This spec records the packaging model, the type-conversion boundary,
> and the context-reuse contract.

## Purpose

This capability exposes the solver to Python as the `dds3` package, backed by a
pybind11 native extension. It exists so Python users get double-dummy analysis
(solve, DD tables, par, play analysis) as an ordinary importable module and
installable wheel, without touching the C ABI directly. It wraps the modern
context API and the flat API from [dds-public-api](dds-public-api.md).

## Behaviour & invariants

> Per-function signatures are in `docs/python_interface.md`; these are the
> whole-binding facts.

- **`_dds3` is the pybind11 extension; `dds3` is the Python package.** The
  extension (`src/bindings.cpp` + `src/converters.cpp`) links `//library/src:dds`.
  The `dds3` package (`dds3/__init__.py`) re-exports the extension's symbols and
  is what users import; it falls back from `._dds3` to a top-level `_dds3` import
  so it works both in-package and when the extension is a bare module.
- **Public surface is the package `__all__`.** `python/dds3/__init__.py` re-exports
  the extension symbols; see that list and `docs/python_interface.md` rather than
  duplicating names here. Notable capability-wide pieces: a Python `SolverContext`
  for TT reuse, and `initialise_static_memory` for lookup-table setup. Note
  `set_max_threads` is exported but **deprecated** — it is an alias of
  `initialise_static_memory` that ignores its argument and warns; it does not set
  a worker count.
- **In-package native staging is platform-specific.** `_dds3_in_package` copies
  the built extension into `dds3/` under the filename Python expects —
  `dds3/_dds3.so` on unix, `dds3/_dds3.pyd` on Windows (selected via
  `//:build_windows`). `dds3_lib` bundles `__init__.py` + that staged extension as
  its `data`.
- **`SolverContext` is exposed for reuse.** A Python `SolverContext` maps to the
  C++ [solver-context](solver-context.md); holding one across multiple solves reuses its
  transposition table, mirroring the C++ reuse model. Guarded by
  `context_reuse_test`.
- **Type conversions are centralised.** `converters.{cpp,hpp}` marshal Python
  values ↔ the C++ deal/table/par structs; the conversion contract (PBN strings,
  binary deals, table/par result shapes) is guarded by `type_conversions_test`.
- **Wheels are built from the same package.** `dds3_wheel` (`py_wheel`,
  distribution `dds3`, version `1.0.0`, `strip_path_prefixes = ["python/"]`)
  packages `dds3_lib` + the staged extension; `dds3_wheel_dist` produces the
  distributable `dist/`. Requires Python 3.10+.
- **Behaviour tracks the C++ core.** The binding adds no solving logic of its own;
  the `py_test` suite exercises the main entry paths (smoke, solve, tables, par,
  analyse, convert_pbn, context reuse) against the same core. It is coverage of
  the binding surface, not a claimed full parity matrix.

## Key entry points

- `python/BUILD.bazel` — `_dds3`, `_dds3_in_package`, `dds3_lib`, `dds3_wheel`,
  `dds3_wheel_dist`, and the `py_test` suite.
- `python/src/bindings.cpp`, `python/src/converters.{cpp,hpp}` — the extension.
- `python/dds3/__init__.py` — the package surface (`__all__`).
- Consumer guide: `docs/python_interface.md`.
- Guarded by the `//python:*_test` suite.

## Known gaps / non-goals

- The binding is a thin wrapper — it does not add Python-level convenience beyond
  exposing the C++ API and `SolverContext`.
- Wheels are built per host platform; cross-platform wheel matrices are outside
  this capability.
- Per-function argument documentation lives in `docs/python_interface.md`, not
  here.
