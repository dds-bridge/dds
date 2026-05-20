Repository-level instructions for GitHub Copilot and agents

Purpose
- These files provide repository-scoped guidance that Copilot, Copilot Chat, and other repository-aware agents can surface before answering or making changes.

Files
- `bazel.instructions.md` — Bazel/build guidance
- `cpp.instructions.md` — C++/clangd/Serena guidance
- `git.instructions.md` — Git/MCP usage
- `github.instructions.md` — Branching and PR rules

Usage
- Agents should read these files to apply repository-specific context. Humans can use this README as a quick pointer.

Contact
- If you need changes to these instructions, edit the corresponding file and open a PR.
