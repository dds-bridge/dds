
````

---

## 4️⃣  Add `library/src/BUILD.bazel`

````markdown .planning/tasks/04-build-file/001-create-build-file.md
**Task Title**: Create `library/src/BUILD.bazel`  
**Parent Feature**: T4 – Add `library/src/BUILD.bazel`  
**Complexity**: Medium  
**Estimated Time**: 0.5 h  
**Dependencies**: 003-copy-include-h  

**Description**  
Create a new `BUILD.bazel` in `library/src/` that defines the `dds` and `testable_dds` targets.  

**Implementation Details**  
```python library/src/BUILD.bazel
load("//:CPPVARIABLES.bzl", "DDS_CPPOPTS", "DDS_LINKOPTS", "DDS_LOCAL_DEFINES")
load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
    name = "dds",
    srcs = glob(["*.cpp"]),
    hdrs = glob(["*.h"]),
    copts = DDS_CPPOPTS,
    linkopts = DDS_LINKOPTS,
    local_defines = DDS_LOCAL_DEFINES,
    visibility = ["//visibility:public"],
    include_prefix = "dds",
    strip_include_prefix = "src",
    deps = [],
)

cc_library(
    name = "testable_dds",
    srcs = glob(["*.cpp", "*.h"]),
    hdrs = glob(["*.h"]),
    copts = DDS_CPPOPTS,
    linkopts = DDS_LINKOPTS,
    local_defines = DDS_LOCAL_DEFINES,
    visibility = [
        "//tests:__pkg__",
        "//tests/regression/heuristic_sorting:__pkg__",
    ],
    includes = ["include", "src"],
    strip_include_prefix = "src",
    include_prefix = "dds",
    deps = [],
)
