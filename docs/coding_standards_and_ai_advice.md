# Coding agents and coding standards

Coding agents are becoming increasingly powerful and which tool and model that scores best in test changes if not by the week at least on a monthly basis. These recommendations and instructions for coding were collected in the first quarter of 2026 when the bulk of the work to modernise the codebase for release 3.0.0 was carried out.

Configurations for MCP (Model Content Protocol) servers are not included. MCP servers add a lot of power, but can also expose severe vulnerabilities. How they should be deployed is definitely not one size fits all.

## Coding Standards

Keeping a consistent coding style is more important than ever as it helps both humans and coding agents. Preferred styles are document in the `.github/instructions directory`, which is where Github Copilot looks for its permanent instructions.

1. [C++](../.github/instructions/cpp.instructions.md)
2. [Bazel](../.github/instructions/bazel.instructions.md)
3. [Git](../.github/instructions/git.instructions.md)
4. [GitHub](../.github/instructions/github.instructions.md)

## Recommended tools for coding agents

### clangd
https://clangd.llvm.org

Language servers are well known to most integrated development environments but coding agents are typically not able to interact with them directly. There are several MCP wrappers that can surface a language server to a coding agent. This is, however, serving raw JSON responses that the coding agent has to interpret and reason around. If - as is highly recommended - you run serena then prefer to let serena handle the integration with language servers.

### Serena
https://github.com/oraios/serena

Probably the most important tool at the time of writing. Serena provides semantic analysis and instructions to help coding agents stay on target. 

### Code Context Engine
https://github.com/elara-labs/code-context-engine

Creates and maintains an index of the codebase. This means that a coding agent often can read only the relevant part of a file instead of searching all the content of several files. 

As a side note, I find it interesting that vector embeddings were removed from Claude Code. My amatuer understanding is that Serena tells the agent what the code is doing and code context enginer where the interesting code. This differs from vector embeddnings which tells the coding agent which parts of the codebase looks similar. Knowing that calls to write to the database looks similar is not useful, but the information where they are and what they write is.

## Documentation and code completion

Code documentation for DDS3 is generated through doxygen, which extracts formatted comments from the source code. The
build command

```
bazelisk build //:doxygen_docs
```

generates a stack of local html pages. Open `doxygen_output/html/pages.html` to read the documentation.

### Extracting compile_commands.json

Language servers, such as clangd, rely on a file called `compile_commands.json` which contains information about how project artefacts are build. 
https://github.com/kiron1/bazel-compile-commands

https://github.com/hedronvision/bazel-compile-commands-extractor