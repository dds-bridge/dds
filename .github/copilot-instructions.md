# DDS (Double Dummy Solver) Repository Instructions

## Project Overview

**DDS** is a double-dummy solver for bridge hands, written in C++. It calculates the maximum number of tricks a partnership can take for a given contract. The library:

- Supports multi-threading (OpenMP, GCD, STL, PPL implementations)
- Works on Windows, Linux, and macOS
- Provides both C and C++ APIs
- Uses transposition tables for efficient solving
- Is actively being modernized to version 3.0 with improved memory management and C++20 features

**Current State**: Work in progress towards version 3.0. The project has been refactored from the legacy 2.9.0 codebase to use modern C++ features, Bazel build system, and modular architecture.

**Languages & Tools**: C++20, Bazel 7.x, GoogleTest for testing

**Repository Size**: ~50 C++ source files in library/src, ~20 test files, several examples

## Building and Testing

**ALWAYS use Bazel** for building and testing. Legacy Makefiles exist but are deprecated.

### Essential Commands

```bash
# Build everything (required before testing or running)
bazel build //...

# Run all tests
bazel test //...

# Build specific target
bazel build //library/src:dds

# Run specific test
bazel test //library/tests:dtest

# Build with optimization
bazel build -c opt //...

# Build with debug symbols
bazel build -c dbg //...

# Build with specific flags (example: enable ASAN)
bazel build --define=asan=true //...
```

### Build Time Expectations
(On a modern multi-core system with 8+ cores and SSD)
- Initial build (cold cache): 2-5 minutes
- Incremental builds: 10-30 seconds
- Full test suite: 1-3 minutes
- Individual test: 2-10 seconds

Note: Times may vary significantly based on hardware and network conditions.

### Before Making Changes
ALWAYS run these commands first to establish baseline:
```bash
bazel build //...
bazel test //...
```
This ensures any pre-existing issues are not attributed to your changes.

### After Making Changes
ALWAYS validate your changes:
```bash
# Build and test
bazel build //...
bazel test //...
```

## Project Layout and Architecture

### Directory Structure
```
<repository-root>/
├── .github/
│   ├── copilot-instructions.md     # This file
│   ├── instructions/                # Path-specific instructions
│   │   ├── cpp.instructions.md     # C++ style guide
│   │   ├── bazel.instructions.md   # Bazel build rules
│   │   ├── github.instructions.md  # PR/workflow rules
│   │   └── git.instructions.md     # Git usage
│   └── workflows/
│       ├── ci_linux.yml            # Linux CI pipeline
│       └── ci_macos.yml            # macOS CI pipeline
├── library/
│   ├── src/                        # Main source code
│   │   ├── api/                    # Public C/C++ API headers
│   │   │   ├── dds.h              # Internal header (data structures, included by dds.hpp)
│   │   │   ├── dll.h              # C API
│   │   │   └── solve_board.hpp    # C++ solver interface
│   │   ├── solver_context/         # Solver state management
│   │   ├── trans_table/            # Transposition table implementation
│   │   ├── system/                 # Threading/memory/system utilities
│   │   ├── moves/                  # Move generation
│   │   ├── heuristic_sorting/      # Move ordering heuristics
│   │   ├── lookup_tables/          # Precomputed tables
│   │   ├── utility/                # Helper functions
│   │   ├── ab_search.cpp/hpp       # Alpha-beta search core
│   │   ├── solve_board.cpp/hpp     # Main solve entry points
│   │   ├── par.cpp                 # Par score calculations
│   │   └── BUILD.bazel             # Build configuration
│   └── tests/                      # Test suites
│       ├── dtest.cpp               # Main test driver
│       ├── system/                 # System tests
│       └── regression/             # Regression tests
├── examples/                       # Example programs
│   ├── par.cpp
│   ├── solve_all_boards.cpp
│   └── BUILD.bazel
├── hands/                          # Test hand files
│   └── list100.txt                 # 100 test hands
├── docs/                           # Documentation
├── copilot/                        # Copilot-related files (plans, tasks)
├── BUILD.bazel                     # Root build file
├── MODULE.bazel                    # Bazel module definition
├── .bazelrc                        # Bazel configuration
├── .clang-tidy                     # C++ linter configuration
└── README.md
```

