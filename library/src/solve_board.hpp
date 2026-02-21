/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <vector>

#include <api/dll.h>


auto solve_single_common(
  const int thrId,
  const int bno) -> void;

auto copy_solve_single(
  const std::vector<int>& crossrefs) -> void;

auto solve_chunk_common(
  const int thrId) -> void;

auto detect_solve_duplicates(
  const Boards& bds,
  std::vector<int>& uniques,
  std::vector<int>& crossrefs) -> void;
