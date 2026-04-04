## Introduction to DDS3

DDS3 is a double dummy solver for bridge hands. Version 3.0 uses the same
search algorithm as version 2.x, but the source code has been modernised. The
project has been split into several subcomponents, each responsible for a
specific part of the search algorithm. This modularisation makes the codebase
easier to read and reason about, which helps not only humans but also modern
coding agents. Throughout the codebase, you will find evidence that Claude
Code and GitHub Copilot have made significant contributions to the
modernisation.

DDS3 is currently built with C++20. I am hoping to move towards C++ modules and
C++23 in the next couple of months.

There are build scripts for macOS and Linux, but not yet for Windows. The
reason is simple: I do not currently have access to a Windows machine. Pull
requests to support Windows builds are very welcome.

### Motivation for creating version 3

I wanted to use DDS 2.9.0 for training declarer models, but memory management
prevented me from solving several hands in parallel while also preserving the
transposition table. Preserving the table is required when making repeated
calls for the same hand while training a declarer model against double-dummy
perfect defenders.

To address these issues, and also take advantage of modern C++ features, I had to update the project to a more modular build structure. This allowed me to create a library with dynamic memory management.

It soon became obvious that merging these changes back into the original repository
would not be feasible. 

Martin Nygren, April 2026

## Version 3.0 Release Status

Current baseline for this branch:

- C++ toolchain uses `bazel-contrib/toolchains_llvm` pinned to LLVM 21.1.8 for
    `darwin-aarch64` and `linux-x86_64`.
- Full non-ASAN validation passes with `bazelisk test //...`.
- Doxygen target is enabled via Bazel (`//:doxygen_docs`).

Recommended pre-release verification:

```bash
bazelisk shutdown
bazelisk test //... --test_output=errors --test_verbose_timeout_warnings
bazelisk build //...
bazel build //:doxygen_docs
```

### Sanitizer Status

AddressSanitizer is enabled through `.bazelrc` (`--config=asan`). On macOS,
ASAN runtime behavior can still vary by loader and sandbox context (especially
for Python extension tests). Treat ASAN on macOS as a separate gate and verify
it explicitly before final release tagging.

### Original project
================

You can find the original project [here](https://github.com/dds-bridge/dds) and
below is the introduction and credits for DDS 2.9.0.

Introduction
============
DDS is a double-dummy solver of bridge hands.  It is provided as a Windows DLL and as C++ source code suitable for a number of operating systems.  It supports single-threading and multi-threading  for improved performance.

DDS offers a wide range of functions, including par-score calculations.

Please refer to the [home page](http://privat.bahnhof.se/wb758135) for details.

The current version is DDS 2.9.0, released in August 2018 and licensed under the Apache 2.0 license in the LICENSE FILE.

Release notes are in the ChangeLog file.

(c) Bo Haglund 2006-2014, (c) Bo Haglund / Soren Hein 2014-2018.


Credits
=======
Many people have generously contributed ideas, code and time to make DDS a great program.  While leaving out many people, we thank the following here.

The code in Par.cpp for calculating par scores and contracts is based on Matthew Kidd's perl code for ACBLmerge.  He has kindly given permission to include a C++ adaptation in DDS.

Alex Martelli cleaned up and ported code to Linux and to Mac OS X in 2006.  The code grew a bit outdated over time, and in 2014 Matthew Kidd contributed updates.

Brian Dickens found bugs in v2.7 and encouraged us to look at GitHub.  He also set up the entire historical archive and supervised our first baby steps on GitHub.

Foppe Hemminga maintains DDS on ArchLinux.  He also contributed a version of the documentation file completely in .md mark-up language.

Pierre Cossard contributed the code for multi-threading on the Mac using GDS.

Soren Hein made a number of contributions before becoming a co-author starting with v2.8 in 2014.

====

## API Documentation

DDS 3.0 provides two API levels to suit different use cases:

### Modern C++ API (Recommended for New Projects)

The modern API uses instance-scoped `SolverContext` with automatic resource management (RAII):

```cpp
#include <dds/dds.hpp>

auto main() -> int
{
    // Configure solver
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_default_mb_ = 2000;
    cfg.tt_mem_maximum_mb_ = 2000;

    // Create context (automatic cleanup on destruction)
    SolverContext ctx(cfg);

    // Solve boards
    Deal deal{};
    // ... initialize deal with cards ...
    
    FutureTricks fut{};
    SolveBoard(ctx, deal, -1, 3, 0, &fut);

    return 0;
}
```

**Benefits:**
- Automatic memory management (no manual cleanup)
- Per-context configuration (different settings per thread)
- Better multithreading (no global state contention)
- Explicit transposition table control

### Legacy C API (Backward Compatible)

The legacy API uses global state and is maintained for backward compatibility:

```c
#include <api/dll.h>

struct Deal deal = {0};
struct FutureTricks fut = {0};

SetMaxThreads(4);
SetResources(2000, 4);
SolveBoard(deal, -1, 3, 0, &fut, 0);

FreeMemory();
```

**Note:** Legacy initialization functions (`SetThreading`, `SetMaxThreads`, `SetResources`, `FreeMemory`) are deprecated but remain callable. Several of them
are no ops. See [docs/api_migration.md](docs/api_migration.md) for migration guidance.

### Migration Guide

For detailed migration examples and best practices, see:
- **[API Migration Guide](docs/api_migration.md)** - Step-by-step migration from legacy to modern API
- **[Legacy C API Reference](docs/legacy_c_api.md)** - Full documentation of deprecated functions
- **[Migration Example](examples/migration_example.cpp)** - Side-by-side legacy and modern API sample
- **[SolverContext Documentation](library/src/README_SolverContext.md)** - Modern API details

**Quick decision:**
- **New C++ projects**: Use modern API (`#include <dds/dds.hpp>`)
- **Existing C projects**: Continue with legacy API (no changes required)
- **Migration**: Follow incremental migration guide in docs/api_migration.md

## Python Interface

DDS 3.0 includes a modern Python interface for bridge hand analysis:

**Build the Python extension:**
```bash
bazel build //python:_dds3
```

**Run Python tests:**
```bash
export PYTHONPATH=python:bazel-bin/python
bazel test //python:python_interface_smoke_test
```

**Use in Python:**
```python
from dds3 import solve_board_pbn

pbn = "N:QJ6.K652.J85.T98 873.J97.AT764.Q4 K5.T83.KQ9.A7652 AT942.AQ4.32.KJ3"
result = solve_board_pbn(pbn, trump=1)  # Solve in hearts
print(f"Tricks: {result['score']}")
```

**For complete documentation, see:**
- **[Python Interface Guide](docs/python_interface.md)** - Full API reference, examples, and best practices
- **Unit Tests** - See `python/tests/` for usage examples