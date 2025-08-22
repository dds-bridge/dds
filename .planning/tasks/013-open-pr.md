**Task Title**: Open PR for library re‑organisation  
**Parent Feature**: T8 – Commit and push  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 003-push-branch  

**Description**  
Create a pull request on GitHub with a clear title and description.  

**Implementation Details**  
- Use the `github` MCP server:  
  ```json
  {
    "name": "github_create_pr",
    "arguments": {
      "base": "mn_main",
      "head": "feature/library-structure",
      "title": "Re‑organise source and test directories into library/ package",
      "body": "This PR moves all C++ sources and tests into a dedicated `library/` package, updates BUILD files, and cleans up obsolete targets."
    }
  }
  ```  

**Acceptance Criteria**  
- PR is created and visible on GitHub.  

**Testing Approach**  
- Verify PR status and CI results.  

**Quick Status**  
- Enables review and merge.
