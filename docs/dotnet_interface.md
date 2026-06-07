# DDS_Core – .NET API for the Double Dummy Solver

`DDS_Core` is a modern, type‑safe .NET wrapper around Bo Haglund’s **Double Dummy Solver (DDS)**  .  
Please note that this library does not support `.Net Framework` and requires `.Net 8.0` or later.
The library exposes the full DDS functionality through idiomatic `.Net` structures and methods, supporting both:

- The **legacy C API** (`SolveBoard`, `CalcDDtable`, `Par`, `AnalysePlay`, …)
- The **modern C++ API** (via `SolverContext` and `SolverConfig`)

The goal is to provide a stable, fast, and fully documented .NET interface to the DDS engine.

---

## Table of Contents
1. [Introduction](#introduction)
2. [Legacy vs. Modern DDS API](#legacy-vs-modern-dds-api)
3. [Basic Usage](#basic-usage)
4. [API Overview](#api-overview)
    a. [Modern API (Recommended)](#modern-api-recommended)  
       - [Construction & Lifetime](#construction-lifetime)  
       - [Single Board Solving Modern](#single-board-solving-modern)         
       - [Double Dummy Table Calculation Modern](#double-dummy-table-calculation-modern)  
       - [Par Score Calculation Modern](#par-score-calculation-modern)        
       - [Logging](#logging)
    b. [Legacy API Overview](#legacy-api)   
       - [Configuration & Resources](#configuration-resources)  
       - [Single Board Solving](#single-board-solving)  
       - [Multiple Board Solving](#multiple-board-solving)  
       - [Double Dummy Table Calculation](#double-dummy-table-calculation)  
       - [Par Score Calculation](#par-score-calculation)  
       - [Par Text Conversion](#par-text-conversion)  
       - [Play Analysis](#play-analysis)  
       - [Utility Functions](#utility-functions)  
    c. [Conversion and error handling](#conversion-and-error-handling)
        - [Error Handling](#error-handling)  
        - [Internal Error Handling](#internal-error-handling)  
    d. [Threading & Performance](#threading-performance)  
    e. [Examples](#examples)

---

# Introduction

`DDS_Core` provides a managed .NET interface to the DDS engine.  
All data structures (`Deal`, `FutureTricks`, `DdTableResults`, etc.) are blittable and optimized for interoperation with the DDS library.

The wrapper is designed for:

- high performance  
- minimal marshalling overhead  
- strong type safety  
- full compatibility with both DDS C and C++ APIs  
- the project follows the .NET Framework Design Guidelines, including naming conventions, and leverages method overloading to provide a clear, expressive, and flexible API surface. 
- one-dimensional InlineArrays and two-dimensional array-like types are used interop arrays to avoid unnecessary heap allocations and improve performance while keeping `.Net` bounds checking
- the modern `SolverContext` class encapsulates solver state and resources, allowing for efficient reuse across multiple calls and handles the resource housekeeping and guard against memory leaks.

---

# Legacy vs. Modern DDS API

DDS exposes two API layers:

| API | Description | Status |
|-----|-------------|--------|
| **Legacy C API** | Global functions such as `SolveBoard`, `CalcDDtable`, `Par`, `AnalysePlay` | Supported but deprecated |
| **Modern C++ API** | RAII‑based `SolverContext` and `SolverConfig` | Recommended |

In `DDS_Core`, all legacy functions are marked with:

```csharp
[Obsolete("Use SolverContext instead.")]
```

# Basic Usage
Using some common declarations:
```csharp
using DDS_Core;
...
var dds = new DDS()
var hands
{
    // fill in card holdings...
};

```

Legacy API usage example:
```csharp
var rc = dds.SolveBoard(dl: deal,
                        target: 0,
                        solutions: 1,
                        mode: 0,
                        out var fut,
                        threadIndex: 0   // use 0 for single-threaded, or thread ID for multi-threaded contexts);
```


The same sample but using the modern API:
```csharp

var ctx = new SolverContext(); // create one context per thread, and reuse for multiple calls
    rc  = ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);
```

# API Overview

This document provides a structured overview of the public API exposed by the `DDS_Core` .NET wrapper.  
The API is grouped into functional areas that mirror the underlying DDS engine capabilities.
The API is divided into two layers:

1. **Modern API** — based on `SolverContext` (recommended)
2. **Legacy API** — thin wrappers around the original C API (maintained for compatibility)

---

## Modern API (Recommended)

The modern API is built around the RAII‑style `SolverContext` object, mirroring the C++ DDS API.  
Each context instance owns its own transposition table, configuration, and solver state.

---

### Construction & Lifetime

```csharp
var ctx = new SolverContext();
var ctx = new SolverContext(new SolverConfig { ... });
```

---

### Configuration & Resources 

- **SetMaxThreads(int userThreads)**  
  Sets the maximum number of threads used by the legacy solver backend.

- **SetThreading(int code)**   
  Selects the threading backend. Returns `1` on success.

- **SetResources(int maxMemoryMB, int maxThreads)**  
  Configures memory and thread limits for the legacy solver.

- **FreeMemory()**  
  Frees memory allocated by the legacy solver.

- **ResetBestMovesLite()**  
   Resets the internal cache of best moves.

---

### Single Board Solving Modern 

Functions for analyzing a single bridge deal using double‑dummy analysis.

- **int SolveBoard(Deal dl, int target, int solutions, int mode, out FutureTricks fut, int threadIndex = 0)**  
  Solves a binary `Deal` and returns trick results.

- **int SolveBoard(DealPBN pbn, int target, int solutions, int mode, out FutureTricks fut, int threadIndex = 0)**  
  PBN‑formatted variant (legacy).

---

### Double Dummy Table Calculation Modern

Computes DDS table for one deal, including optional par calculations.


- **int CalcDdTable(DdTableDeal deal, out DdTableResults table)**  
  Computes the double‑dummy table for a binary deal.

- **int CalcDdTable(DdTableDealPBN deal, out DdTableResults table)**  
  PBN variant.

---

### Par Score Calculation Modern

Computes par contracts and scores based on DDS table results.

- **int CalcPar(DdTableDeal table_deal
          , int vulnerable
          , out DdTableResults table_results
          , out ParResults par_results)**  

---

### Logging
- **void LogAppend(string message)**  
  Appends a log message to the per thread log file. 

- **void LogClear()**  
  Clears the per thread log file.

---

---

## Legacy API 

The legacy API provides direct access to the original C functions. Many if these are marked as `[Obsolete]` 
and should be used with caution, as they may not manage resources as efficiently as the modern API.

### Configuration & Resources  

- **SetMaxThreads(int userThreads)**  
  Deprecated. This sets the maximum number of threads used by the legacy solver backend. 
  The modern API manages threading implicitly via `SolverContext` and does not require manual configuration.

- **SetThreading(int code)**  
  Deprecated. configures the threading backend for the legacy API. 
  The modern API uses a more efficient and flexible threading model that does not require manual selection.

- **SetResources(int maxMemoryMB, int maxThreads)**  
  Deprecated. Sets memory and thread resources for the legacy API. The modern API uses `SolverConfig`.

- **FreeMemory()**  
  Deprecated. Frees memory allocated by the legacy API. The modern API uses RAII via `SolverContext`.

### Single Board Solving  
- **int SolveBoard(Deal dl, ...)**  
  Solves a single deal via double dummy analysis . Deprecated – use SolverContext.

- **int SolveBoard(DealPBN pbn, ...)**  
  The same only with PBN-format. Deprecated.

### Multiple Board Solving  
- **int SolveAllBoards(Board boards, out SolvedBoards solved)**  
  Solves multiple boards. Deprecated, but not yet implemented in the modern api.

- **int SolveAllBoards(BoardsPBN boards, out SolvedBoards solved)**  
  The same only with PBN-format. Deprecated, but not yet implemented in the modern api.

### Double Dummy Table Calculation  
- ** int CalcDdTable(DdTableDeal deal, out DdTableResults table)**  
  Calculates the double‑dummy table for a single deal. Deprecated – use SolverContext.
  
- **int CalcDdTable(DdTableDealPBN deal, out DdTableResults table)**  
  The same only with PBN-format. Deprecated – use SolverContext.

- **int CalcAllTables(...)**  
  As above but for multiple deals. Deprecated, but not yet implemented in the modern api.

### Par Score Calculation  
- **int Par(DdTableResults table, out ParResults pres, int vulnerable)**  
  Calculate par scores based on the double‑dummy table results. Deprecated – use SolverContext.CalcPar.

- **int CalcPar(DdTableDeal deal, ...)**  
  Combines DD-table calculation and par-score in one function. Deprecated – use SolverContext

- **int CalcPar(DdTableDealPBN deal, ...)**  
  The same only with PBN-format. Deprecated – use SolverContext.

- **int ParSide()**  
  Calculates par score for a specific side. Deprecated, but not yet implemented in the modern api.
  Exists in both a variant with binary and without PBN-format.

- **int ParDealer()**  
  Calculates par score for the dealer. Deprecated, but not yet implemented in the modern api.

- **int DealerParBothSides()**  
  Calculates par score for both sides. Deprecated, but not yet implemented in the modern api.

- **int ParAll()**  
  Calculates par score for all sides. Deprecated, but not yet implemented in the modern api.

### Play Analysis  
- **int AnalysePlay(Deal dl, PlayTraceBin play, ...)**  
  analyses a play sequence for a single deal. Deprecated, but not yet implemented in the modern api.

- **int AnalysePlay(DealPBN dl, PlayTracePBN play, ...)**  
  PBN-version. Deprecated, but not yet implemented in the modern api.

- **int AnalyseAllPlays(...)**  
  analyses multiple play sequences across multiple deals. Deprecated, but not yet implemented in the modern api.

---

## Conversion and error handling.
### Par Text Conversion    
- **int ConvertToTextFormat(ParResultsMaster pres, out string resp)**  
  Converts par results to a human‑readable text format. 

- **int ConvertToTextFormat(ParResultsMasters pres, out ParTextResults resp)**  
  Converts multi‑side par results to structured text output.

---


### Error Handling 

Most public methods returns an error code that should be checked:

```csharp
var rc = ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);

if (rc !=  (int)SolveBoardResult.NoFault)
    throw new InvalidOperationException($"DDS_Core failed with code: {rc}");
```
### Internal Error Handling

In DEBUG mode, the library will throw exceptions for any DDS error code returned by the underlying functions. 
This allows for easier debugging and error tracking during development.

---

## Threading & Performance 
The main advantages of the modern API is that it allows for better control over threading and resource management.
Each thread must create their own SolverContext and configure it. Then the same context can be used for multiple calls 
to SolveBoard, CalcDdTable, etc., without the overhead of repeated initialization. If the context is created with a using
statement, it will automatically free resources when disposed. This will guard against memory leaks and ensure that all resources 
are properly released.

## Examples 

```csharp
using DDS_Core;
...

public void sample()
{
    var dds = new DDS();

    var hands = new FourHands(new uint[4][]  // C# 12 syntax!
        {
          [ (uint)(rT | r8 | r5)
          , (uint)(rA | rT | r7 | r2)
          , (uint)(rK | rQ | r8)
          , (uint)(rA | r3 | r2)]
          , 
          [ (uint)(rJ | r2)
          , (uint)(rK | r9)
          , (uint)(rA | rJ | r7 | r4 | r3 | r2)
          , (uint)(rJ | r8 | r7)]
          ,
          [ (uint)(rK | rQ | r3)
          , (uint)(rJ | r8 | r6 | r5 | r4 | r3)
          , (uint)(r5)
          , (uint)(rK | r9 | r6)]
          , [ (uint)(rA | r9 | r7 | r6 | r4)
          , (uint)(rQ)
          , (uint)(rT | r9 | r6)
          , (uint)(rQ | rT | r5 | r4)]
        });

    var cfg = new SolverConfig(){
                   TTKind          = TTKind.Small
                 , DefaultMemoryMB = 256
                 , MaximumMemoryMB = 1024
                 }; 

    using var ctx = new SolverContext(cfg);

    ...

    var deal = new Deal { Trump            = (int)Suit.Hearts
                    , First            = 0                       // North was to lead - ie. West is declarer 
                    , CurrentTrickSuit = new int[3] {2, 0, 0 }   // Diamonds
                    , CurrentTrickRank = new int[3] {8, 0, 0 }   // 8 of Diamonds already lead
                    , RemainingCards   = hands
                    };
   
    var rc = ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);

    if (rc !=  (int)SolveBoardResult.NoFault)
    {
        ErrorMessage( rc , out string error);
        throw new InvalidOperationException(error);               
    }

    if (!(int)ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut))
        throw new InvalidOperationException("DDS_Core failed to solve the board.");


    ...

    var ddTableDeal = new() {Cards = hands };

    rc = ctx.CalcDdTable( ddTableDeal , out DdTableResults table_results);

    if (rc !=  (int)SolveBoardResult.NoFault)
    {
        ErrorMessage( rc , out string error);
        throw new InvalidOperationException(error);               
    }

   ... 
}

```