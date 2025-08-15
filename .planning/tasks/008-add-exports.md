**Task Title**: Export `library/src` targets from root `BUILD.bazel`  
**Parent Feature**: T5 – Update top‑level `BUILD.bazel`  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 001-remove-filegroups  

**Description**  
Add an `exports` rule so that other packages can depend on `//library/src:dds` and `//library/src:testable_dds`.  

**Implementation Details**  
```python
exports_files(["//library/src:dds", "//library/src:testable_dds"])
````

**Acceptance Criteria**  
- `bazel build //library/src:dds` and `bazel build //library/src:testable_dds` succeed from any package.  

**Testing Approach**  
- Run `bazel build //...` and `bazel test //...`.  

**Quick Status**  
- Enables downstream consumers to reference the new library.  
