/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#pragma once

/// @file dll.h
/// @brief The flat legacy C API: the historical Haglund/Hein entry points.
///
/// This is the public function-declaration surface. It is included only by API
/// consumers (examples, bindings, tests) and by the API implementation files
/// that define these symbols — never by internal solver code, which takes the
/// data model from <api/dds_data_types.hpp> and the constants from
/// <api/dds_constants.hpp> directly.

#include <api/dds_constants.hpp>   // DLLEXPORT / STDCALL / EXTERN_C, DDS_VERSION,
                                   // MAXNOOFBOARDS, MAXNOOFTABLES, RETURN_* / TEXT_*
#include <api/dds_data_types.hpp>  // struct Deal, Boards, FutureTricks, DdTable*, Par*, ...

/**
 * @brief Initialize the solver's static memory.
 *
 * Allocates the transposition-table memory pools, registers scheduler and
 * thread-manager state, and performs one-time lookup-table initialization.
 * This does NOT control the number of worker threads — use the
 * SolveAllBoardsN / CalcAllTablesN family for per-call thread caps.
 */
EXTERN_C DLLEXPORT auto STDCALL InitializeStaticMemory() -> void;

/**
 * @brief Deprecated alias of InitializeStaticMemory().
 *
 * @deprecated Use InitializeStaticMemory(); the thread count argument is
 *             ignored (internal batch threading was removed). In the modern
 *             C++ API, thread count is controlled by the embedding application
 *             (typically one SolverContext per worker thread), or per call via
 *             the SolveAllBoardsN / CalcAllTablesN family.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param userThreads Ignored; retained for backward compatibility.
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. It simply forwards to InitializeStaticMemory().
 */
EXTERN_C DLLEXPORT auto STDCALL SetMaxThreads(
  int userThreads) -> void;

/**
 * @brief Set the threading backend used by the solver.
 *
 * @deprecated Use SolverContext instead - threading is implicit (one context per thread).
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param code Threading backend code (see documentation)
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. The modern C++ API does not require threading configuration;
 * instead, create one SolverContext instance per thread.
 */
EXTERN_C DLLEXPORT auto STDCALL SetThreading(
  int code) -> int;

/**
 * @brief Set memory and thread resources for the solver.
 *
 * @deprecated Use SolverContext with SolverConfig instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param maxMemoryMB Maximum memory in megabytes
 * @param maxThreads Maximum number of threads
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which provides per-instance configuration through SolverConfig.
 */
EXTERN_C DLLEXPORT auto STDCALL SetResources(
  int maxMemoryMB,
  int maxThreads) -> void;

/**
 * @brief Free memory used by the solver.
 *
 * @deprecated Use SolverContext RAII instead - cleanup is automatic.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. The modern C++ API uses RAII (Resource Acquisition Is
 * Initialization) through SolverContext, which automatically cleans up
 * resources when the context goes out of scope. No explicit cleanup needed.
 */
EXTERN_C DLLEXPORT auto STDCALL FreeMemory() -> void;

/**
 * @brief Solve a single bridge Deal using double dummy analysis
 *
 * @deprecated Use SolverContext with the SolveBoard(SolverContext&, ...)
 *             overload instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param dl The Deal to analyze
 * @param target Target number of tricks
 * @param solutions Solution mode (1 = best, 2 = all, etc.)
 * @param mode Analysis mode
 * @param futp Pointer to result structure
 * @param threadIndex Index of thread to use
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which accumulates transposition-table knowledge across calls instead of
 * relying on the internal thread-indexed memory pools.
 */
EXTERN_C DLLEXPORT auto STDCALL SolveBoard(
  struct Deal dl,
  int target,
  int solutions,
  int mode,
  struct FutureTricks * futp,
  int threadIndex) -> int;

/**
 * @brief Solve a single bridge Deal in PBN format using double dummy analysis.
 *
 * @deprecated Use SolverContext with the solve_board_pbn(SolverContext&, ...)
 *             overload instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param dlpbn The PBN Deal to analyze
 * @param target Target number of tricks
 * @param solutions Solution mode
 * @param mode Analysis mode
 * @param futp Pointer to result structure
 * @param thrId Index of thread to use
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which allows the transposition table and thread resources to be reused
 * across calls instead of being reallocated internally on each call.
 */
EXTERN_C DLLEXPORT auto STDCALL SolveBoardPBN(
  struct DealPBN dlpbn,
  int target,
  int solutions,
  int mode,
  struct FutureTricks * futp,
  int thrId) -> int;

