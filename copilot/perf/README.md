Small microbench harnesses for heuristic investigations.

weight_alloc_microbench: measures legacy WeightAllocNT0 vs new CallHeuristic CPU cost.

Build and run (from workspace root):

```bash
bazel build //copilot/perf:weight_alloc_microbench
./bazel-bin/copilot/perf/weight_alloc_microbench
```

Fixtures
--------
The microbenchmark can use a few representative `pos` fixtures shipped in `copilot/perf/fixtures.h`:

- `fixture_basic()` — minimal empty position (control)
- `fixture_trump_rich()` — small trump-heavy scenario (from integration tests)
- `fixture_dense()` — denser synthetic distribution to stress heuristics

To use a fixture, include `copilot/perf/fixtures.h` in the microbenchmark and replace the `pos` initializer with a call to one of the fixture functions.
