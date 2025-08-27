## Project Overview

This project is a C++ library called DDS, which stands for Double Dummy Solver. It's designed to solve bridge hands, calculating the maximum number of tricks a partnership can take for a given contract. The library is written in C++ and supports multi-threading for performance. It can be used on various operating systems, including Windows, Linux, and macOS.

The library provides a C-style API, as seen in `library/src/dds.h`, with functions like `CalcDDtable`, `SolveBoard`, and `Par`. The core logic is implemented in C++ files within the `library/src` directory.

The project uses Bazel for building. The `BUILD.bazel` and `MODULE.bazel` files in the root directory define the build targets and dependencies.

## Building and Running

The project is built using Bazel. Here are the main commands:

*   **Build the library:** `bazel build //:dds`
*   **Run the tests:** `bazel test //library/tests/...`

To use the library in your own project, you would typically link against the compiled `dds` library and include the `dds.h` header file.

## Development Conventions

*   **Coding Style:** The code follows a consistent C-style, with a focus on performance and portability. Header files (`.h`) are used for declarations, and source files (`.cpp`) for implementations. The code is well-commented, explaining the purpose of functions and complex logic.
*   **API Design:** The public API is defined in `library/src/dds.h` and consists of a set of C functions. This makes it easy to interface with the library from other languages.
*   **Testing:** The project has a `library/tests` directory, indicating a commitment to testing. The tests are also built and run using Bazel.
*   **Examples:** The `examples` directory provides several small programs that demonstrate how to use the library's functions. These are a great starting point for understanding how to integrate DDS into your own applications.