/**
 * @brief Calculate the double dummy table for a given Deal.
 *
 * @deprecated Use SolverContext with the calc_dd_table(SolverContext&, ...)
 *             overload instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param tableDeal Deal for which to calculate the table
 * @param tablep Pointer to result table
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which allows the transposition table and thread resources to be reused
 * across calls instead of being reallocated internally on each call.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcDDtable(
  struct DdTableDeal tableDeal,
  struct DdTableResults * tablep) -> int;

/**
 * @brief CalcDDtable with an explicit worker-thread cap.
 *
 * @deprecated Use SolverContext with the calc_dd_table(SolverContext&, ...)
 *             overload instead; the modern API computes the table on the
 *             calling thread, so per-call thread caps no longer apply -
 *             the embedding application controls parallelism (typically one
 *             SolverContext per worker thread).
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param maxThreads Maximum worker threads; <= 0 selects the automatic
 *        (hardware_concurrency) default.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcDDtableN(
  struct DdTableDeal tableDeal,
  struct DdTableResults * tablep,
  int maxThreads) -> int;

/**
 * @brief Calculate the double dummy table for a PBN Deal.
 *
 * @deprecated Use SolverContext with the calc_dd_table_pbn(SolverContext&, ...)
 *             overload instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param tableDealPBN PBN Deal for which to calculate the table
 * @param tablep Pointer to result table
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which allows the transposition table and thread resources to be reused
 * across calls instead of being reallocated internally on each call.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcDDtablePBN(
  struct DdTableDealPBN tableDealPBN,
  struct DdTableResults * tablep) -> int;

/**
 * @brief CalcDDtablePBN with an explicit worker-thread cap.
 *
 * @deprecated Use SolverContext with the calc_dd_table_pbn(SolverContext&, ...)
 *             overload instead; the modern API computes the table on the
 *             calling thread, so per-call thread caps no longer apply -
 *             the embedding application controls parallelism (typically one
 *             SolverContext per worker thread).
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param maxThreads Maximum worker threads; <= 0 selects the automatic
 *        (hardware_concurrency) default.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcDDtablePBNN(
  struct DdTableDealPBN tableDealPBN,
  struct DdTableResults * tablep,
  int maxThreads) -> int;

/**
 * @brief Calculate double dummy tables for multiple deals.
 *
 * @param dealsp Pointer to multiple deals
 * @param mode Analysis mode
 * @param trumpFilter Array of trump suit filters
 * @param resp Pointer to result tables
 * @param presp Pointer to par results
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTables(
  struct DdTableDeals const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTablesRes * resp,
  struct AllParResults * presp) -> int;

/**
 * @brief CalcAllTables with an explicit worker-thread cap.
 *
 * @param maxThreads Maximum worker threads; <= 0 selects the automatic
 *        (hardware_concurrency) default.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTablesN(
  struct DdTableDeals const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTablesRes * resp,
  struct AllParResults * presp,
  int maxThreads) -> int;

/**
 * @brief Calculate double dummy tables for multiple PBN deals.
 *
 * @param dealsp Pointer to multiple PBN deals
 * @param mode Analysis mode
 * @param trumpFilter Array of trump suit filters
 * @param resp Pointer to result tables
 * @param presp Pointer to par results
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTablesPBN(
  struct DdTableDealsPBN const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTablesRes * resp,
  struct AllParResults * presp) -> int;

/**
 * @brief CalcAllTablesPBN with an explicit worker-thread cap.
 *
 * @param maxThreads Maximum worker threads; <= 0 selects the automatic
 *        (hardware_concurrency) default.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTablesPBNN(
  struct DdTableDealsPBN const * dealsp,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTablesRes * resp,
  struct AllParResults * presp,
  int maxThreads) -> int;

/**
 * @brief Unbounded CalcAllTables: any number of deals, one parallel board job.
 *
 * Legacy CalcAllTablesN remains capped at MAXNOOFTABLES. This entry point
 * expands all deal×strain boards and solves them in a single
 * parallel_all_boards_n dispatch (heap-backed), matching the ddss large-batch
 * shape while preserving the fixed-size ABI of the legacy structs.
 *
 * @param numDeals Number of deals (may exceed MAXNOOFTABLES)
 * @param deals Flat array of numDeals deals
 * @param mode Par mode (-1 = no par); par requires all strains and non-null par
 * @param trumpFilter Per-strain filter (0 = include)
 * @param results Output array of numDeals tables
 * @param par Optional par output (numDeals); required when mode requests par
 * @param maxThreads Worker cap; <= 0 means auto
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTablesX(
  int numDeals,
  struct DdTableDeal const * deals,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTableResults * results,
  struct ParResults * par,
  int maxThreads) -> int;

/**
 * @brief PBN variant of CalcAllTablesX.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcAllTablesPBNX(
  int numDeals,
  struct DdTableDealPBN const * deals,
  int mode,
  int const trumpFilter[DDS_STRAINS],
  struct DdTableResults * results,
  struct ParResults * par,
  int maxThreads) -> int;

/**
 * @brief Solve multiple bridge deals in PBN format.
 *
 * @param bop Pointer to multiple PBN deals
 * @param solvedp Pointer to results for solved Boards
 * @return 1 on success, error code otherwise
 */
