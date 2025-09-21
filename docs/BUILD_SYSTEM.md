# Build System Documentation

## Heuristic Sorting Library Build Configuration

The DDS project now includes a modular heuristic sorting library that can be enabled through build-time configuration.

### Build Configurations

#### Default Configuration (Original Heuristic)
```bash
bazel build //library/src:dds
```
Uses the original heuristic implementation embedded in moves.cpp.

#### Heuristic Sorting
The project uses the modular heuristic sorting library as the default implementation. The library target
`//library/src/heuristic_sorting` is available and is included as a dependency by targets that need
heuristic sorting.

### Library Dependencies

The heuristic sorting library is automatically included as a dependency:

```starlark
deps = [
    "//library/src/utility:constants",
    "//library/src/utility:lookup_tables",
    "//library/src/heuristic_sorting",  # <- Heuristic sorting library
],
```

### Configuration Details

#### CPPVARIABLES.bzl
The build variable mapping that previously injected `DDS_USE_NEW_HEURISTIC` has been removed. The
heuristic sorting library is included by depending on `//library/src/heuristic_sorting` where required.

#### BUILD.bazel
The project no longer relies on a build-time define to select the heuristic implementation. Targets that need heuristic sorting should depend on `//library/src/heuristic_sorting`.

### Testing Both Configurations

To verify both configurations work:

```bash
# Test original heuristic
bazel build //library/src/heuristic_sorting:heuristic_sorting

# Test new heuristic
bazel build //library/src/heuristic_sorting:heuristic_sorting --config=new_heuristic

# Both should build successfully
```

### Library Structure


### Visibility and Access

The heuristic sorting library is marked with `visibility = ["//visibility:public"]` and can be used by any target in the project.

### Dependencies

The heuristic sorting library depends on:

### Migration Path

1. **Phase 1**: Use default configuration for production (original heuristic)
2. **Phase 2**: Test with `--config=new_heuristic` for validation
3. **Phase 3**: Switch default to new heuristic once fully validated
4. **Phase 4**: Remove old heuristic code after confidence period
