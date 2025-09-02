# Build System Documentation

## Heuristic Sorting Library Build Configuration

The DDS project now includes a modular heuristic sorting library that can be enabled through build-time configuration.

### Build Configurations

#### Default Configuration (Original Heuristic)
```bash
bazel build //library/src:dds
```
Uses the original heuristic implementation embedded in moves.cpp.

#### New Heuristic Configuration
```bash
bazel build //library/src:dds --config=new_heuristic
```
Uses the new modular heuristic sorting library.

#### Debug Build with New Heuristic
```bash
bazel build //library/src:dds --config=debug_new_heuristic
```
Debug build using the new heuristic library.

#### Optimized Build with New Heuristic
```bash
bazel build //library/src:dds --config=opt_new_heuristic
```
Optimized build using the new heuristic library.

### Manual Configuration
You can also manually specify the configuration:
```bash
bazel build //library/src:dds --define=use_new_heuristic=true
```

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
The `DDS_USE_NEW_HEURISTIC` flag is conditionally added based on the build configuration:

```starlark
DDS_LOCAL_DEFINES = select({
    "//:build_macos": ["DDS_THREADS_GCD"],
    "//:debug_build_macos": ["DDS_THREADS_GCD"],
    "//:build_linux": [],
    "//:debug_build_linux": [],
    "//conditions:default": [],
}) + select({
    "//:new_heuristic": ["DDS_USE_NEW_HEURISTIC"],
    "//conditions:default": [],
})
```

#### BUILD.bazel
Configuration setting for the new heuristic:

```starlark
config_setting(
    name = "new_heuristic",
    define_values = {"use_new_heuristic": "true"},
)
```

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

- `//library/src/heuristic_sorting` - Main heuristic sorting library
- `//library/src/heuristic_sorting:testable_heuristic_sorting` - Test version with internal symbols exposed

### Visibility and Access

The heuristic sorting library is marked with `visibility = ["//visibility:public"]` and can be used by any target in the project.

### Dependencies

The heuristic sorting library depends on:
- `//library/src/utility:constants` - DDS constants and definitions
- `//library/src/utility:lookup_tables` - Precomputed lookup tables

### Migration Path

1. **Phase 1**: Use default configuration for production (original heuristic)
2. **Phase 2**: Test with `--config=new_heuristic` for validation
3. **Phase 3**: Switch default to new heuristic once fully validated
4. **Phase 4**: Remove old heuristic code after confidence period
