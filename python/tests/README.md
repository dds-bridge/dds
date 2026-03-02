# Python Unit Tests for dds3 Interface

## Running Tests

### Prerequisites
```bash
pip install pytest
```

### Running All Tests
```bash
cd /path/to/dds3/repository
export PYTHONPATH=python:bazel-bin/python:$PYTHONPATH
pytest python/tests/ -v
```

### Running Specific Test Module
```bash
pytest python/tests/test_solve_board.py -v
pytest python/tests/test_calc_tables.py -v
pytest python/tests/test_par.py -v
pytest python/tests/test_type_conversions.py -v
```

### Running Specific Test
```bash
pytest python/tests/test_solve_board.py::TestSolveBoard::test_solve_board_basic -v
```

## Test Organization

### test_import.py (Smoke Test)
- Basic module import validation
- API function callable checks
- Minimal setup validation

### test_solve_board.py
- **TestSolveBoard**: Tests for single-board solving with binary format input
  - Happy path: basic deal solving
  - Invalid inputs: trump, first, trick suit/rank validation
  - Default parameters behavior

- **TestSolveBoardPBN**: Tests for PBN string-based solving
  - Happy path: valid PBN parsing
  - Invalid inputs: PBN format, trump, first validation
  - Default parameters (trump=4/NT, first=0/North)

- **TestSolveBoardParity**: Cross-API consistency tests

### test_calc_tables.py
- **TestCalcDDTable**: Single DD table calculation
  - Happy path: basic table computation
  - Invalid inputs: card array size validation
  - Result structure validation

- **TestCalcAllTablesPBN**: Batch DD table calculation
  - Happy path: single and multiple deals
  - Par calculation modes (mode parameter)
  - Trump filter validation (0=include, 1=skip per strain)
  - Default parameters (mode=-1, trump_filter=(0,0,0,0,0))

- **TestTableParity**: Single vs batch result structure consistency

### test_par.py
- **TestPar**: Par score calculation
  - Happy path: par score computation
  - Vulnerability levels (0=none, 1=both, 2=NS, 3=EW)
  - Result structure validation

### test_type_conversions.py
- **TestArrayConversions**: Array/list/tuple conversion and validation
  - Tuple, list, and mixed sequence acceptance
  - Size validation (must be exactly 3 for trick arrays)
  - Boundary tests for trick suit (0-3) and rank (0-14)

- **TestPBNConversions**: PBN string parsing validation
  - Valid PBN format acceptance
  - Invalid seat designations
  - Truncated/empty string handling

- **TestTrumpFilterValidation**: Trump filter parameter bounds
  - Valid values: 0 (include), 1 (skip)
  - Invalid values: -1, 2, etc.
  - All-skip rejection

## Test Coverage Summary

### Functions Tested
- ✅ solve_board (binary format)
- ✅ solve_board_pbn (PBN format)
- ✅ calc_dd_table (single table)
- ✅ calc_all_tables_pbn (batch tables)
- ✅ par (par score calculation)

### Test Categories
- ✅ Happy path (valid inputs, expected outputs)
- ✅ Boundary validation (min/max values)
- ✅ Invalid input rejection (with error message checks)
- ✅ Default parameter behavior
- ✅ Type conversion (tuple, list, sequence)
- ✅ Result structure validation

### Known Limitations
- Par and DD table tests use simplified deals that may not produce valid results
  - Tests focus on input validation and API structure, not solver correctness
  - Actual solver correctness is validated by C++ unit tests
- Some tests use pytest.skip() when unable to create valid test input

## Continuous Integration

Tests are designed to run deterministically on both Linux and macOS CI environments.
The PYTHONPATH should include both the source package directory and the Bazel extension output directory.
`dds3` prefers `dds3._dds3` and falls back to top-level `_dds3` for local Bazel workflows.

Example CI command:
```bash
bazel build //python:dds3_lib
bazel test //python:python_interface_smoke_test
export PYTHONPATH=$PWD/python:$PWD/bazel-bin/python
pytest python/tests/ -v --tb=short
```
