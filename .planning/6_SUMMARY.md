# Executive Summary – Library Re‑organisation

- **Feature**: Re‑organise source and test directories into a dedicated `library/` package.
- **Value**: Aligns repository with Bazel best practices, improves maintainability, and simplifies CI.
- **Implementation**: Move files, create `library/src/BUILD.bazel`, clean top‑level `BUILD.bazel`, update documentation generation.
- **Timeline**: 1–2 days (including local testing and PR review).
- **Top 3 Risks**:
  1. Build failures due to include path changes – mitigated by local testing.
  2. Test failures after move – mitigated by running `bazel test //...` locally.
  3. CI breakage from removed filegroups – mitigated by verifying build before PR.
- **Definition of Done**:
  - All tests pass.
  - CI builds succeed.
  - Documentation generation works.
  - No obsolete targets remain in top‑level `BUILD.bazel`.
- **Next Steps**:
  1. Create a new branch `feature/library-structure`.
  2. Apply the plan tasks (T1–T8).
  3. Push changes and open a PR for review.
  