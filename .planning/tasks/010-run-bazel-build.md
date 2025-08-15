**Task Title**: Run `bazel build //...` locally  
**Parent Feature**: T7 – Run CI locally  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 002-add-exports  

**Description**  
Execute a full build to catch any missing dependencies or path issues.  

**Implementation Details**  
- `bazel build //...`  

**Acceptance Criteria**  
- Build completes without errors.  

**Testing Approach**  
- Observe build output.  

**Quick Status**  
- Validates overall build integrity.  
````

````markdown .planning/tasks/07-ci/002-run-bazel-test.md
**Task Title**: Run `bazel test //...` locally  
**Parent Feature**: T7 – Run CI locally  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 001-run-bazel-build  

**Description**  
Execute all tests to ensure nothing broke during the move.  

**Implementation Details**  
- `bazel test //...`  

**Acceptance Criteria**  
- All tests pass.  

**Testing Approach**  
- Review test output.  

**Quick Status**  
- Confirms test suite health. 
