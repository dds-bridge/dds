/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

/// @file debug.h
/// @brief Debug configuration flags and statistics support.
/// @defgroup utility_debug Debug Utilities
/// @{

/// @brief Debug flag performance impact summary.
///
/// A number of debug flags cause diagnostic output files to be generated.
/// One file per thread is created. Performance impact varies significantly:
///
/// <table>
/// <tr><th>Mode</th><th>Clock Ticks</th><th>File Size (KB)</th></tr>
/// <tr><td>No debugging</td><td>66</td><td>--</td></tr>
/// <tr><td>DDS_TOP_LEVEL</td><td>63</td><td>8</td></tr>
/// <tr><td>DDS_AB_STATS</td><td>71</td><td>36</td></tr>
/// <tr><td>DDS_AB_STATS + DDS_AB_DETAILS</td><td>75</td><td>64</td></tr>
/// <tr><td>DDS_AB_HITS</td><td>680</td><td>12004</td></tr>
/// <tr><td>DDS_TT_STATS</td><td>68</td><td>8</td></tr>
/// <tr><td>DDS_TIMING</td><td>125</td><td>8</td></tr>
/// <tr><td>All Together</td><td>720</td><td>--</td></tr>
/// </table>
///
/// @note These measurements are from a single hand, single-threaded execution
/// with ~207,000 AB nodes and ~63,000 trick nodes. Times are approximate;
/// actual performance depends on hardware and hand complexity.

/// @name Debug Configuration Flags
/// Enable individual debug modes to generate diagnostic output files.
/// @{

/// @brief Enable all debug output files.
/// Convenience flag that enables all debug modes below simultaneously.
// #define DDS_DEBUG_ALL

/// @brief File extension for debug output files.
#define DDS_DEBUG_SUFFIX ".txt"

/// @brief Log data about each call to the top-level AB routine.
/// Generates detailed information about solver entry points and parameters.
// #define DDS_TOP_LEVEL
#define DDS_TOP_LEVEL_PREFIX "toplevel"

/// @brief Enable AB search statistics (node counts, timing, etc.).
/// Records alpha-beta search performance metrics for optimization analysis.
// #define DDS_AB_STATS
#define DDS_AB_STATS_PREFIX "ABstats"

/// @brief Enable detailed AB search statistics.
/// Must be combined with DDS_AB_STATS for enhanced diagnostic output.
// #define DDS_AB_DETAILS

/// @brief Log transposition table hits and misses.
/// Tracks which positions are stored to and retrieved from the TT cache.
// #define DDS_AB_HITS
#define DDS_AB_HITS_RETRIEVED_PREFIX "retrieved"
#define DDS_AB_HITS_STORED_PREFIX "stored"

/// @brief Enable transposition table usage statistics.
/// Reports memory efficiency and hit rates of the position cache.
// #define DDS_TT_STATS
#define DDS_TT_STATS_PREFIX "TTstats"

/// @brief Enable timing of AB search and related functions.
/// Measures execution time and attempts to calculate exclusive (non-overlapping) times.
// #define DDS_TIMING
#define DDS_TIMING_PREFIX "timer"

/// @brief Enable detailed timing breakdown.
/// Requires DDS_TIMING; provides per-function timing details.
// #define DDS_TIMING_DETAILS

/// @brief Enable move generation quality statistics.
/// Analyzes the effectiveness of move ordering heuristics.
// #define DDS_MOVES
#define DDS_MOVES_PREFIX "movestats"

/// @brief Enable detailed move generation statistics.
/// Requires DDS_MOVES; provides per-move-type analysis.
// #define DDS_MOVES_DETAILS

/// @brief Enable scheduler timing (provided by build system).
/// Logs thread pool scheduling decisions and timing.
#define DDS_SCHEDULER_PREFIX "sched"

/// @}

#ifdef DDS_DEBUG_ALL
#define DDS_TOP_LEVEL
#ifndef DDS_AB_STATS
#define DDS_AB_STATS
#endif
#ifndef DDS_AB_DETAILS
#define DDS_AB_DETAILS
#endif
#ifndef DDS_AB_HITS
#define DDS_AB_HITS
#endif
#ifndef DDS_TT_STATS
#define DDS_TT_STATS
#endif
#ifndef DDS_TIMING
#define DDS_TIMING
#endif
#ifndef DDS_TIMING_DETAILS
#define DDS_TIMING_DETAILS
#endif
#ifndef DDS_MOVES
#define DDS_MOVES
#endif
#ifndef DDS_MOVES_DETAILS
#define DDS_MOVES_DETAILS
#endif
#endif

/// @name Performance Counters
/// @brief Statistics counters for profiling and analysis.
/// @{

/// @brief Number of available counter slots for performance tracking.
constexpr int COUNTER_SLOTS = 200;

/// @brief Global array of performance counters.
/// Each thread may use these counters for statistics collection.
/// Size: COUNTER_SLOTS (200) entries.
extern long long counter[COUNTER_SLOTS];

/// @}

/// @}
