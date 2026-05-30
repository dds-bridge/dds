/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <vector>

#include <api/dll.h>


auto solve_all_boards_n_seq(
  Boards const& bds,
  SolvedBoards& solved) -> int;

auto detect_solve_duplicates(
  const Boards& bds,
  std::vector<int>& uniques,
  std::vector<int>& crossrefs) -> void;
