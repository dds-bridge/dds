**Task Title**: Commit all reorganised files  
**Parent Feature**: T8 – Commit and push  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 001-create-branch  

**Description**  
Stage and commit all moved files, new `BUILD.bazel`, and updated root `BUILD.bazel`.  

**Implementation Details**  
- `git add .`  
- `git commit -m "Re‑organise source and test directories into library/ package"`  

**Acceptance Criteria**  
- Commit contains all changes.  

**Testing Approach**  
- `git log -1` shows the commit.  

**Quick Status**  
- Prepares changes for review.  
````

````markdown .planning/tasks/08-commit/003-push-branch.md
**Task Title**: Push branch to remote  
**Parent Feature**: T8 – Commit and push  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 002-commit-changes  

**Description**  
Push the new branch to the remote repository.  

**Implementation Details**  
- `git push origin feature/library-structure`  

**Acceptance Criteria**  
- Branch appears on GitHub.  

**Testing Approach**  
- `git branch -r` shows the remote branch.  

**Quick Status**  
- Makes changes visible to reviewers.
