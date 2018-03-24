/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2016 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#ifndef DDS_ABSEARCH_H
#define DDS_ABSEARCH_H


#define DDS_POS_LINES 5
#define DDS_HAND_LINES 12
#define DDS_NODE_LINES 4
#define DDS_FULL_LINE 80
#define DDS_HAND_OFFSET 16
#define DDS_HAND_OFFSET2 12
#define DDS_DIAG_WIDTH 34


bool ABsearch(
  struct pos * posPoint,
  int target,
  int depth,
  struct localVarType * thrp);

bool ABsearch0(
  struct pos * posPoint,
  int target,
  int depth,
  struct localVarType * thrp);

bool ABsearch1(
  struct pos * posPoint,
  int target,
  int depth,
  struct localVarType * thrp);

bool ABsearch2(
  struct pos * posPoint,
  int target,
  int depth,
  struct localVarType * thrp);

bool ABsearch3(
  struct pos * posPoint,
  int target,
  int depth,
  struct localVarType * thrp);

void Make0(
  struct pos * posPoint,
  int depth,
  moveType * mply);

void Make1(
  struct pos * posPoint,
  int depth,
  moveType * mply);

void Make2(
  struct pos * posPoint,
  int depth,
  moveType * mply);

void Make3(
  struct pos * posPoint,
  unsigned short int trickCards[DDS_SUITS],
  int depth,
  moveType * mply,
  localVarType * thrp);

evalType Evaluate(
  pos * posPoint,
  int trump,
  localVarType * thrp);

void InitFileTopLevel(
  localVarType * thrp,
  const string& fname);

void InitFileABstats(
  localVarType * thrp,
  const string& fname);

void InitFileABhits(
  int thrId);

void InitFileTTstats(
  int thrId);

void InitFileTimer(
  int thrId);

void InitFileMoves(
  int thrId);

void InitFileScheduler();

void CloseFileTopLevel(
  int thrId);

void CloseFileABhits(
  int thrId);

void DumpTopLevel(
  struct localVarType * thrp,
  int tricks,
  int lower,
  int upper,
  int printMode);

void RankToText(
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS],
  char text[DDS_HAND_LINES][DDS_FULL_LINE]);

#endif
