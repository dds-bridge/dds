# DDS API Migration Guide

This guide helps you migrate from the legacy C API to the modern C++ API.

## Table of Contents
- [Overview](#overview)
- [Why Migrate?](#why-migrate)
- [API Comparison](#api-comparison)
- [Migration Examples](#migration-examples)
- [Common Patterns](#common-patterns)
- [Performance Notes](#performance-notes)
- [FAQ](#faq)

## Overview

DDS provides two API layers:

### Modern C++ API (Recommended)
- **Header:** `#include <dds/dds.hpp>`
- **Core Type:** `SolverContext` - instance-scoped solver state
- **Configuration:** `SolverConfig` - per-context settings
- **Memory Management:** RAII - automatic cleanup
- **Threading:** Implicit - one context per thread

### Legacy C API (Backward Compatible)
- **Header:** `#include <api/dll.h>`
- **State:** Global and thread-local
- **Configuration:** `SetThreading()`, `SetResources()`, etc.
- **Memory Management:** Manual - `FreeMemory()` required
- **Threading:** Explicit backend selection

## Why Migrate?

### Benefits of Modern C++ API

| Feature | Modern API | Legacy API |
|---------|-----------|------------|
| **Memory Safety** | RAII automatic cleanup | Manual `FreeMemory()` calls |
| **Thread Safety** | One context per thread | Global state with locking |
| **Configuration** | Per-instance via `SolverConfig` | Global via `SetResources()` |
| **TT Lifecycle** | Explicit control (`reset_for_solve()`) | Implicit global pool |
| **Performance** | Equal or better | Baseline |
| **Flexibility** | Multiple contexts, different configs | Single global config |

### Key Advantages

1. **No resource leaks** - Context automatically cleans up when destroyed
2. **Better multithreading** - Each thread owns its context (no contention)
3. **Explicit lifecycle** - You control when to reset or clear transposition tables
4. **Type safety** - C++ types and compile-time checks
5. **Modern patterns** - Smart pointers, RAII, standard library integration

## API Comparison

### Deprecated Functions and Replacements

| Legacy Function | Modern Replacement | Notes |
|----------------|-------------------|-------|
| `SetThreading(code)` | Create one `SolverContext` per thread | Threading is implicit |
| `SetMaxThreads(n)` | One `SolverContext` per worker thread | Control thread count in your app |
| `SetResources(mem, threads)` | `SolverConfig::tt_mem_maximum_mb_` + per-thread contexts | Split memory and thread control |
| `FreeMemory()` | Context destruction | Automatic via RAII |

### Core Solving Functions

The solving functions remain compatible but have new overloads:

```cpp
// Legacy C API
int SolveBoard(struct Deal dl, int target, int solutions, int mode,
               struct FutureTricks* futp, int thrId);

// Modern C++ API - new overload
int SolveBoard(SolverContext& ctx, const Deal& dl, int target,
               int solutions, int mode, struct FutureTricks* futp);
```

## Migration Examples

### Example 1: Single Board Solve

#### Before (Legacy C API)
```c
#include <api/dll.h>

int main() {
    // Global configuration
    SetMaxThreads(4);
    SetResources(2000, 4);
    
    // Solve a board
    Deal dl;
    // ... initialize dl ...
    
    FutureTricks fut;
    int res = SolveBoard(dl, -1, 3, 0, &fut, 0);
    
    if (res == RETURN_NO_FAULT) {
        // Use results
        printf("Tricks: %d\n", fut.score[0]);
    }
    
    // Manual cleanup
    FreeMemory();
    return 0;
}
```

#### After (Modern C++ API)
```cpp
#include <dds/dds.hpp>
#include <memory>

int main() {
    // Per-instance configuration
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_default_mb_ = 2000;
    cfg.tt_mem_maximum_mb_ = 2000;
    
    // Create context
    SolverContext ctx(cfg);
    
    // Solve a board
    Deal dl;
    // ... initialize dl ...
    
    FutureTricks fut;
    int res = SolveBoard(ctx, dl, -1, 3, 0, &fut);
    
    if (res == RETURN_NO_FAULT) {
        // Use results
        std::cout << "Tricks: " << fut.score[0] << "\n";
    }
    
    // Automatic cleanup when ctx goes out of scope
    return 0;
}
```

### Example 2: Multiple Boards with TT Reuse

#### Before (Legacy C API)
```c
#include <api/dll.h>

void solve_many_boards(Deal* boards, int count) {
    SetResources(2000, 4);
    
    for (int i = 0; i < count; i++) {
        FutureTricks fut;
        SolveBoard(boards[i], -1, 3, 0, &fut, 0);
        // Process results...
    }
    
    FreeMemory();
}
```

#### After (Modern C++ API)
```cpp
#include <dds/dds.hpp>

void solve_many_boards(const std::vector<Deal>& boards) {
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_default_mb_ = 2000;
    cfg.tt_mem_maximum_mb_ = 2000;
    
    SolverContext ctx(cfg);
    
    for (const auto& board : boards) {
        FutureTricks fut;
        SolveBoard(ctx, board, -1, 3, 0, &fut);
        // Process results...
        // TT accumulates knowledge across boards
    }
    
    // Optional: clear TT between unrelated problems
    // ctx.reset_for_solve();
    
    // Automatic cleanup
}
```

### Example 3: Multithreaded Processing

#### Before (Legacy C API)
```c
#include <api/dll.h>
#include <pthread.h>

void* worker(void* arg) {
    int thrId = *(int*)arg;
    Deal dl;
    // ... initialize dl ...
    
    FutureTricks fut;
    SolveBoard(dl, -1, 3, 0, &fut, thrId);  // Thread ID required
    return NULL;
}

int main() {
    SetMaxThreads(4);
    SetResources(2000, 4);
    
    pthread_t threads[4];
    int ids[4] = {0, 1, 2, 3};
    
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }
    
    for (int i = 0; i < 4; i++) {
        pthread_join(threads[i], NULL);
    }
    
    FreeMemory();
    return 0;
}
```

#### After (Modern C++ API)
```cpp
#include <dds/dds.hpp>
#include <thread>
#include <vector>

void worker(Deal dl) {
    // Each thread creates its own context
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Large;
    cfg.tt_mem_default_mb_ = 500;   // 500MB per thread (default TT size)
    cfg.tt_mem_maximum_mb_ = 500;   // Cap TT size at 500MB per thread
    
    SolverContext ctx(cfg);
    
    FutureTricks fut;
    SolveBoard(ctx, dl, -1, 3, 0, &fut);
    // Process results...
    
    // Automatic cleanup when thread exits
}

int main() {
    std::vector<Deal> deals;
    // ... initialize deals ...
    
    std::vector<std::thread> threads;
    for (const auto& dl : deals) {
        threads.emplace_back(worker, dl);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

## Common Patterns

### Pattern 1: Reusable Context for Batch Processing

```cpp
class BatchSolver
{
  public:
    BatchSolver() 
    : ctx_(default_config())
    {
    }
    
    auto solve(const Deal& dl) -> FutureTricks
    {
        FutureTricks fut;
        SolveBoard(ctx_, dl, -1, 3, 0, &fut);
        return fut;
    }
    
    auto reset() -> void
    {
        ctx_.reset_for_solve();  // Clear TT for new problem set
    }
    
  private:
    SolverContext ctx_;
    
    static auto default_config() -> SolverConfig
    {
        SolverConfig cfg;
        cfg.tt_kind_ = TTKind::Large;
        cfg.tt_mem_default_mb_ = 2000;
        cfg.tt_mem_maximum_mb_ = 2000;
        return cfg;
    }
};
```

### Pattern 2: Conditional TT Management

```cpp
void solve_tournament(const std::vector<Deal>& boards) {
    SolverContext ctx;
    
    for (size_t i = 0; i < boards.size(); i++) {
        FutureTricks fut;
        SolveBoard(ctx, boards[i], -1, 3, 0, &fut);
        
        // Clear TT between rounds but not within rounds
        if ((i + 1) % 32 == 0) {
            ctx.reset_for_solve();  // New round, unrelated boards
        }
    }
}
```

### Pattern 3: Memory-Constrained Environments

```cpp
SolverConfig make_small_config() {
    SolverConfig cfg;
    cfg.tt_kind_ = TTKind::Small;      // Use small TT implementation
    cfg.tt_mem_default_mb_ = 100;      // Default TT size 100MB
    cfg.tt_mem_maximum_mb_ = 100;      // Hard cap 100MB
    return cfg;
}

void solve_on_embedded(const Deal& dl) {
    SolverContext ctx(make_small_config());
    FutureTricks fut;
    SolveBoard(ctx, dl, -1, 3, 0, &fut);
}
```

## Performance Notes

### Memory Usage
- **Legacy API**: Global pool shared across threads
- **Modern API**: Per-context allocation (multiply by thread count)

**Recommendation**: In multithreaded scenarios, reduce `tt_mem_maximum_mb_` per context to avoid over-allocation.

### Transposition Table Efficiency
The modern API gives you explicit control over TT lifecycle:

```cpp
SolverContext ctx;

// Scenario 1: Related boards (same deal, different contracts)
// Keep TT - it accumulates useful positions
for (auto contract : contracts) {
    solve_with_contract(ctx, deal, contract);
    // NO reset - TT knowledge carries over
}

// Scenario 2: Unrelated boards
// Reset TT - old positions won't help
for (auto deal : unrelated_deals) {
    ctx.reset_for_solve();  // Clear between deals
    solve_board(ctx, deal);
}
```

### Threading Overhead
- **Legacy API**: Locking on shared global state
- **Modern API**: No locking (each thread owns its context)

Result: Better parallelism with modern API.

## FAQ

### Q: Will the legacy C API be removed?
**A:** No. It remains supported indefinitely for binary compatibility. However, new features may only appear in the modern API.

### Q: Can I mix both APIs in the same program?
**A:** Yes, but not recommended. Choose one approach for consistency. The modern API is preferred for new code.

### Q: Do I need to change my build system?
**A:** No. Just update your `#include` directive and use the new API. Both APIs link against the same library.

### Q: What about C-only projects?
**A:** The legacy C API remains fully supported. Consider wrapping the modern C++ API if you can link against C++.

### Q: Are there performance differences?
**A:** The modern API is equal or slightly better due to reduced locking and better cache locality. Solve quality is identical.

### Q: How do I migrate incrementally?
**A:** 
1. Keep using legacy API with deprecation warnings
2. Add `#include <dds/dds.hpp>` alongside `<api/dll.h>`
3. Migrate one function/module at a time
4. Remove legacy includes once complete

### Q: What if I encounter issues?
**A:** Report issues on GitHub. The legacy API will remain functional while you migrate.

## Additional Resources

- [Legacy C API Reference](legacy_c_api.md) - Full documentation of deprecated functions
- [SolverContext Documentation](../library/src/README_SolverContext.md) - Modern API details
- [Build System Guide](BUILD_SYSTEM.md) - Bazel configuration
- [Examples](../examples/) - Sample programs using both APIs

## Summary

| If you need... | Use... |
|----------------|--------|
| Simple single-threaded solving | Modern API - simplest, safest |
| Batch processing with TT reuse | Modern API - explicit control |
| Multi-threaded solving | Modern API - better parallelism |
| C-only compatibility | Legacy API - still supported |
| Existing code maintenance | Legacy API - no rush to migrate |

**Recommendation for new projects:** Use the modern C++ API with `SolverContext`. It's safer, clearer, and more flexible.
