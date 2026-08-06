**Python hints**

See also `python/tests/README.md` and `docs/python_interface.md`

*Run via Bazel*

```bash
bazelisk run //python/examples:dd_table_for_deal "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
```

or

```bash
bazelisk run //python/examples:dd_table_for_deal -- hands/example.pbn
bazelisk run //python/examples:dd_table_for_deal -- --vul ns hands/example.pbn
```


*Run without Bazel*

To run the examples directly, without Bazel, first build the native module and set `PYTHONPATH`:

    bazelisk build //python:_dds3
    export PYTHONPATH=python:bazel-bin/python

Direct commands:

```bash
python python/examples/dd_table_for_deal.py "N:73.QJT.AQ54.T752 QT6.876.KJ9.AQ84 5.A95432.7632.K6 AKJ9842.K.T8.J93"
```

or

```bash
python python/examples/dd_table_for_deal.py hands/example.pbn
```
