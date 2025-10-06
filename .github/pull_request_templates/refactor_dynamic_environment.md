# Harden TT init + context ownership flag stabilization

Summary
- Fix crash in dtest under `--define=tt_context_ownership=true` by hardening TransTableS::MakeTT and aligning eager TT creation.
- Add env overrides for TT sizing and optional debug logging.
- Exclude legacy SolverContext.cpp from build to avoid duplicate/legacy implementation conflicts.

Details
- TransTableS::MakeTT: removed pre-free loops causing unsafe behavior with constrained memory; guarded maxIndex when `maxmem <= summem`; added debug log emitted when `DDS_DEBUG_TT_CREATE` is set.
- Memory eager path: honor `DDS_TT_DEFAULT_MB` and `DDS_TT_LIMIT_MB` consistently; log effective sizes when `DDS_DEBUG_TT_CREATE` is set; avoid accidental TT creation during teardown by using `maybeTransTable()`.
- Build: `library/src/BUILD.bazel` excludes legacy `SolverContext.cpp`; route context through `system/SolverContext.*` and `SolverContextAdapter`.

Validation
- `bazel test //...` passes with and without `--define=tt_context_ownership=true`.
- `dtest` builds and runs with both default and constrained TT sizes, no segfaults.

Next steps (separate PR)
- Complete ownership handoff (remove `ThreadData::transTable` and `ttExternallyOwned`) and surface Reset/Clear/Resize on `SolverContext`; add focused unit tests.
