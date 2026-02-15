/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_SOLVEBOARD_H
#define DDS_SOLVEBOARD_H

#include <vector>

#include <api/dll.h>

using namespace std;


auto solve_single_common(
  const int thrId,
  const int bno) -> void;

auto copy_solve_single(
  const vector<int>& crossrefs) -> void;

auto solve_chunk_common(
  const int thrId) -> void;

auto detect_solve_duplicates(
  const Boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs) -> void;

#endif