EXTERN_C DLLEXPORT auto STDCALL SolveAllBoards(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp) -> int;

/**
 * @brief SolveAllBoards with an explicit worker-thread cap.
 *
 * @param maxThreads Maximum worker threads; <= 0 selects the automatic
 *        (hardware_concurrency) default.
 */
EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsN(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp,
  int maxThreads) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsBin(
  struct Boards const * bop,
  struct SolvedBoards * solvedp) -> int;

/**
 * @brief SolveAllBoardsBin with an explicit worker-thread cap.
 *
 * @param maxThreads Maximum worker threads; <= 0 selects the automatic
 *        (hardware_concurrency) default.
 */
EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsBinN(
  struct Boards const * bop,
  struct SolvedBoards * solvedp,
  int maxThreads) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsSeq(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllBoardsBinSeq(
  struct Boards const * bop,
  struct SolvedBoards * solvedp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllChunks(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllChunksBin(
  struct Boards const * bop,
  struct SolvedBoards * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL SolveAllChunksPBN(
  struct BoardsPBN const * bop,
  struct SolvedBoards * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL Par(
  struct DdTableResults const * tablep,
  struct ParResults * presp,
  int vulnerable) -> int;

/**
 * @brief Calculate the double dummy table and par result for a given Deal.
 *
 * @deprecated Use SolverContext with the calc_par(SolverContext&, ...)
 *             overload instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param tableDeal Deal for which to calculate the table
 * @param vulnerable Vulnerability (0 = None, 1 = Both, 2 = NS, 3 = EW)
 * @param tablep Pointer to result table
 * @param presp Pointer to result par information
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which allows the transposition table and thread resources to be reused
 * across calls instead of being reallocated internally on each call.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcPar(
  struct DdTableDeal tableDeal,
  int vulnerable,
  struct DdTableResults * tablep,
  struct ParResults * presp) -> int;

/**
 * @brief Calculate the double dummy table and par result for a PBN Deal.
 *
 * @deprecated Use SolverContext with the calc_par_pbn(SolverContext&, ...)
 *             overload instead.
 *             See docs/api_migration.md for modern C++ API examples.
 *
 * @param tableDealPBN PBN Deal for which to calculate the table
 * @param tablep Pointer to result table
 * @param vulnerable Vulnerability (0 = None, 1 = Both, 2 = NS, 3 = EW)
 * @param presp Pointer to result par information
 * @return 1 on success, error code otherwise
 *
 * This function is part of the legacy C API and is maintained for backward
 * compatibility. New code should use the modern C++ API with SolverContext,
 * which allows the transposition table and thread resources to be reused
 * across calls instead of being reallocated internally on each call.
 */
EXTERN_C DLLEXPORT auto STDCALL CalcParPBN(
  struct DdTableDealPBN tableDealPBN,
  struct DdTableResults * tablep,
  int vulnerable,
  struct ParResults * presp) -> int;

EXTERN_C DLLEXPORT auto STDCALL SidesPar(
  struct DdTableResults const * tablep,
  struct ParResultsDealer sidesRes[2],
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL DealerPar(
  struct DdTableResults const * tablep,
  struct ParResultsDealer * presp,
  int dealer,
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL DealerParBin(
  struct DdTableResults const * tablep,
  struct ParResultsMaster * presp,
  int dealer,
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL SidesParBin(
  struct DdTableResults const * tablep,
  struct ParResultsMaster sidesRes[2],
  int vulnerable) -> int;

EXTERN_C DLLEXPORT auto STDCALL ConvertToDealerTextFormat(
  struct ParResultsMaster const * pres,
  char * resp) -> int;

EXTERN_C DLLEXPORT auto STDCALL ConvertToSidesTextFormat(
  struct ParResultsMaster const * pres,
  struct ParTextResults * resp) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalysePlayBin(
  struct Deal dl,
  struct PlayTraceBin play,
  struct SolvedPlay * solved,
  int thrId) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalysePlayPBN(
  struct DealPBN dlPBN,
  struct PlayTracePBN playPBN,
  struct SolvedPlay * solvedp,
  int thrId) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalyseAllPlaysBin(
  struct Boards const * bop,
  struct PlayTracesBin const * plp,
  struct SolvedPlays * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL AnalyseAllPlaysPBN(
  struct BoardsPBN const * bopPBN,
  struct PlayTracesPBN const * plpPBN,
  struct SolvedPlays * solvedp,
  int chunkSize) -> int;

EXTERN_C DLLEXPORT auto STDCALL GetDDSInfo(
  struct DDSInfo * info) -> void;

EXTERN_C DLLEXPORT auto STDCALL ErrorMessage(
  int code,
  char line[80]) -> void;
