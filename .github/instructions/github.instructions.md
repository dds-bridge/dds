---
name: GitHub Workflow Rules
alwaysApply: false
---

# GitHub Workflow Rules

## Overview
This project uses GitHub as its primary version control and collaboration platform. All code contributions, fixes, and features must go through a pull request (PR) workflow.
You have access to an MCP server named **`github`** (running `mcp-github`) which can create branches, push commits, and open PRs directly from the Continue.dev environment. See `.vscode/mcp.json` for configuration.

## Branching Strategy
- **Default branch:** `main`
- Always create a new branch for changes.  
  Format:  
  - `feature/<short-description>` for new features  
  - `fix/<short-description>` for bug fixes  
  - `chore/<short-description>` for maintenance
  - `refactor/<short-description>` for refactoring
- Branch names must be lowercase and use hyphens instead of spaces.

## Pull Request Rules
1. **Always** open a PR for changes — no direct commits to `main`.
2. Include:
   - A clear title describing the change
   - A concise but informative description
3. Assign at least **one reviewer** from the core team.
4. All PRs must pass:
   - CI build
   - All unit and integration tests
   - Any lint/format checks
5. Small PRs are preferred — keep them focused on one logical change.

## Tooling
Use the **`github`** MCP server (configured in `.vscode/mcp.json`) to:
   - Create branches
   - Commit and push changes
   - Open pull requests
   - Check PR status
## Example MCP Server Commands
You can instruct Continue.dev to:
- `Use the github MCP server to create a new branch called fix/memory-leak`
- `Push the current branch and open a pull request titled "Fix memory leak in cache manager"`
- `List open pull requests for this repo`
- `Merge PR #45 after approval`

## Automation
- CI/CD runs automatically for all PRs.
- Approved PRs can be merged by maintainers.
- Use **squash merging** to keep history clean.

## Notes for Continue.dev
- You are allowed to automate branch creation and PR submission using the `github` MCP server.
- If multiple PRs are required, ensure each is isolated to its own branch.
- You may request human review before merging.
```
