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
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp);

bool ABsearch0(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp);

bool ABsearch1(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp);

bool ABsearch2(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp);

bool ABsearch3(
  pos * posPoint,
  const int target,
  const int depth,
  ThreadData * thrp);

void Make0(
  pos * posPoint,
  const int depth,
  moveType const * mply);

void Make1(
  pos * posPoint,
  const int depth,
  moveType const * mply);

void Make2(
  pos * posPoint,
  const int depth,
  moveType const * mply);

void Make3(
  pos * posPoint,
  unsigned short trickCards[DDS_SUITS],
  const int depth,
  moveType const * mply,
  ThreadData * thrp);

evalType Evaluate(
  pos const * posPoint,
  const int trump,
  ThreadData const * thrp);

#endif
