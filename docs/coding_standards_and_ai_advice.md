# Coding agents and coding standards

Coding agents are becoming increasingly powerful and which tool and model that scores best in test changes if not by the week at least on a monthly basis. These recommendations and instructions for coding were collected in the first quarter of 2026 when the bulk of the work to modernise the codebase for release 3.0.0 was carried out.

Configurations for MCP servers are not included. MCP servers add a lot of power, but can also expose severe vulnerabilities. How they should be deployed is definitely not one size fits all.

## Recommended tools for coding agents

### clangd
https://clangd.llvm.org

Language servers are well known by most integrated development environments but coding agents are typically not able to interact with them directly. There are several MCP wrappers that can surface a language server to a coding agent. This is, however, surfacing raw JSON responses that the coding agent has to interpret and reason around. If - as is highly recommended - you run serena then prefer to let serena handle the integration with language servers.

### Serena
https://github.com/oraios/serena

Probably the most important tool at the time of writing.

### Code Context Engine

## Coding Standards

Keeping a consistent coding style is more important than ever as it helps both humans and coding agents. Preferred styles are document in the `.github/instructions directory`

1. [C++](../.github/instructions/cpp.instructions.md)
2. [Bazel](../.github/instructions/bazel.instructions.md)
3. [Git](../.github/instructions/git.instructions.md)
4. [GitHub](../.github/instructions/github.instructions.md)

## Documentation and code completion

### Extracting compile_commands.json

https://github.com/kiron1/bazel-compile-commands

https://github.com/hedronvision/bazel-compile-commands-extractor