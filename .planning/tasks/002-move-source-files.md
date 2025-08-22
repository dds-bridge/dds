## 2️⃣  Move Source Files

````markdown .planning/tasks/02-move-sources/001-copy-src-cpp.md
**Task Title**: Copy `.cpp` files to `library/src/`  
**Parent Feature**: T2 – Move source files  
**Complexity**: Medium  
**Estimated Time**: 1 h  
**Dependencies**: 001-create-library-dir  

**Description**  
Move all `src/*.cpp` files into `library/src/`.  

**Implementation Details**  
- `git mv src/*.cpp library/src/`  
- Update any relative includes if needed (none expected).  

**Acceptance Criteria**  
- All original `.cpp` files are now under `library/src/`.  

**Testing Approach**  
- Run `bazel build //library/src:dds` to confirm compilation.  

**Quick Status**  
- Maintains buildability; no tests affected. 


