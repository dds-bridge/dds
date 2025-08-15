# Testing Strategy – Library Re‑organisation

## Core Test Categories
- **Unit Tests**: `library/tests` contains all existing unit tests.
- **Integration Tests**: Any tests that exercise multiple components (if present).
- **Build Tests**: `bazel build //...` ensures all targets compile.

## Critical Test Scenarios
1. **Compile All Targets**: `bazel build //...` must succeed.
2. **Run All Tests**: `bazel test //...` must pass.
3. **Doxygen Generation**: `bazel build //:doxygen_docs` produces a non‑empty zip.
4. **Include Path Validation**: Source files compile with `-I` paths correctly set.

## Automation
- All tests are automated via Bazel; CI will run them on every PR.
- Manual testing is not required unless a new feature is added.
