# SolverContext API Reference

`SolverContext` is the modern, RAII‑style API for interacting with the DDS engine.  
Each instance owns its own solver state, transposition table, configuration, and logging buffer.

This API is **recommended for all new development** and replaces the legacy global C API.

---

## Table of Contents

- [Overview](#overview)
- [Construction & Lifetime](#construction--lifetime)
- [Transposition Table Management](#transposition-table-management)
- [Solving Methods](#solving-methods)
- [Par Calculation](#par-calculation)
- [Logging](#logging)
- [Error Handling](#error-handling)
- [Summary Table](#summary-table)

---

# Overview

`SolverContext` encapsulates a complete DDS solver instance.  
It provides:

- isolated solver state  
- per‑context transposition table  
- configurable memory limits  
- thread‑safe usage (one context per thread)  
- modern API parity with the C++ DDS interface  

A context must be disposed when no longer needed.

---

# Construction & Lifetime

### **Default constructor**

```csharp
var ctx = new SolverContext();

or

var cfg = new SolverConfig()
             {
               TTKind          = TTKind.Large
             , DefaultMemoryMB = 256
             , MaximumMemoryMB = 1024
             };

using var ctx = new SolverContext(cfg);

or 

using var ctx = new SolverContext(new new SolverConfig(TTKind.Large,256,1024));
```` 

Please note that the `using` statement ensures that the context is properly disposed, freeing any allocated resources. 
As an alternative one might also call `ctx.Dispose()` explicitly when the context is no longer needed.


### Transposition Table Management
The transposition table (TT) is a key performance component of DDS.
SolverContext exposes full control over TT configuration and lifecycle.

- **void ConfigureTT(TTKind kind, int defaultMb, int maxMb)**  
  Configures the transposition table settings.

- **void ResizeTT(int defaultMb, int maxMb)**  
  Resizes the transposition table according to new memory limits.

- **void ClearTT()**  
  Clears all entries from the transposition table.
  Note: This is not necessary in normal usage.

- **void ResetForSolve()**
  Resets the solver state and clears the transposition table in preparation for a new solve.
  Note: This is not necessary in normal usage.

- **void ResetBestMovesLite()**  
  Clears lightweight best‑move caches used by the solver.

### Solving Methods
- **int SolveBoard(in Deal dl,
               int target,
               int solutions,
               int mode,
               out FutureTricks fut)**  
  
  Solves a single board with the specified parameters.
  
- **int CalcDdTable(in DdTableDeal deal,
                out DdTableResults table)**
  Solves a single board returning a full double dummy table.
  
- **int CalcDdTable(in DdTableDealPBN dealPbn,
                out DdTableResults table)**  
  PBN variant.
  
  
### Par Calculation
- **int CalcPar(in DdTableDeal deal,
            int vulnerable,
            out DdTableResults table,
            out ParResults par)**  

  Computes the double‑dummy table, the par score and contracts in a single optimized call.

### Logging
Each SolverContext maintains its own log buffer.

- **void LogAppend(string message)**  
  Appends a message to the internal log.
- **void LogClear()**  
  Clears the internal log.

### Error Handling
- - ****  
- - ****  
- - ****  
- - ****  

### Summary Table
# SolverContext Summary

| Category | Methods |
|:---------|:---------|
| **Construction** | `SolverContext()`, `SolverContext(SolverConfig)` |
| **Transposition Table Management** | `ConfigureTT`, `ResizeTT`, `ClearTT`, `ResetForSolve`, `ResetBestMovesLite` |
| **Solving** | `SolveBoard`, `CalcDdTable` (binary), `CalcDdTable` (PBN) |
| **Par Calculation** | `CalcPar` |
| **Logging** | `LogAppend`, `LogClear` |

