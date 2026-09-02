/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#pragma once

#include <ostream>
#include <string>
#include <chrono>
#include <ctime>
#include <iostream>

using Clock = std::chrono::steady_clock;

/// @file TestTimer.hpp
/// @brief High-resolution performance timing utility for tests.
/// 
/// Provides a TestTimer class for measuring wall-clock and CPU time
/// of test execution. Useful for performance regression detection
/// and identifying slow test hands.

/// Convert a `clock()` tick delta to milliseconds.
/// Uses floating-point so `1000 * ticks` cannot overflow 32-bit `long`
/// (wasm32 batches longer than ~2.15s when CLOCKS_PER_SEC is 1e6).
long clock_delta_to_ms(clock_t delta);

/// Timer for measuring test performance.
/// Tracks both wall-clock (user) and CPU (system) time for test execution.
class TestTimer
{
  private:
    std::string name_;      ///< Timer name for display
    long count_;            ///< Number of times started/stopped
    long user_cum_;         ///< Cumulative user time (milliseconds)
    long user_cum_old_;     ///< Previous cumulative user time (milliseconds)
    long sys_cum_;          ///< Cumulative system time (milliseconds)
    int pending_hands_;     ///< Hands counted into the open start()/end() batch
    bool sys_time_known_;   ///< False when clock() is unusable (e.g. wasm32)

    std::chrono::time_point<Clock> user0_;  ///< Wall-clock start time
    clock_t sys0_;             ///< CPU start time

  public:

    TestTimer();
    ~TestTimer();

    /// Reset timer to zero.
    void reset();

    /// Mark process-CPU (`clock()`) measurements as unavailable.
    /// Used when `clock()` returns `(clock_t)-1` (seen under wasm32 Emscripten
    /// pthreads, where the process CPU clock is epoch-based and overflows
    /// 32-bit `clock_t`). Also a test seam.
    void mark_sys_time_unavailable();

    /// Whether process-CPU time is available for reporting.
    bool sys_time_known() const;

    /// Set the name for this timer.
    /// @param s Name to display with timer results
    void set_name(const std::string& s);

    /// Start timing an operation.
    /// @param number Number of iterations (for per-iteration reporting)
    void start(const int number = 1);
    
    /// Stop timing and accumulate results.
    void end();

    /// Record one completed batch without wall-clock measurement.
    /// Updates cumulative totals. Used by end() and by unit tests.
    /// Non-positive hands is ignored (no cumulative updates).
    /// @param hands Number of hands in the batch
    /// @param user_ms Batch user (wall) time in milliseconds
    /// @param sys_ms Batch system (CPU) time in milliseconds
    void record(const int hands, const long user_ms, const long sys_ms);

    /// Print timer status while running.
    /// @param reached Number of iterations completed so far
    /// @param number Total number of iterations
    void print_running(
        const int reached,
        const int number);
    
    /// Print basic timer summary.
    void print_basic() const;
    
    /// Print detailed per-hand timer results.
    /// @param out Output stream
    void print_hands(std::ostream& out = std::cout) const;
};
