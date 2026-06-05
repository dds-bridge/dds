# DDS_Core – .NET API for the Double Dummy Solver

`DDS_Core` is a modern, type‑safe .NET wrapper around Bo Haglund’s **Double Dummy Solver (DDS)**.  
The library exposes the full DDS functionality through idiomatic C# structures and methods, supporting both:

- The **legacy C API** (`SolveBoard`, `CalcDDtable`, `Par`, `AnalysePlay`, …)
- The **modern C++ API** (via `SolverContext` and `SolverConfig`)

The goal is to provide a stable, fast, and fully documented .NET interface to the DDS engine.

---

## Table of Contents
1. [Introduction](#introduction)  
2. [Legacy vs. Modern DDS API](#legacy-vs-modern-dds-api)  
3. [Basic Usage](#basic-usage)  
4. [API Overview](#api-overview)  
   - [Configuration & Resources](#configuration--resources)  
   - [Single Board Solving](#single-board-solving)  
   - [Multiple Board Solving](#multiple-board-solving)  
   - [Double Dummy Table Calculation](#double-dummy-table-calculation)  
   - [Par Score Calculation](#par-score-calculation)  
   - [Par Text Conversion](#par-text-conversion)  
   - [Play Analysis](#play-analysis)  
   - [Utility Functions](#utility-functions)  
5. [Error Handling](#error-handling)  
6. [Threading & Performance](#threading--performance)  
7. [Examples](#examples)

---

# Introduction

`DDS_Core` provides a managed .NET interface to the DDS engine.  
All data structures (`Deal`, `FutureTricks`, `DdTableResults`, etc.) are blittable and optimized for interoparation with the DDS library.

The wrapper is designed for:

- high performance  
- minimal marshalling overhead  
- strong type safety  
- full compatibility with both DDS C and C++ APIs  

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

Legacy API usage example:
```csharp
var dds = new DDS();

var deal = new Deal
{
    // fill in card holdings...
};

dds.SolveBoard(
    dl: deal,
    target: 0,
    solutions: 1,
    mode: 0,
    out var fut
);

Console.WriteLine($"Best trick count: {fut.score[0]}");
```


The same sample but using the modern API:
```csharp
using DDS_Core;
...

var dds = new DDS();

var deal = new Deal
{
    // fill in card holdings...
};

var cfg = new SolverConfig()
             {
               TTKind          = TTKind.Large
             , DefaultMemoryMB = 256
             , MaximumMemoryMB = 1024
             };

using (var ctx = new SolverContext(cfg))
{
    var rc = ctx.SolveBoard(deal, -1, 1, 0, out FutureTricks fut);
}
```

# API Overview

This document provides a structured overview of the public API exposed by the `DDS_Core` .NET wrapper.  
The API is grouped into functional areas that mirror the underlying DDS engine capabilities.
The API is divided into two layers:

1. **Modern API** — based on `SolverContext` (recommended)
2. **Legacy API** — thin wrappers around the original C API (maintained for compatibility)

---

# Modern API (Recommended)

The modern API is built around the RAII‑style `SolverContext` object, mirroring the C++ DDS API.  
Each context instance owns its own transposition table, configuration, and solver state.

---

## SolverContext

### Construction & Lifetime

```csharp
var ctx = new SolverContext();
var ctx = new SolverContext(new SolverConfig { ... });

---

## Configuration & Resources

These functions belong to the **legacy C API** and are preserved for backward compatibility.  
Modern applications should prefer `SolverContext` and `SolverConfig`.

### Methods
- **SetMaxThreads(int userThreads)**  
  Sets the maximum number of threads used by the legacy solver backend.

- **SetThreading(int code)**  
  Selects the threading backend. Returns `1` on success.

- **SetResources(int maxMemoryMB, int maxThreads)**  
  Configures memory and thread limits for the legacy solver.

- **FreeMemory()**  
  Frees memory allocated by the legacy solver.

---

## Single Board Solving

Functions for analyzing a single bridge deal using double‑dummy analysis.

### Methods
- **SolveBoard(Deal dl, int target, int solutions, int mode, out FutureTricks fut, int threadIndex = 0)**  
  Solves a binary `Deal` and returns trick results.

- **SolveBoard(DealPBN pbn, int target, int solutions, int mode, out FutureTricks fut, int threadIndex = 0)**  
  PBN‑formatted variant (legacy).

---

## Multiple Board Solving

Batch‑oriented solving of multiple deals, significantly faster than repeated single‑deal calls.

### Methods
- **SolveAllBoards(Boards bop, out SolvedBoards solved)**  
  Solves multiple PBN deals.

- **SolveAllBoards(BoardsPBN boards, out SolvedBoards solved)**  
  Binary variant (legacy).

---

## Double Dummy Table Calculation

Computes DDS tables for one or more deals, including optional par calculations.

### Methods
- **CalcDdTable(DdTableDeal deal, out DdTableResults table)**  
  Computes the double‑dummy table for a binary deal.

- **CalcDdTable(DdTableDealPBN deal, out DdTableResults table)**  
  PBN variant.

- **CalcAllTables(DdTableDeals deals, int mode, intArray5 trumpFilter, out DdTablesResult resTables, out AllParResults parResults)**  
  Computes tables and par results for multiple deals.

- **CalcAllTables(DdTableDealsPBN deals, int mode, intArray5 trumpFilter, out DdTablesResult resTables, out AllParResults parResults)**  
  PBN variant.

---

## Par Score Calculation

Computes par contracts and scores based on DDS table results.

### Methods
- **Par(DdTableResults table, out ParResults pres, int vulnerable)**  
  Computes par score for both sides.

- **CalcPar(DdTableDeal deal, int vulnerable, out DdTableResults tableResults, out ParResults parResults)**  
  Computes DDS table and par score in one call.

- **CalcPar(DdTableDealPBN deal, out DdTableResults tableResults, int vulnerable, out ParResults parResults)**  
  PBN variant.

- **ParSide(DdTableResults table, out ParResultsDealers sidesRes, int vulnerable)**  
  Computes par results for both sides separately.

- **ParDealer(DdTableResults table, out ParResultsDealer pres, int dealer, int vulnerable)**  
  Computes par score for a specific dealer.

- **DealerParBothSides(DdTableResults table, out ParResultsMaster pres, int dealer, int vulnerable)**  
  Computes par contracts for both sides for a specific dealer.

- **ParAll(DdTableResults table, out ParResultsMasters sidesRes, int vulnerable)**  
  Computes par results for all sides and dealers.

---

## Par Text Conversion

Converts par results into human‑readable text formats.

### Methods
- **ConvertToTextFormat(ParResultsMaster pres, out string resp)**  
  Converts dealer‑based par results to a text string.

- **ConvertToTextFormat(ParResultsMasters pres, out ParTextResults resp)**  
  Converts multi‑side par results to structured text output.

---

## Play Analysis

Analyzes play sequences (traces) and computes optimal lines of play.

### Methods
- **AnalysePlay(Deal dl, PlayTraceBin play, out SolvedPlay solved, int thrId)**  
  Analyzes a binary deal and play trace.

- **AnalysePlay(DealPBN dl, PlayTracePBN play, out SolvedPlay solved, int thrId)**  
  PBN variant.

- **AnalyseAllPlays(Boards bop, PlayTracesBin plp, out SolvedPlays solved, int chunkSize)**  
  Batch analysis of multiple deals and play traces.

- **AnalyseAllPlays(BoardsPBN bop, PlayTracesPBN plp, out SolvedPlays solved, int chunkSize)**  
  PBN variant.

---

## Utility Functions

General helper functions for diagnostics and error reporting.

### Methods
- **GetDDSInfo(out DdsInfo info)**  
  Retrieves version and build information from the DDS engine.

- **ErrorMessage(int code, out string line)**  
  Returns a human‑readable description of a DDS error code.

---

## Error Handling

Most public methods returns an error code that should be checked:

```csharp
 var rc = dds.SolveAllBoards( bop, out  solved);

 if (rc !=  (int)SolveBoardResult.NoFault)
     throw new InvalidOperationException($"DDS_Core failed with code: {rc}");
```