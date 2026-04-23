# Legacy C API Reference

This document describes the legacy C API exposed by DDS for C and ABI-compatible integrations.

Header:

- `#include <api/dll.h>`

## Scope

The C API remains supported for existing clients and tools.

- Global/thread-indexed API style
- C structs in `dll.h`
- Integer status/error return codes
- Manual lifecycle/configuration functions

For migration to the modern C++ interface, see `docs/api_migration.md`.

## Core C Data Structures

Common structs used by C entry points:

- `Deal`, `DealPBN`
- `FutureTricks`
- `Boards`, `BoardsPBN`, `SolvedBoards`
- `DdTableDeal`, `DdTableDealPBN`, `DdTableResults`
- `ParResults`, `ParResultsDealer`

These core structs are defined directly in `api/dll.h`. In this repository, `api/dds.h` includes `api/dll.h`, rather than defining these public C structs itself. Additional legacy C API structs used by other functions, such as play-analysis and utility types, are also declared in `api/dll.h`.

## Solve Functions

Single-board solve:

- `SolveBoard(Deal dl, int target, int solutions, int mode, FutureTricks* futp, int thrId)`
- `SolveBoardPBN(DealPBN dlpbn, int target, int solutions, int mode, FutureTricks* futp, int thrId)`

Batch solve:

- `SolveAllBoards(BoardsPBN const* bop, SolvedBoards* solvedp)`
- `SolveAllBoardsBin(Boards const* bop, SolvedBoards* solvedp)`
- `SolveAllChunksPBN(BoardsPBN const* bop, SolvedBoards* solvedp, int chunkSize)`
- `SolveAllChunksBin(Boards const* bop, SolvedBoards* solvedp, int chunkSize)`

## DD Table Functions

- `CalcDDtable(DdTableDeal tableDeal, DdTableResults* tablep)`
- `CalcDDtablePBN(DdTableDealPBN tableDealPBN, DdTableResults* tablep)`
- `CalcAllTables(Boards* bop, int mode, int trumpFilter[DDS_STRAINS], DdTablesRes* resp, AllParResults* presp)`
- `CalcAllTablesPBN(BoardsPBN* bop, int mode, int trumpFilter[DDS_STRAINS], DdTablesRes* resp, AllParResults* presp)`

## Play Analysis Functions

- `AnalysePlayBin(Deal dl, PlayTraceBin play, SolvedPlay* solved, int thrId)`
- `AnalysePlayPBN(DealPBN dlPBN, PlayTracePBN playPBN, SolvedPlay* solved, int thrId)`
- `AnalyseAllPlaysBin(Boards* bop, PlayTracesBin* plp, SolvedPlays* solvedp, int chunkSize)`
- `AnalyseAllPlaysPBN(BoardsPBN* bopPBN, PlayTracesPBN* plpPBN, SolvedPlays* solvedp, int chunkSize)`

## Par Functions

- `Par(DdTableResults* tablep, ParResults* presp, int vulnerable)`
- `DealerPar(DdTableResults* tablep, ParResultsDealer* presp, int dealer, int vulnerable)`

## Initialization and Resource Functions

Legacy global configuration/lifecycle:

- `SetThreading(int code)`
- `SetMaxThreads(int userThreads)`
- `SetResources(int maxMemoryMB, int maxThreads)`
- `FreeMemory()`

## Utility Functions

- `ErrorMessage(int code, char line[80])`
- `GetDDSInfo(DDSInfo* info)`

## Return Codes

Functions return integer status codes from `api/dll.h`.

Examples:

- `RETURN_NO_FAULT` (success)
- `RETURN_PBN_FAULT`
- `RETURN_TOO_MANY_BOARDS`
- `RETURN_THREAD_INDEX`
- `RETURN_CHUNK_SIZE`

Use `ErrorMessage(code, buffer)` to map codes to human-readable messages.

## Minimal C Example

```c
#include <api/dll.h>

int solve_one(struct Deal dl)
{
    struct FutureTricks fut;
    int rc;

    SetMaxThreads(1);
    SetResources(512, 1);

    rc = SolveBoard(dl, -1, 3, 0, &fut, 0);

    FreeMemory();
    return rc;
}
```
