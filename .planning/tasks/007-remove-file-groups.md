**Task Title**: Remove obsolete `filegroup` targets from root `BUILD.bazel`  
**Parent Feature**: T5 – Update top‑level `BUILD.bazel`  
**Complexity**: Medium  
**Estimated Time**: 0.5 h  
**Dependencies**: 001-create-build-file  

**Description**  
Delete the `dds_sources`, `dds_headers`, and any other `filegroup` definitions that are no longer needed.  

**Implementation Details**  
- Edit `BUILD.bazel` and remove the `filegroup` blocks.  

**Acceptance Criteria**  
- `BUILD.bazel` contains only the new `library/src/BUILD.bazel` reference and any other necessary top‑level targets.  

**Testing Approach**  
- Run `bazel build //...` to ensure no errors.  

**Quick Status**  
- Keeps the root BUILD clean and prevents accidental use of stale filegroups.  
