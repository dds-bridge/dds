/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <string>
#include <chrono>

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

    /// Print timer status while running.
    /// @param reached Number of iterations completed so far
    /// @param number Total number of iterations
    void print_running(
        const int reached,
        const int number);
    
    /// Print basic timer summary.
    void print_basic() const;
    
    /// Print detailed per-hand timer results.
    void print_hands() const;
};
