# Implementation Breakdown – Library Re‑organisation

| ID | Title | Description | Complexity | Dependencies | Acceptance Criteria |
|----|-------|-------------|------------|--------------|---------------------|
| T1 | Create `library/` package | Create `library/` directory with `src/` and `tests/` sub‑dirs. | Low | None | Directory structure exists. |
| T2 | Move source files | Copy/move `src/*.cpp`, `src/*.h`, `include/*.h` into `library/src/`. | Medium | T1 | All source files present in new location. |
| T3 | Move test files | Copy/move `tests/` contents into `library/tests/`. | Medium | T1 | All test files present in new location. |
| T4 | Add `library/src/BUILD.bazel` | Define `dds` and `testable_dds` cc_library targets with correct `copts`, `linkopts`, `local_defines`, `include_prefix`, `strip_include_prefix`. | Medium | T2 | `bazel build //library/src:dds` succeeds. |
| T5 | Update top‑level `BUILD.bazel` | Remove old `filegroup` targets; add `exports_files` if needed; add `exports` to expose `library/src` targets. | Medium | T4 | `bazel build //...` passes without errors. |
| T6 | Update `doxygen_docs` genrule | Adjust `srcs` glob to point to `library/src/` and `library/tests/`. | Low | T2, T3 | `bazel build //:doxygen_docs` generates zip. |
| T7 | Run CI locally | Execute `bazel test //...` and `bazel build //...` to verify. | Low | T4, T5, T6 | All tests pass, no build errors. |
| T8 | Commit and push | Create a new branch, commit changes, push, and open PR. | Low | T7 | PR created and CI passes. |

**Critical Path**: T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8  
**Parallel Work**: T2 and T3 can run concurrently after T1.
