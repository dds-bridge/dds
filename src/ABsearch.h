/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_ABSEARCH_H
#define DDS_ABSEARCH_H

#include "dds.h"
#include "Memory.h"


bool ABsearch(
  struct pos * posPoint,
  const int target,
  const int depth,
  struct ThreadData * thrp);

bool ABsearch0(
  struct pos * posPoint,
  const int target,
  const int depth,
  struct ThreadData * thrp);

bool ABsearch1(
  struct pos * posPoint,
  const int target,
  const int depth,
  struct ThreadData * thrp);

bool ABsearch2(
  struct pos * posPoint,
  const int target,
  const int depth,
  struct ThreadData * thrp);

bool ABsearch3(
  struct pos * posPoint,
  const int target,
  const int depth,
  struct ThreadData * thrp);

void Make0(
  struct pos * posPoint,
  const int depth,
  moveType const * mply);

void Make1(
  struct pos * posPoint,
  const int depth,
  moveType const * mply);

void Make2(
  struct pos * posPoint,
  const int depth,
  moveType const * mply);

void Make3(
  struct pos * posPoint,
  unsigned short int trickCards[DDS_SUITS],
  const int depth,
  moveType const * mply,
  ThreadData * thrp);

evalType Evaluate(
  pos * posPoint,
  int trump,
  ThreadData * thrp);

#endif
