## Introduction to DDS3

DDS3 is a double dummy solver for bridge hands. It is a drop-in replacement for DDS 2.9.0 which has been the leading solver for many years, based on the initial work of Bo Haglund in 2006 and the previous modernisation by Søren Hein in 2014.  With Søren's encouragement, I have updated the official release in order to retain continuity for the user base, and I have been added as an administrator.

DDS3 is a double dummy solver for bridge hands. Version 3.0 uses the same
search algorithm as version 2.x, but the source code has been modernised. The
project has been split into several subcomponents, each responsible for a
specific part of the search algorithm. This modularisation makes the codebase
easier to read and reason about, which helps not only humans but also modern
coding agents. Throughout the codebase, you will find evidence that Claude
Code and GitHub Copilot have made significant contributions to the
modernisation.

There are build scripts for macOS, Linux, and Windows but I have myself only used the library on macOS.

Plenty of people like to use a double dummy solver for statistical analysis, typically in Python. I have added a bare-bones Python interface to the solver.

### Motivation for creating version 3

I wanted to use DDS 2.9.0 for training declarer models, but memory management
prevented me from solving several hands in parallel while also preserving the
transposition table. Preserving the table is required when making repeated
calls for the same hand while training a declarer model against double-dummy
perfect defenders.

To address these issues, and also take advantage of modern C++ features, I had to update the project to a more modular build structure. This allowed me to create a library with dynamic memory management.

Martin Nygren, May 2026

## Version 3.0 Documentation

[C++ Interface](docs/c++_interface.md)

[Python Interface](docs/python_interface.md)

[Legacy C Interface](docs/legacy_c_api.md)

[Migrating to the modern API](docs/api_migration.md)

[Notes about the build system](docs/BUILD_SYSTEM.md)


## Version 3.0 Release Status

Current baseline for this branch:

- C++ toolchain uses `bazel-contrib/toolchains_llvm` pinned to LLVM 20.1.8 for
    `darwin-aarch64` and LLVM 21.1.8 for `linux-x86_64`.
- Full non-ASAN validation passes with `bazelisk test //...`.
- Doxygen docs target is available as a manual developer target (`//:doxygen_docs`)
    and requires `doxygen` and `zip` on `PATH`.

## 2.9 Documentation

You can find the original [README](doc/README_2_9_0.md) and descriptions of the search algorithm in the doc folder.

