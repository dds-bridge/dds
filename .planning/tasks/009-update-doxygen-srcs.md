**Task Title**: Update `doxygen_docs` genrule to new paths  
**Parent Feature**: T6 – Update `doxygen_docs` genrule  
**Complexity**: Low  
**Estimated Time**: 0.5 h  
**Dependencies**: 003-copy-include-h  

**Description**  
Modify the `srcs` glob in the `doxygen_docs` genrule to point to `library/src/` and `library/tests/`.  

**Implementation Details**  
```python
srcs = glob([
    "library/src/*.cpp",
    "library/src/*.h",
    "library/tests/*.h",
]) + ["Doxyfile"]
````

**Acceptance Criteria**  
- `bazel build //:doxygen_docs` produces a non‑empty `doxygen_docs.zip`.  

**Testing Approach**  
- Run `bazel build //:doxygen_docs` locally.  

**Quick Status**  
- Keeps documentation generation functional. 
