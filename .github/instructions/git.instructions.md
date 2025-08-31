---
name: Git Commands
languages: ["git"]
alwaysApply: false
---

# Git Commands (MCP‑Integrated)

## MCP Server
- **Name:** `git` (see `.vscode/mcp.json` for configuration)
- **Type:** `git-mcp`
- **Capabilities:**
  - Commit & push changes
  - Inspect current branch, repo, and owner
  - Compare changes against a base branch

> **Tip:** Use the `git` MCP server for all low‑level Git operations instead of shell commands.

## Common Operations

| Action | MCP Command | Example |
|--------|-------------|---------|
| Commit | `git commit` | `git commit -m "Add new helper"` |
| Push | `git push` | `git push origin <branch>` |
| Branch status | `git branch` | `git branch --show-current` |
| Repo info | `git repo` | `git repo --owner` |
| Diff vs base | `git diff` | `git diff mn_main` |

> Use the `git` MCP server’s JSON‑based API. For example, to create a commit you can send:  
> ```json
> {
>   "name": "git_commit",
>   "arguments": {
>     "message": "Add new helper"
>   }
> }
> ```

## Workflow Integration
1. **Create a branch** (see `github.md` for naming conventions).  
2. **Make changes** locally.  
3. **Commit** with a clear message.  
4. **Push** the branch.  
5. **Open a PR** via the `github` MCP server (see `github.md`).  

---

**Note:** All Git operations should be performed through the `git` MCP server to keep the environment consistent and to allow Continue.dev to track changes automatically.