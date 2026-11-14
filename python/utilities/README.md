**Python hints**

See also `python/tests/README.md` and `docs/python_interface.md`

## `dd_table_for_deal`

*Run via Bazel*

```bash
bazelisk run //python/utilities:dd_table_for_deal "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
```

or

```bash
bazelisk run //python/utilities:dd_table_for_deal -- hands/example.pbn
bazelisk run //python/utilities:dd_table_for_deal -- --vul ns hands/example.pbn
bazelisk run //python/utilities:dd_table_for_deal -- --limit 3 hands/multi_board.pbn
```


*Run without Bazel*

To run the utilities directly, without Bazel, first build the native module and set `PYTHONPATH`:

    bazelisk build //python:_dds3
    export PYTHONPATH=python:bazel-bin/python

Direct commands:

```bash
python python/utilities/src/dd_table_for_deal.py "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
```

or

```bash
python python/utilities/src/dd_table_for_deal.py hands/example.pbn
```

## `benchmark`

Runs `dtest` timing comparisons. Wrapper at repo root: `./benchmark.sh`.

```bash
bazelisk run //python/utilities:benchmark
python python/utilities/src/benchmark.py --build -- -n 8
```

Compare against a sibling [Bridge-Solver-CPP](https://github.com/macroxue/Bridge-Solver-CPP)
checkout (`../Bridge-Solver-CPP/solver`), an explicit `--bridgesolver PATH`, or both:

```bash
python python/utilities/src/benchmark.py --bridge-solver-cpp -s calc
python python/utilities/src/benchmark.py --bridge-solver-cpp --bridgesolver /path/to/solver -s calc
```

## `create_list_for_dtest`

Generate random `dtest` hand-list files (conventionally `listNNN.txt` with `--seed NNN`).

```bash
bazelisk run //python/utilities:create_list_for_dtest -- -n 100 --seed 100 -o hands/list100.txt
bazelisk test //python/utilities:create_list_for_dtest_test
```

See also `utilities/src/regenerate_hand_lists.sh` and `utilities/src/verify_lists.sh`.

## `compare_bridge_solver`

Wall-time bake-off of DDS vs
[Bridge-Solver-CPP](https://github.com/macroxue/Bridge-Solver-CPP)
(same CLI as [macroxue/bridge-solver](https://github.com/macroxue/bridge-solver))
on full double-dummy tables (5 strains × 4 declarers). DDS is capped at one
thread so the comparison matches their single-threaded `solver` process.

Defaults assume a sibling checkout at `../Bridge-Solver-CPP` with a built
`solver` binary. Relative paths resolve from the directory where you invoke
bazel (`BUILD_WORKING_DIRECTORY`).

```bash
# From dds/, with Bridge-Solver-CPP beside it:
bazelisk run //python/utilities:compare_bridge_solver -- --limit 25
```

Build their solver if needed (their PGO `makefile` assumes Linux `/proc/cpuinfo`):

```bash
git clone https://github.com/macroxue/Bridge-Solver-CPP.git ../Bridge-Solver-CPP
cd ../Bridge-Solver-CPP
c++ -std=c++17 -O3 -o solver solver.cc
```

Override paths for a different checkout:

```bash
bazelisk run //python/utilities:compare_bridge_solver -- \
  --bridge-solver ../bridge-solver/solver \
  --deals-dir ../bridge-solver/deals/fixed \
  --limit 25
```

Default DDS threading is 1 (fair vs their single-threaded process). For a
multi-threaded DDS run:

```bash
bazelisk run //python/utilities:compare_bridge_solver -- \
  --limit 25 \
  --dds-threads 8
```

`--dds-threads 0` uses all hardware threads. `--threads` is an alias.

`--deals-dir` expects their deal-file layout (`deal.01`, …; `RESULTS` is ignored).
Exit status is non-zero if any trick table mismatches.

```bash
bazelisk test //python/utilities:compare_bridge_solver_test
```

## Tests

```bash
bazelisk test //python/utilities:benchmark_test
bazelisk test //python/utilities:convert_pbn_test
bazelisk test //python/utilities:dd_table_for_deal_par_test
bazelisk test //python/utilities:create_list_for_dtest_test
bazelisk test //python/utilities:compare_bridge_solver_test
```
