**Task Title**: Move test files to `library/tests/`  
**Parent Feature**: T3 – Move test files  
**Complexity**: Medium  
**Estimated Time**: 1 h  
**Dependencies**: 001-create-library-dir  

**Description**  
Move all contents of the existing `tests/` directory into `library/tests/`.  

**Implementation Details**  
- `git mv tests/* library/tests/`  

**Acceptance Criteria**  
- `tests/` is empty or removed; all test files reside under `library/tests/`.  

**Testing Approach**  
- Run `bazel test //library/tests:all` to confirm tests compile and run.  

**Quick Status**  
- Keeps test suite functional.