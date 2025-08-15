# Product Requirements Document – Library Re‑organisation

## Overview
- **Goal**: Move all source (`src/`) and test (`tests/`) directories into a dedicated `library/` package to align with Bazel best‑practice layout.
- **Problem**: Current top‑level `BUILD.bazel` mixes source, header, and test files, making the repository hard to scale and violating the “one package per directory” rule.
- **Success Metrics**:
  - `bazel build //...` and `bazel test //...` pass with no errors.
  - No test failures after the move.
  - Build times remain within 10 % of current baseline.

## Requirements
| Requirement | Description |
|-------------|-------------|
| **R1** | All C++ source files must reside under `library/src/`. |
| **R2** | All test files must reside under `library/tests/`. |
| **R3** | A `BUILD.bazel` must exist in `library/src/` defining `dds` and `testable_dds` cc_library targets. |
| **R4** | Top‑level `BUILD.bazel` must be cleaned of obsolete filegroups and target definitions. |
| **R5** | All existing build flags (`DDS_CPPOPTS`, `DDS_LINKOPTS`, `DDS_LOCAL_DEFINES`) remain in effect. |
| **R6** | Include paths and visibility rules are preserved. |
| **R7** | Documentation generation (`doxygen_docs`) remains available. |

## User Experience
- Developers clone the repo and run `bazel build //library/dds` or `bazel test //library/tests:all`.
- CI pipelines automatically pick up the new layout without manual changes.

## Technical Approach
- Create a new `library/` package.
- Move `src/` → `library/src/`, `tests/` → `library/tests/`.
- Add `library/src/BUILD.bazel` with cc_library definitions.
- Update top‑level `BUILD.bazel` to reference `//library/src:dds` and `//library/src:testable_dds`.
- Remove obsolete `filegroup` targets and duplicate `srcs`/`hdrs` definitions.
- Adjust `include_prefix` and `strip_include_prefix` to match new paths.

## Acceptance Criteria
- `bazel build //library/dds` succeeds.
- `bazel test //library/tests:all` passes.
- No lint or build errors in CI.
- `doxygen_docs` still generates `doxygen_docs.zip`.

## Open Questions
- **OQ1**: Are there any external tools (e.g., IDE configs) that reference the old `src/` path?  
- **A1**: No, there are not external tools that reference the old `src` path.
- **OQ2**: Should we keep the old `src/` directory as a symlink for backward compatibility?
- **A2**: No, there is no need to keep it.
- **OQ3**: Do we need to expose any new public headers from `library/src/`?
- **A3**: We will have to create a new top-level target which depends on the library and adds public headers.  

