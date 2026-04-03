# Python Interface Documentation

## Overview

The DDS (Double Dummy Solver) library provides a Python interface for analyzing bridge hands using the double-dummy solver. This interface allows you to calculate trick distribution, par scores, and other double-dummy analysis from Python.

## Building the Python Interface

### Prerequisites
- Python 3.10+ (tested with 3.10, 3.11, 3.12, 3.14)
- Bazel 7.x
- C++ compiler (clang 15+ or GCC 11+)

### Build Instructions

```bash
# Build the Python extension and Python package wrapper
bazel build //python:dds3_lib

# Build wheel artifact
bazel build //python:dds3_wheel_dist

# Build with optimizations
bazel build -c opt //python:_dds3

# Build with debug symbols
bazel build -c dbg //python:_dds3
```

The compiled extension will be located at `bazel-bin/python/_dds3.so`.
For wheel packaging, the extension is also copied into the package as `dds3/_dds3.so`.

## Installation and Testing

### Setup
```bash
# Create a virtual environment (optional but recommended)
python -m venv venv
source venv/bin/activate

# Install pytest (if not already installed)
pip install pytest
```

### Running Unit Tests
```bash
# Set PYTHONPATH to include source package and top-level extension fallback
export PYTHONPATH=python:bazel-bin/python

# Run Bazel smoke test for Python bindings
bazel test //python:python_interface_smoke_test

# Or use pytest directly
pytest python/tests/ -v

# Run specific test file
pytest python/tests/test_solve_board.py -v
```

### Test Coverage
The Python interface includes 65 comprehensive unit tests covering:
- Type validation and boundary checking
- PBN (Portable Bridge Notation) parsing
- Array/sequence conversions
- Error handling and exception propagation
- Default parameter behavior
- Solver invocation, result structure, and API integration (not full numerical validation of DDS solver results)

## API Reference

### Core Functions

#### `solve_board(deal, target=-1, solutions=3, mode=0, thread_index=0)`

Solves a single bridge deal using binary card format.

**Parameters:**
- `deal` (dict): Dictionary with keys:
  - `trump` (int, 0-4): Trump suit (0=♠, 1=♥, 2=♦, 3=♣, 4=NT)
  - `first` (int, 0-3): Player to lead (0=North, 1=East, 2=South, 3=West)
  - `remain_cards` (list[list[int]]): 4x4 array of bitmasks, `[hand][suit]`
  - `current_trick_suit` (tuple[int, int, int]): Current trick suits (0-3)
    - `current_trick_rank` (tuple[int, int, int]): Current trick ranks (0 or 2-14; 0 = unset)

**Returns:**
- dict with keys: `nodes`, `cards`, `suit`, `rank`, `equals`, `score`

**Example:**
```python
from dds3 import solve_board

deal = {
    "trump": 0,        # Spades
    "first": 0,        # North leads
    "remain_cards": [
        [0x7FFC, 0, 0, 0],        # North: all spades
        [0, 0x7FFC, 0, 0],        # East: all hearts
        [0, 0, 0x7FFC, 0],        # South: all diamonds
        [0, 0, 0, 0x7FFC],        # West: all clubs
    ],
    "current_trick_suit": (0, 0, 0),
    "current_trick_rank": (0, 0, 0),
}

result = solve_board(deal)
print(f"Tricks available: {result['score']}")
```

#### `solve_board_pbn(remain_cards, trump=4, first=0, current_trick_suit=(0,0,0), current_trick_rank=(0,0,0), target=-1, solutions=3, mode=0, thread_index=0)`

Solves a single bridge deal using PBN (Portable Bridge Notation).

**Parameters:**
- `remain_cards` (str): PBN string (e.g., "N:AK.234.456.789TJQ W:QJ.AKQJ.789.234 E:T9.T9.TJ.AK S:8765.8765.AKQJ32.6")
- `trump` (int, default=4): Trump suit (0-4)
- `first` (int, default=0): Player to lead
- `current_trick_suit` (tuple, default=(0,0,0)): Current trick suits
- `current_trick_rank` (tuple, default=(0,0,0)): Current trick ranks (0 or 2-14; 0 = unset)
- Other parameters: same as `solve_board`

