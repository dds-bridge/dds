---
name: Git Commands
languages: []
alwaysApply: false
---


## Common Operations

| Action | MCP Command | Example |
|--------|-------------|---------|
| Commit | `git commit` | `git commit -m "Add new helper"` |
| Push | `git push` | `git push origin <branch>` |
| Branch status | `git branch` | `git branch --show-current` |
| Repo info | `git repo` | `git repo --owner` |
| Diff vs base | `git diff` | `git diff main` |


## Workflow Integration
1. **Create a branch** (see `github.instructions.md` for naming conventions).  
2. **Make changes** locally.  
3. **Commit** with a clear message.  
4. **Push** the branch.  
5. **Open a PR** via the `github` MCP server (see `github.instructions.md`).  

---