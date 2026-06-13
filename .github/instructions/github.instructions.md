---
name: GitHub Workflow Rules
alwaysApply: false
---

# GitHub Workflow Rules

## Overview
This project uses GitHub as its primary version control and collaboration platform. All code contributions, fixes, and features must go through a pull request (PR) workflow.

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
Use `git` for local version control, and the GitHub CLI (`gh`) for GitHub operations:
   - Create branches
   - Commit and push changes
   - Open pull requests
   - Check PR status

## Automation
- CI/CD runs automatically for all PRs.
- Approved PRs can be merged by maintainers.

```
