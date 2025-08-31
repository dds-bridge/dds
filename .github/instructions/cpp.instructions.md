---
name: C++ Development Rules (clangd MCP)
languages: ["cpp", "c++", "h", "hpp", "cc"]
alwaysApply: false
---

# C++ Development Rules with clangd MCP Integration

## MCP Server
- **Name:** `cpp` (see `.vscode/mcp.json` for configuration)
- **Type:** clangd integration
- **Status:** Must be running for all C++ work
- **Name:** `serena` (see `.vscode/mcp.json` for configuration)
- **Type:** semantic code retrieval and editing (see `.vscode/mcp.json`)
- **Status:** Must be running for all C++ work

## Tooling
| Feature | What it does | When to use |
|---------|--------------|-------------|
| Diagnostics | Real-time error reporting | Before building |
| Fix suggestions | Auto-apply clangd fixes when diagnostics appear |  |
| Code completion | Signature help | While typing |
| Go-to-definition / references | Navigate code during refactoring |  |
| Rename-symbol | Cross-file rename | Renaming identifiers |
| Organize-includes | Remove unused includes  |  Before committing |
| clang-tidy | Style & safety checks in the CI pipeline |  Before committing |

When suggesting code:
- Consider that the development environment has advanced semantic code analysis through Serena
- Follow patterns that work well with LSP-based tools (proper symbol definitions, clear function signatures)
- Maintain good code structure since Serena works best with well-organized, modular code
- Ensure changes are compatible with clangd analysis

Code organization preferences:
- Favor clear symbol-level organization (Serena operates at the symbol level)
- Use meaningful function and class names (aids semantic retrieval)
- Maintain proper C++ namespacing and module structure

> **Tip:** If a clangd command fails, fall back to manual analysis and report the issue.

## Workflow
1. **Diagnostics** – run first to catch compile errors before any changes.
2. **Refactor** – use *find-references* to locate all usages during refactoring.
3. **Rename** – use *rename-symbol* for safe cross-file changes when renaming identifiers.
4. **Include management** – run *organize-includes* before committing to remove unused includes.
5. **Post-change diagnostics** – verify no new errors after code changes.

## Coding Guidelines
- **Standard:** C++20 (`-std=c++20`)
- **RAII** – manage resources via destructors.
- **Smart pointers** – prefer `std::unique_ptr` / `std::shared_ptr`.
- **Const-correctness** – mark data/functions const where possible.
- **No `using namespace` in headers.**
- **Google C++ Style Guide** – follow naming, formatting, and layout.
- **Enums** – use `enum class`.
- **Functions** – keep short; extract helpers if > 20 lines.
- **Variables** – snake_case; classes – PascalCase.
- **Header guards** – `#pragma once` or traditional guards.
## Safety
- Initialize all variables.
- Check bounds on container access.
- Prefer exceptions over error codes when appropriate.
- Use `std::optional` for nullable values.
- Mark compile-time constants with `constexpr`.
## Documentation
- Public APIs: Doxygen-style comments.
- Explain non-obvious design choices.
- Provide usage examples for complex interfaces.

## Testing
- Use GoogleTest.
- Cover normal, edge, and failure cases.
- Keep tests fast and focused.

## Build System
- **Bazel** is the sole build system.
- Generate `BUILD` files automatically.
- Compiler flags: `-Wall -Wextra -Werror`.
- Ensure `WORKSPACE` is up-to-date.
---

**Reminder:** All changes should be reviewed via a pull request. Use the `github` MCP server to create remote branches and open PRs.
```
