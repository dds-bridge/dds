/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <string>

#include <api/dll.h>

/// @file bridge_solver_runner.hpp
/// @brief Convert DDS PBN deals and parse macroxue/bridge-solver output.

/// Convert a DDS PBN remain-cards string (`N:…`) to a macroxue deal file body.
/// When trump/first are in range, appends strain and lead lines (solve mode).
/// @return Deal text, or empty string on parse failure (optional *error set).
std::string pbn_to_macroxue_deal(
  const std::string& pbn,
  std::string* error = nullptr);

std::string pbn_to_macroxue_deal(
  const std::string& pbn,
  int trump,
  int first,
  std::string* error = nullptr);

/// Parse macroxue `solver` stdout (or RESULTS lines) into a DDS DD table.
/// Trick order on each strain line is S N W E.
/// @return true on success
bool parse_macroxue_solver_stdout(
  const std::string& text,
  DdTableResults& out,
  std::string* error = nullptr);

/// Parse single-strain / single-lead stdout (`D  3  0.00 s …`) to trick count.
bool parse_macroxue_solver_solve_stdout(
  const std::string& text,
  int& tricks,
  std::string* error = nullptr);

/// DDS SolveBoard scores the leading side; fixed-lead bridge-solver reports
/// the other side (declarer). Convert with remaining trick count.
auto leading_side_tricks_from_declarer_side(
  int remaining_tricks,
  int declarer_side_tricks) -> int;

/// Run `solver -i -f FILE -m0` on a temp deal file built from *pbn*.
/// @return true on success
bool run_bridge_solver_table(
  const std::string& binary,
  const std::string& pbn,
  DdTableResults& out,
  std::string* error = nullptr);

/// Run `solver -f FILE -m0` with trump+lead in the deal file; return DDS-style
/// leading-side trick score for SolveBoard comparison.
bool run_bridge_solver_solve(
  const std::string& binary,
  const std::string& pbn,
  int trump,
  int first,
  int& leading_side_tricks,
  std::string* error = nullptr);
