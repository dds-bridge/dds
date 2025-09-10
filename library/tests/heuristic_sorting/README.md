Purpose

Short notes and commands for running the heuristic-sorting test suite and related comparison/fuzz tools.

Prerequisites

- Bazel installed and on PATH
- A supported C++ toolchain; for coverage generation on macOS you will likely need GNU gcov (brew install gcc) or run coverage in a Linux container/CI

Build with new heuristic enabled

To build the workspace and enable the new heuristic library (this defines the runtime toggle `set_use_new_heuristic`):

```bash
bazel build //... --define=new_heuristic=true
```

Run tests (no cached results)

Run the whole heuristic-sorting test suite with the new heuristic and force tests to re-run:

```bash
bazel test //library/tests/heuristic_sorting:all \
  --define=new_heuristic=true \
  --nocache_test_results \
  --test_output=errors
```

Run a single test target (faster iteration):

```bash
bazel test //library/tests/heuristic_sorting:targeted_unit_tests \
  --define=new_heuristic=true \
  --nocache_test_results \
  --test_output=errors
```

Run the fuzz driver (tests are sandboxed so pass env via --test_env):

```bash
bazel test //library/tests/heuristic_sorting:fuzz_driver \
  --define=new_heuristic=true \
  --nocache_test_results \
  --test_output=errors \
  --test_env=FUZZ_SEED=42 \
  --test_env=FUZZ_ITER=1000
```

Coverage notes

I attempted a `bazel coverage` run; Bazel invoked `gcov` with flags that do not match the macOS-provided toolchain, producing no coverage data (errors like "gcov: Unknown command line argument '-output'" in the test logs).

Quick fixes:

- Install GNU gcov via Homebrew (recommended on macOS):

```bash
brew install gcc
# make gcov point to the GNU one if necessary:
# ln -s "$(which gcov-13)" "$HOME/bin/gcov"
# ensure $HOME/bin is first in PATH
```

- Re-run coverage once gcov is GNU-compatible:

```bash
bazel coverage //library/tests/heuristic_sorting:all \
  --define=new_heuristic=true \
  --nocache_test_results \
  --test_output=errors
```

- Alternatively run coverage inside a Linux container/CI where Bazel + GNU toolchain are available.

Notes and best practices

- The test package contains a small comparison harness which can switch between legacy and new implementations when built with `--define=new_heuristic=true` and then `set_use_new_heuristic(...)` is available at runtime.
- Use `--test_output=all` when debugging tests to capture stdout from gtest.
- Save golden outputs (normalize ordering JSON) under `build/compare-results` for triage when legacy vs new disagree.

Contact

If you want, I can add more golden unit tests or generate an HTML coverage report once a working gcov is available.