**Returns:**
- dict with keys: `nodes`, `cards`, `suit`, `rank`, `equals`, `score`

**Example:**
```python
from dds3 import solve_board_pbn

pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
result = solve_board_pbn(pbn, trump=1)  # Hearts
print(f"Tricks: {result['score']}")
```

#### `calc_dd_table(table_deal)`

Calculates the double-dummy table for all contracts and strains.

**Parameters:**
- `table_deal` (dict): Dictionary with key:
  - `cards` (list[list[int]]): 4x4 array of bitmasks, `[hand][suit]`

**Returns:**
- dict with key: `res_table`
  - `res_table`: 5x4 array where `res_table[strain][hand]` = tricks available

**Example:**
```python
from dds3 import calc_dd_table

table_deal = {
    "cards": [
        [0x7FFC, 0, 0, 0],
        [0, 0x7FFC, 0, 0],
        [0, 0, 0x7FFC, 0],
        [0, 0, 0, 0x7FFC],
    ],
}

result = calc_dd_table(table_deal)
# result['res_table'][0][0] = tricks for spades, North
# result['res_table'][4][0] = tricks for NT, North
```

#### `calc_all_tables_pbn(deals_pbn, mode=-1, trump_filter=[0,0,0,0,0])`

Calculates double-dummy tables for multiple PBN deals with optional par scores.

**Parameters:**
- `deals_pbn` (list[str]): List of PBN strings
- `mode` (int, default=-1): Par vulnerability / calculation mode
    - `-1`: Disable par calculation (par_results will be empty list)
    - `0`: None vulnerable
    - `1`: Both vulnerable
    - `2`: North-South vulnerable
    - `3`: East-West vulnerable
- `trump_filter` (sequence[int], default=(0,0,0,0,0)): Strains to skip (0=include, 1=skip)
  - Accepts any sequence type (list, tuple, etc.)
  - Order: [♠, ♥, ♦, ♣, NT]

**Returns:**
- dict with keys: `no_of_boards`, `tables`, `par_results` (empty list when mode=-1)
  - `tables[i]['res_table']` is always a 5x4 matrix in fixed strain order:
    `[♠, ♥, ♦, ♣, NT]`
  - `res_table[strain][hand]` gives tricks for that strain/hand
  - If a strain is skipped by `trump_filter`, that row is present but zero-filled

**Example:**
```python
from dds3 import calc_all_tables_pbn

deals = [
    "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3",
    "N:AK.234.456.789TJQ W:QJ.AKQJ.789.234 E:T9.T9.TJ.AK S:8765.8765.AKQJ32.6",
]

result = calc_all_tables_pbn(deals, mode=0)
print(f"Boards analyzed: {result['no_of_boards']}")
print(f"Par results: {result['par_results']}")
```

#### `par(table_results, vulnerable=0)`

Calculates par contracts and scores for a given double-dummy table.

**Parameters:**
- `table_results` (dict): DD table result with key:
  - `res_table`: 5x4 array from `calc_dd_table`
- `vulnerable` (int, default=0): Vulnerability (0=none, 1=both, 2=NS, 3=EW)

**Returns:**
- dict with keys: `par_contracts_string`, `par_score`

**Example:**
```python
from dds3 import calc_dd_table, par

table_deal = {
    "cards": [
        [0x7FFC, 0, 0, 0],
        [0, 0x7FFC, 0, 0],
        [0, 0, 0x7FFC, 0],
        [0, 0, 0, 0x7FFC],
    ],
}

dd_result = calc_dd_table(table_deal)
par_result = par(dd_result, vulnerable=0)
print(f"Par: {par_result['par_score']}")
print(f"Contract: {par_result['par_contracts_string']}")
```

## Card Representation

### Binary Format (remain_cards)
Cards are represented using DDS rank bitmasks shifted left by 2:
- 2 = `0x0004`
- 3 = `0x0008`
- ...
- A = `0x4000`

Examples:
- `0x0004` = 2 only
- `0x0008` = 3 only
- `0x4000` = A only
- `0x7FFC` = All cards (A-K-Q-J-T-9-8-7-6-5-4-3-2)

