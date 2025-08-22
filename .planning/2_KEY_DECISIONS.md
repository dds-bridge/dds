# Architecture Decisions – Library Re‑organisation

## Decision 1: Package Structure
- **Context**: Bazel recommends one package per directory to avoid cross‑package dependencies.
- **Options**:
  1. Keep everything in the root package.
  2. Create a dedicated `library/` package.
- **Chosen**: **Create `library/` package**.
- **Rationale**: Improves modularity, simplifies visibility, and aligns with existing `rules_cc` patterns.

## Decision 2: Target Placement
- **Context**: `dds` and `testable_dds` are core libraries.
- **Options**:
  1. Keep them in the root `BUILD.bazel`.
  2. Move them into `library/src/BUILD.bazel`.
- **Chosen**: **Move into `library/src/BUILD.bazel`**.
- **Rationale**: Keeps source files and their targets colocated, easing maintenance.

## Decision 3: Build File Cleanup
- **Context**: Top‑level `BUILD.bazel` contains obsolete `filegroup` targets.
- **Options**:
  1. Keep filegroups for backward compatibility.
  2. Remove them and rely on package targets.
- **Chosen**: **Remove obsolete filegroups**.
- **Rationale**: Reduces clutter and potential confusion; tests will reference the new package targets.

## Decision 4: Include Path Management
- **Context**: Existing `include_prefix="dds"` and `strip_include_prefix="src"`.
- **Options**:
  1. Keep same settings in new `BUILD.bazel`.
  2. Adjust to new relative paths.
- **Chosen**: **Keep same settings**.
- **Rationale**: Maintains public API layout (`dds/...`) without breaking existing includes.

--- 