### Key Files to Know

**Build Configuration**:
- `BUILD.bazel` - Root build targets, exports `//library/src:dds` library
- `library/src/BUILD.bazel` - Main library build rules
- `MODULE.bazel` - External dependencies (GoogleTest, etc.)
- `.bazelrc` - Compiler flags, build modes

**Public API** (use these include paths):
- `#include <api/dll.h>` - C API
- `#include <dds/dds.hpp>` - C++ API (includes api/dll.h and api/solve_board.hpp)
  - Note: The file is at `library/src/dds.hpp`, but the `dds/` prefix is added by Bazel's `include_prefix` directive in the build rules

**Public API Files** (actual file locations):
- `library/src/api/dll.h` - C API declarations
- `library/src/dds.hpp` - C++ API wrapper
- `library/src/api/dds.h` - Internal header (data structures, included by dds.hpp)
- `library/src/api/solve_board.hpp` - C++ solver interface

**Core Solver**:
- `library/src/ab_search.cpp` - Alpha-beta search implementation
- `library/src/solve_board.cpp` - Main solving functions
- `library/src/quick_tricks.cpp` - Quick tricks calculation
- `library/src/later_tricks.cpp` - Later tricks analysis

**Configuration**:
- `.clang-tidy` - Linter rules (runs in CI)
- `.github/instructions/*.instructions.md` - Coding standards

### Major Architectural Components

1. **Solver Context** (`solver_context/`) - Manages solver state, transposition tables, threading resources
2. **Transposition Table** (`trans_table/`) - Caches previously solved positions
3. **Move Generation** (`moves/`) - Generates legal moves for current position
4. **Heuristic Sorting** (`heuristic_sorting/`) - Orders moves for efficient search
5. **AB Search** (`ab_search.cpp`) - Core alpha-beta negamax search
6. **System Layer** (`system/`) - Threading, memory, platform abstractions

### Dependencies That Aren't Obvious

- The project uses a **custom memory manager** for transposition tables
- Threading is abstracted through `system/` layer (supports OpenMP, GCD, STL)
- **API layer** (`api/`) provides both C and C++ interfaces to the same solver
- Tests depend on `testable_dds` target which exposes internal symbols

## CI/CD Pipeline

### GitHub Actions Workflows

**On Every PR** (must pass before merge):
1. **Build** - `bazel build //...` on Linux and macOS
2. **Test** - `bazel test //...` on both platforms
3. **Lint** - clang-tidy checks (configured in `.clang-tidy`)

**Workflow Files**:
- `.github/workflows/ci_linux.yml` - Ubuntu 22.04, latest Bazel
- `.github/workflows/ci_macos.yml` - macOS 13, latest Bazel

### How to Debug CI Failures

If CI fails:
1. Check the workflow logs in GitHub Actions
2. Reproduce locally with exact CI command:
   ```bash
   bazelisk build //...
   bazelisk test //...
   ```
3. For clang-tidy issues, run:
   ```bash
   # Generate compile_commands.json
   bazel build //... --config=clang-tidy
   # Run clang-tidy manually
   clang-tidy library/src/your_file.cpp
   ```

### Additional Validation Steps

Before finalizing changes, optionally run:
```bash
# Check for memory leaks (Linux only)
bazel build --define=asan=true //...
bazel test --define=asan=true //...

# Performance test with real hands
bazel build //library/tests:dtest
./bazel-bin/library/tests/dtest -f hands/list100.txt -s solve -n 4
```

## Coding Standards and Conventions

**CRITICAL**: Follow the rules in `.github/instructions/cpp.instructions.md` exactly. Key points:

### Naming Conventions
- **Types** (classes, structs, enums): `PascalCase` (e.g., `SolverContext`, `MoveGenerator`)
- **Functions**: `snake_case` (e.g., `solve_board()`, `calculate_moves()`)
- **Variables**: `snake_case` (e.g., `trick_count`, `best_move`)
- **Member variables**: `snake_case` with trailing underscore (e.g., `data_`, `cache_size_`)
- **Constants**: `PascalCase` (e.g., `MaxTricks`, `DefaultThreads`)
- **Macros**: `ALL_CAPS` (e.g., `DDS_VERSION`)

