**Task Title**: Copy `.h` files to `library/src/`  
**Parent Feature**: T2 – Move source files  
**Complexity**: Medium  
**Estimated Time**: 1 h  
**Dependencies**: 001-copy-src-cpp  

**Description**  
Move all `src/*.h` files into `library/src/`.  

**Implementation Details**  
- `git mv src/*.h library/src/`  

**Acceptance Criteria**  
- All original `.h` files are now under `library/src/`.  

**Testing Approach**  
- Run `bazel build //library/src:dds` to confirm compilation.  

**Quick Status**  
- Keeps headers in sync with sources.  
