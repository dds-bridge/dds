**Task Title**: Copy `include/*.h` files to `library/src/`  
**Parent Feature**: T2 – Move source files  
**Complexity**: Medium  
**Estimated Time**: 0.5 h  
**Dependencies**: 002-copy-src-h  

**Description**  
Move all `include/*.h` files into `library/src/`.  

**Implementation Details**  
- `git mv include/*.h library/src/`  

**Acceptance Criteria**  
- All original `include/*.h` files are now under `library/src/`.  

**Testing Approach**  
- Run `bazel build //library/src:dds` to confirm compilation.  

**Quick Status**  
- No impact on existing includes.
