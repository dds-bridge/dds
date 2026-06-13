# Coding agents and coding standards

Coding agents are improving quickly, and the best tool or model for a task can change from one month to the next. This note collects the coding guidance and tooling recommendations that were assembled during the first quarter of 2026, when most of the modernisation work for release 3.0.0 was completed.

This document does not prescribe a specific MCP server setup. MCP servers can be powerful, but they also introduce security risks, so the right deployment strategy depends on the environment.

## Coding Standards

Consistent style matters even more when both humans and coding agents are reading and editing the same code. The preferred conventions are documented in the `.github/instructions` directory, which is where GitHub Copilot looks for its persistent instructions.

1. [C++](../.github/instructions/cpp.instructions.md)
2. [Bazel](../.github/instructions/bazel.instructions.md)
3. [Git](../.github/instructions/git.instructions.md)
4. [GitHub](../.github/instructions/github.instructions.md)

## Recommended Tools for Coding Agents

### clangd

https://clangd.llvm.org

Language servers are familiar to most IDE users, but coding agents usually cannot interact with them directly. MCP wrappers can expose a language server to an agent, but they often do so by forwarding raw JSON responses that still need to be interpreted. If you also run Serena, prefer to let Serena handle the language-server integration.

### Serena

https://github.com/oraios/serena

Serena is the most useful tool in this workflow. It provides semantic analysis and retrieval features that help coding agents stay focused on the right parts of the codebase and understand the structure of the language they are working in.

### Code Context Engine

https://github.com/elara-labs/code-context-engine

Code Context Engine builds and maintains an index of the codebase, which lets a coding agent inspect the relevant parts of a file without scanning unrelated content across many files.

One observation from this tooling landscape is that different systems solve different problems. Serena helps explain what the code is doing, while a code index helps locate where the interesting code lives. That is more useful than simply surfacing syntactically similar code, which is often not enough to guide a change.

## Documentation and Code Completion

Code documentation for DDS3 is generated with Doxygen, which extracts formatted comments from the source code. Run the following command to generate the local documentation:

    bazelisk build //:doxygen_docs

The generated HTML pages are written under `bazel-bin/doxygen_output/html/` (and packaged as `bazel-bin/doxygen_docs.zip`). Open `bazel-bin/doxygen_output/html/index.html` to read the documentation.

### Extracting compile_commands.json

Language servers such as clangd rely on a `compile_commands.json` file, which describes how the project is built.

On macOS or Linux, the following command generates that file when the `bazel-compile-commands` utility is installed:

```
bazel-compile-commands //...
```

https://github.com/kiron1/bazel-compile-commands

An alternative is Hedron Compile Commands, which can be integrated into the Bazel build. It appears to be less actively maintained, but it is still worth knowing about:

https://github.com/hedronvision/bazel-compile-commands-extractor