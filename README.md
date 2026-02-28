Work in progress towards version 3
==================================
I wanted to use DDS 2.9.0 for training declarer models but found that memory management prevented me from solving several hands in parallel whilst also preserving the transposition table. The latter is required to make several calls for the same hand training a declarer model against double dummy perfect defenders.

To address these issues, and also take advantage of modern C++ features, I had to update the project to a more modular build structure. This allowed me to create a library with dynamic memory management.

There is no version 3 yet as the code is not ready to be released. It is, however, clear that merging these changes back into the original repository will not be feasible.

Martin Nygren, October 2025

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


Overview
========

The distribution consists of the following directories.

* **src**, the source code for the library.
* **include**, where the public interface of the library is specified.
* **lib**, the place where the library file is "installed" for test purposes.
* **doc**, where the library interface is documented and the algorithms behind DDS are explained at a high level.
* **hands**, a repository for input files to the test programs.
* **test**, a test program.
* **examples**, some minimal programs showing how to interface in practice with a number of library functions.

There is a parallel distribution, [**ddd**](https://github.com/dds-bridge/ddd).  It consisting of an old driver program for DDS contributed under the GPL (not under the Apache license) by Flip Cronje, and updated by us to support the multi-threaded library file.

If you install ddd manually, put it in a directory parallel to these directories (src etc.) and then read the README file in that directory.  If you use GitHub, then dds is a sub-module.


Supported systems
=================
The DLLs work out of the box on Windows systems.  There is a single-threaded version for old Windows versions, and there is a multi-threaded version that works on all modern Windows systems.  This is the one you should use if in doubt.  

The distributed Windows DLL uses Windows multi-threading.  The code compiles on windows (see INSTALL) with at least:

* Visual C++ 2010 Express editions or later.
* The TDM-GCC/Mingw port of g++.
* g++ on Cygwin.

We have also compiled the code and/or had help from other contributors on the following systems.

* Linux Ubuntu with g++ and with OpenMP multi-threading.
* Mac OS 10.9 with g++ and with OpenMP multi-threading.  Also with clang without multi-threading.  Also with GCD multi-threading compiling with LLVM.

Here the libraries are `.a` files, not DLLs.  There are also Makefiles for shared libraries available.

Note that Apple stopped using g++ in Xcode a while back, DDS does compile using the clang compiler, but since DDS does not support pthreads multi-threading, DDS becomes single-threaded.  To get OpenMP multi-threading you need to use the Homebrew installer and do:

    brew reinstall gcc --without-multilib

The `without-multilib` is important because you won't get OpenMP otherwise, and that's the whole point.  *(Thanks to Matthew Kidd for these instructions.)*

Thanks for Pierre Cossard's contribution, the Mac port now also supports GCD multi-threading with LLVM.

There's an example .Net wrapper on https://github.com/anorsich/dds.net (not supported by us).

Usage
=====

DDS tries to figure out the available number of cores and the available memory.  Based on this, DDS calculates a reasonable number of threads to use.  The user can override this by calling the `SetMaxThreads()` or the `SetResources()` function.  In principle these functions can be called multiple times, but there is overhead associated with this, so only call it at the beginning of your program unless you really want to change the number of threads dynamically.

DDS on Windows calls SetMaxThreads itself when it is attached to a process, so you don't have to.  On Unix-like systems we use an equivalent mechanism, but we have had a report that this does not always happen in the right order of things, so you may want to call SetMaxThreads explicitly.

Docs
====
The DDS library interface is documented. You find the docs, including a Markdown version which you can read online, in the /doc folder.  The Markdown version has not been updated since v2.8.4.

Bugs
====
Version 2.9.0 has no known bugs.

Please report bugs to bo.haglund@bahnhof.se and soren.hein@gmail.com.

Version 3.0.0, preliminary change log
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

**Note:** Legacy initialization functions (`SetThreading`, `SetMaxThreads`, `SetResources`, `FreeMemory`) are deprecated but remain functional. See [docs/api_migration.md](docs/api_migration.md) for migration guidance.

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
bazel test //python/tests:*
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