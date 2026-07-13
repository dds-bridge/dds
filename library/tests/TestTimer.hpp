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
#include <iostream>

using Clock = std::chrono::steady_clock;
using std::chrono::time_point;

/// @file TestTimer.hpp
/// @brief High-resolution performance timing utility for tests.
/// 
/// Provides a TestTimer class for measuring wall-clock and CPU time
/// of test execution. Useful for performance regression detection
/// and identifying slow test hands.

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
    double user_min_;       ///< Min per-hand user time across batches (ms)
    double user_max_;       ///< Max per-hand user time across batches (ms)
    double sys_min_;        ///< Min per-hand system time across batches (ms)
    double sys_max_;        ///< Max per-hand system time across batches (ms)
    long batch_count_;      ///< Number of completed batches
    int pending_hands_;     ///< Hands counted into the open start()/end() batch

    time_point<Clock> user0_;  ///< Wall-clock start time
    clock_t sys0_;             ///< CPU start time

  public:

    TestTimer();
    ~TestTimer();

    /// Reset timer to zero.
    void reset();

    /// Set the name for this timer.
    /// @param s Name to display with timer results
    void set_name(const std::string& s);

    /// Start timing an operation.
    /// @param number Number of iterations (for per-iteration reporting)
    void start(const int number = 1);
    
    /// Stop timing and accumulate results.
    void end();

    /// Record one completed batch without wall-clock measurement.
    /// Updates cumulative totals and per-hand min/max extremes.
    /// Used by end() and by unit tests for deterministic extremes.
    /// Non-positive hands is ignored (no cumulative or extreme updates).
    /// @param hands Number of hands in the batch
    /// @param user_ms Batch user (wall) time in milliseconds
    /// @param sys_ms Batch system (CPU) time in milliseconds
    void record(const int hands, const long user_ms, const long sys_ms);

    /// Whether at least one batch has been recorded.
    bool has_batch_times() const;

    /// Minimum per-hand user time across batches (milliseconds).
    double user_min_ms() const;

    /// Maximum per-hand user time across batches (milliseconds).
    double user_max_ms() const;

    /// Minimum per-hand system time across batches (milliseconds).
    double sys_min_ms() const;

    /// Maximum per-hand system time across batches (milliseconds).
    double sys_max_ms() const;

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
    /// @param show_min Include min per-hand times across batches
    /// @param show_max Include max per-hand times across batches
    void print_hands(
        std::ostream& out = std::cout,
        bool show_min = false,
        bool show_max = false) const;
};