### Formatting
- **Indentation**: 4 spaces (no tabs)
- **Braces**: 
  - **Allman style** (opening brace on new line) for functions, classes, structs, enums, namespaces:
    ```cpp
    void my_function()
    {
        // body
    }
    ```
  - **K&R style** (opening brace on same line) for control statements:
    ```cpp
    if (condition) {
        do_something();
    }
    ```
- **Standard**: C++20 (`-std=c++20`)
- **Header guards**: Use `#pragma once`
- **Include order**: System headers, then project headers

### Modern C++ Requirements
- Use **RAII** for all resource management
- Prefer **smart pointers** (`std::unique_ptr`, `std::shared_ptr`) over raw pointers
- Use **`const`** everywhere appropriate
- Use **`enum class`** not plain `enum`
- Prefer **exceptions** over error codes for error handling
- Use **`std::optional`** for nullable values
- Mark compile-time constants with **`constexpr`**

### Testing
- Use **GoogleTest** framework
- Test files in `library/tests/`
- Cover normal, edge, and failure cases
- Keep tests fast and focused

## Common Pitfalls and Workarounds

### Build Issues

**Problem**: "error: 'X' was not declared in this scope"
- **Cause**: Missing include or wrong include path
- **Fix**: Check include paths use Bazel workspace format: `#include "library/src/..."` or `#include <api/...>`

**Problem**: Bazel build hangs or is very slow
- **Cause**: Large compilation unit or full rebuild
- **Fix**: Use `bazel build --jobs=4` to limit parallelism, or be patient (initial builds are slow)

**Problem**: Linker errors about multiple definitions
- **Cause**: Inline functions in headers without `inline` keyword
- **Fix**: Add `inline` keyword or move to `.cpp` file

### Test Issues

**Problem**: Tests fail with "Segmentation fault"
- **Cause**: Usually memory management bug or uninitialized variable
- **Fix**: Run with ASAN: `bazel test --define=asan=true //library/tests:failing_test`

**Problem**: Tests fail with different results than expected
- **Cause**: Possibly heuristic sorting or transposition table issues
- **Fix**: Check if using correct configuration flags, verify hand input format

### Runtime Issues

**Problem**: Performance much slower than expected
- **Cause**: Debug build or single-threaded execution
- **Fix**: Build with `bazel build -c opt //...` and ensure threading is enabled

**Problem**: Out of memory errors
- **Cause**: Transposition table too large
- **Fix**: Call `SetResources()` to limit memory, reduce thread count

## Important Facts for Agents

### Trust These Instructions
These instructions are comprehensive and tested. **Only search the codebase if**:
- Information here is incomplete for your specific task
- You find a discrepancy between instructions and reality
- You need to understand specific implementation details

### Making Changes
1. **Minimal changes**: Modify only what's necessary
2. **Test immediately**: Run `bazel test //...` after each logical change
3. **Follow patterns**: Match existing code style exactly
4. **Update docs**: If changing public API, update corresponding documentation

### File Locations Quick Reference
- Need to change API? → `library/src/api/`
- Need to fix solver logic? → `library/src/ab_search.cpp` or `library/src/solve_board.cpp`
- Need to add test? → `library/tests/`
- Need to add example? → `examples/`
- Need to update build? → `BUILD.bazel` or `library/src/BUILD.bazel`

### Don't
- Don't change test expected values without verifying correctness
- Don't add new dependencies without strong justification
- Don't disable warnings or errors in build files
- Don't use raw pointers for ownership (use smart pointers)
- Don't add `using namespace std;` in headers

### Always
- Always run build and test before committing
- Always follow the naming conventions exactly
- Always use Bazel, never Makefiles
- Always check `.github/instructions/*.instructions.md` for file-specific rules
- Always create a PR for changes (use the github MCP server)
- Always format code consistently with existing files
