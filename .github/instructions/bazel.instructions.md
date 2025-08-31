---
name: Bazel Build System Rules (MCP-Integrated)
languages: ["bazel", "starlark", "build", "bazelbuild"]
alwaysApply: false
---

# Bazel Build System Rules (MCP-Integrated)


## MCP Server
- **Name:** `bazel` (see `.vscode/mcp.json` for configuration)
- **Type:** `mcp-bazel`
- **Capabilities:**
  - Parse & validate BUILD/.bzl files
  - Build graph analysis
  - Target query & navigation
  - Auto-generate build commands

> **Tip:** Whenever you edit a BUIL or .bzl file, run a `bazel` MCP command first to catch syntax or dependency issues.

## Build Guidelines
| Rule     | What it means                       | Why it matters                           |
|----------|-------------------------------------|-------------------------------------------|
| Explicit deps | List every dependency in `deps = [...]` | Prevents hidden transitive pulls            |
| No wildcards | Avoid `glob([...])` unless absolutely needed | Keeps target graph deterministic             |
| Target naming | `<component>_<purpose>_<type>`       | Easier to read & search                     |
| Idiomatic Starlark | Prefer built-in rules over custom macros | Reduces maintenance                        |
| Consistent formatting | Follow `.bazelrc` & `.bzlformat`   | Keeps codebase uniform                      |
## Best Practices
- Use `cc_library`, `cc_binary`, `cc_test` for C++ targets.
- Keep test suites small & fast.
- Restrict visibility (`visibility = ["//visibility:private"]`) when appropriate.
- Run `bazel build //...` & `bazel test //...` locally before pushing.
- Document any non-trivial macros or build logic in comments.

## Working with the MCP Server
```bash
# Validate a BUILD file
bazel validate //path/to:BUILD

# Query dependencies of a target
bazel deps //my:target

# Generate a build command for a target
bazel build //my:target
```

