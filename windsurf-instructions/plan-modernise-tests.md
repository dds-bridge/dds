# Plan: Modernize and Refactor Test Suite

## Objectives
- Transition from legacy integration-style test harnesses to a modern, maintainable, and automated test suite.
- Enable automated CI/CD validation of core DDS solver functionality.
- Improve test coverage, clarity, and developer experience.

## Tasks
- [ ] Inventory all code and scripts in the `test` directory
- [ ] Document the current test harnesses and their coverage
- [ ] Identify and extract reusable test data and input files
- [x] Select a modern C++ test framework (e.g., GoogleTest, Catch2)
- [ ] Design a structure for new unit and integration tests
- [ ] Incrementally port or rewrite existing tests as automated test cases
- [ ] Integrate new tests with Bazel's native `cc_test` rules
- [ ] Ensure new tests are runnable locally and in CI
- [ ] Deprecate or archive legacy test harnesses after migration

## Current Work: Heuristic Sorting Tests
- [x] Create `tests/heuristic-sorting` directory.
- [x] Scaffold a Google Test suite in `tests/heuristic-sorting/heuristic_sorting_test.cpp`.
- [x] Create Bazel `BUILD` file for the test.
- [ ] Implement tests for heuristic card comparison.
- [ ] Implement tests for move generation and sorting logic.

## Notes
- Prioritize tests for public API and critical solver logic
- Consider adding property-based and regression tests
- Maintain documentation for running and extending tests

---

*This plan is referenced from the main project plan. Update as progress is made or priorities change.*
