# Risk Assessment – Library Re‑organisation

| Risk | Impact | Probability | Mitigation |
|------|--------|-------------|------------|
| R1 | Build failures due to incorrect include paths | High | Verify include_prefix/strip_include_prefix; run `bazel build //...` locally before PR. |
| R2 | Tests fail because of moved source files | Medium | Run `bazel test //...` locally; update test dependencies if needed. |
| R3 | CI pipeline breaks due to missing `filegroup` targets | Medium | Remove obsolete targets only after confirming tests compile against new package. |
| R4 | Documentation generation fails | Low | Update `doxygen_docs` genrule to new paths; run locally. |
| R5 | Merge conflicts with other branches | Low | Create branch early; coordinate with team. |

**Showstoppers**: R1, R2  
**Manageable Risks**: R3, R4, R5