The `remain_cards` array format is `[hand][suit]`:
```python
remain_cards = [
    [north_spades, north_hearts, north_diamonds, north_clubs],
    [east_spades,  east_hearts,  east_diamonds,  east_clubs],
    [south_spades, south_hearts, south_diamonds, south_clubs],
    [west_spades,  west_hearts,  west_diamonds,  west_clubs],
]
```

### PBN Format
Portable Bridge Notation format: `"N:AK.234.456.789TJQ W:QJ.AKQJ.789.234 E:T9.T9.TJ.AK S:8765.8765.AKQJ32.6"`

Format: `[Seat]:[Spades].[Hearts].[Diamonds].[Clubs]`
- Seats: N (North), E (East), S (South), W (West)
- Cards: 2-9, T (10), J, Q, K, A (highest)
- Dots separate suits
- Omitted cards belong to other players

## Validation and Error Handling

### Input Validation
The Python interface validates all inputs:
- Suit values: 0-3 for bids, 0-4 for trump
- Rank values: 0 or 2-14 for trick cards (`0` means unset)
- Card bitmasks: 0..0x7FFC
- Array dimensions: 4x4 for card arrays, 5x4 for results
- PBN format: Must be valid PBN notation

### Exception Handling
- `ValueError`: Invalid input parameters (bounds, format)
- `RuntimeError`: DDS solver errors (e.g., invalid board state)
- `KeyError`: Missing required dictionary keys

**Example:**
```python
from dds3 import solve_board

# This will raise ValueError for invalid suit
try:
    deal = {
        "trump": 5,  # Invalid: must be 0-4
        "first": 0,
        "remain_cards": [[0, 0, 0, 0]] * 4,
        "current_trick_suit": (0, 0, 0),
        "current_trick_rank": (0, 0, 0),
    }
    solve_board(deal)
except ValueError as e:
    print(f"Validation error: {e}")

# This will raise RuntimeError if DDS detects invalid board state
try:
    deal = {
        "trump": 0,
        "first": 0,
        "remain_cards": [[0, 0, 0, 0]] * 4,  # Empty board
        "current_trick_suit": (0, 0, 0),
        "current_trick_rank": (0, 0, 0),
    }
    solve_board(deal)
except RuntimeError as e:
    print(f"DDS error: {e}")
```

## Performance Considerations

- The extension is thread-safe for most operations
- Use `thread_index` parameter for multi-threaded solving (0-based index)
- For batch processing, prefer `calc_all_tables_pbn` over multiple `solve_board_pbn` calls
- Consider using optimized builds (`bazel build -c opt`) for performance-critical code

## Building from Source

### macOS
```bash
# Install prerequisites
brew install bazelisk

# Build
bazelisk build -c opt //python:_dds3
```

The Bazel build downloads the pinned LLVM toolchain automatically via
`bazel-contrib/toolchains_llvm`, so no separate Homebrew LLVM installation is
required.

### Linux
```bash
# Install prerequisites (Ubuntu/Debian)
sudo apt-get install build-essential python3-dev

# Build
bazel build -c opt //python:_dds3
```

On Linux hosts where a pinned LLVM toolchain is configured (for example,
`linux-x86_64`), Bazel also resolves that pinned LLVM toolchain automatically
during the build. On other Linux architectures, Bazel falls back to its
default C++ toolchain resolution (typically using the system compiler).

### Windows
Currently not officially supported. Contributions welcome!

## Troubleshooting

### Import Error: `ModuleNotFoundError: No module named 'dds3'`
Ensure PYTHONPATH includes both source and built extension:
```bash
export PYTHONPATH=python:bazel-bin/python
```

### "incompatible function arguments"
Check that list/array types match expectations:
- `trump_filter` accepts any sequence (list, tuple, etc.)
- `current_trick_suit` and `current_trick_rank` accept both lists and tuples
- `cards` and `remain_cards` must be lists of lists

### DDS errors
Refer to DDS error codes in the C++ library documentation. Common ones:
- Error -2: Invalid board state (e.g., wrong card count)
- Error -14: Wrong number of remaining cards

## Contributing

For questions, bug reports, or feature requests, please open an issue on GitHub.

## License

The Python interface follows the same license as the DDS library.
