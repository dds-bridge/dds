# Legacy C API Compatibility

DDS 3.0 provides two API layers so existing C integrations keep working while
new development can use the modern C++ interface.

## Overview

- **Modern C++ API**: `SolverContext` and `SolverConfig` with RAII and
  per-instance configuration.
- **Legacy C API**: Global state, manual cleanup, maintained for backward
  compatibility.

**Recommendation:** New projects should use the modern C++ API. Existing C
clients can remain on the legacy API without changes.

## API Comparison

| Feature | Legacy C API | Modern C++ API |
| --- | --- | --- |
| Header | `<api/dll.h>` | `<dds/dds.hpp>` |
| Context | Global state | `SolverContext` instance |
| Memory | Manual (`FreeMemory()`) | RAII automatic |
| Threading | Global config | Implicit (one context per thread) |
| TT lifecycle | Global pool | Per-context ownership |
| Performance | Baseline | Equal or better |

## Supported C API Functions

### Fully Supported
These functions remain fully supported and are expected to stay available:

- `SolveBoard`
- `SolveBoardPBN`
- `CalcDDtable`
- `CalcDDtablePBN`
- `AnalysePlayBin`
- `AnalysePlayPBN`
- `Par`
- `DealerPar`
- `GetDDSInfo`
- `SetMaxThreads` (deprecated, still functional)
- `SetResources` (deprecated, still functional)

### Deprecated but Functional
These initialization functions are deprecated but remain operational for
compatibility:

- `SetThreading`
- `SetMaxThreads`
- `SetResources`
- `FreeMemory`

### For C Compatibility Only
The legacy API exists for binary compatibility with existing C clients.
New development should use the modern C++ API instead.

## Migration Guide (Summary)

1. Include the modern header: `#include <dds/dds.hpp>`
2. Create a `SolverContext` instance per thread.
3. Replace global configuration with `SolverConfig` fields.
4. Pass the context into solving functions.
5. Remove manual `FreeMemory()` calls.

For detailed steps and examples, see [api_migration.md](api_migration.md).

## Code Examples

### Legacy C API (Global State)
```c
#include <api/dll.h>

void solve_legacy(Deal dl)
{
    SetMaxThreads(4);
    SetResources(2000, 4);

    FutureTricks fut;
    int res = SolveBoard(dl, -1, 3, 0, &fut, 0);
    (void)res;

    FreeMemory();
}
```

### Modern C++ API (Instance-Scoped)
```cpp
#include <dds/dds.hpp>
#include <memory>

void solve_modern(const Deal& dl)
{
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_maximum_mb_ = 2000;

    SolverContext ctx(std::make_shared<ThreadData>(), cfg);

    FutureTricks fut;
    int res = SolveBoard(ctx, dl, -1, 3, 0, &fut);
    (void)res;
}
```

## FAQ

**Will the C API be removed?**
No. There is no removal plan; the C API remains for backward compatibility.

**Can I mix both APIs in one program?**
Yes, but avoid using global configuration calls while using `SolverContext`.
Prefer the modern API for new work.

**Is performance different?**
Modern API performance is equal or better because it reduces global contention
and allocations.

## Deprecation Timeline

- **Current:** Deprecated init functions are documented and remain functional.
- **Future:** No removal planned; long-term support continues.
- **Recommendation:** New development should use `SolverContext` and
  `SolverConfig`.
