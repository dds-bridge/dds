/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_PLAYANALYSER_H
#define DDS_PLAYANALYSER_H

#include <vector>

#include <api/dll.h>

using namespace std;


void play_single_common(
  const int thrId,
  const int bno);

void play_chunk_common(
  const int thrId);

void detect_play_duplicates(
  const Boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs);

void copy_play_single(
  const vector<int>& crossrefs);

#endif
