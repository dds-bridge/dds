/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <vector>

#include <api/dll.h>


void play_single_common(
  const int thrId,
  const int bno);

void play_chunk_common(
  const int thrId);

void detect_play_duplicates(
  const Boards& bds,
  std::vector<int>& uniques,
  std::vector<int>& crossrefs);

void copy_play_single(
  const std::vector<int>& crossrefs);
