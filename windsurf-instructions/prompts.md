# Pull Request Instructions for GitHub CLI

## Open a Pull Request via GitHub CLI

**Instruction:**

Open a pull request using the GitHub CLI (`gh`). The destination branch should be `[destination_branch]`.
Use a descriptive title and a detailed body explaining the work done.
Always use the `gh pr create` command.

**Example:**

Please open a pull request using the GitHub CLI to the branch `mn_main`.
Use a descriptive title and a summary of the documentation and Bazel migration work.
Always use the `gh pr create` command.

**What this will do:**
1. Run:
   ```sh
   gh pr create --base mn_main --fill --title "YOUR TITLE HERE" --body "YOUR DETAILED DESCRIPTION HERE"
   ```
2. Confirm and provide the pull request URL.

You can copy/paste or adapt this instruction for any future PRs!
