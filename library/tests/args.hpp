/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <string>

/// @file args.hpp
/// @brief Command-line argument parsing for test utilities.
///
/// Provides functions to parse and validate command-line options
/// for the dtest driver program. Options include:
/// - Input file specification
/// - Solver type selection (solve, calc, play, par, dealer_par)
/// - Number of threads
/// - Memory allocation
/// - Slow board reporting
/// - Optional min/max per-hand timing summary across batches

/// Print usage information.
/// @param base Command name for usage message
void usage(
    const char base[]);

/// Print current option values.
void print_options();

/// Resolve `-f` / `--file` to an existing regular file.
///
/// Order: (1) `arg` as a literal path; (2) `hands/list{arg}.txt` under the
/// current working directory; (3) the same under `BUILD_WORKING_DIRECTORY` /
/// `BUILD_WORKSPACE_DIRECTORY` (set by `bazel run`); (4) relative to the
/// directory of `argv0` (the usual `bazel-bin/library/tests/dtest` layout).
/// Directories are not accepted (avoids treating e.g. `-f hands` as a file).
/// @return Resolved path, or empty if no regular file is found
std::string resolve_dtest_input_file(
    const std::string& arg,
    const std::string& argv0);

/// True for a rooted absolute path: POSIX `/...`; Windows drive-rooted
/// `C:\...` / `C:/...`, current-drive rooted `\...` / `/...`, or UNC
/// `\\server\share\...` / `//server/share/...`. Drive-relative forms like
/// `C:bin\dtest` are not absolute.
bool is_dtest_absolute_path(const std::string& path);

/// Parse command-line arguments into global options.
/// @param argc Argument count from main()
/// @param argv Argument vector from main()
void read_args(
    int argc,
    char * argv[]);

