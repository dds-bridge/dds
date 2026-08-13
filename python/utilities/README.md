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

## Tests

```bash
bazelisk test //python/utilities:benchmark_test
bazelisk test //python/utilities:convert_pbn_test
bazelisk test //python/utilities:dd_table_for_deal_par_test
```
