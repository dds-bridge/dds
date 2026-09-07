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
/// @return Deal text, or empty string on parse failure (optional *error set).
std::string pbn_to_macroxue_deal(
  const std::string& pbn,
  std::string* error = nullptr);

/// Parse macroxue `solver` stdout (or RESULTS lines) into a DDS DD table.
/// Trick order on each strain line is S N W E.
/// @return true on success
bool parse_macroxue_solver_stdout(
  const std::string& text,
  DdTableResults& out,
  std::string* error = nullptr);

/// Run `solver -i -f FILE -m0` on a temp deal file built from *pbn*.
/// @return true on success
bool run_bridge_solver_table(
  const std::string& binary,
  const std::string& pbn,
  DdTableResults& out,
  std::string* error = nullptr);
