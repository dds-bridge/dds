#pragma once

#include "heuristic_sorting.h"

// Need to include the actual type definitions for implementation
// These are copied from the main dds files to avoid circular dependencies

struct moveType
{
  int suit;
  int rank;
  int sequence;
  int weight;
};

struct highCardType
{
  int rank;
  int hand;
};

struct pos
{
  unsigned short int rankInSuit[DDS_HANDS][DDS_SUITS];
  unsigned short int aggr[DDS_SUITS];
  unsigned char length[DDS_HANDS][DDS_SUITS];
  int handDist[DDS_HANDS];
  unsigned short int winRanks[50][DDS_SUITS];
  int first[50];
  moveType move[50];
  int handRelFirst;
  int tricksMAX;
  highCardType winner[DDS_SUITS];
  highCardType secondBest[DDS_SUITS];
};

struct absRankType
{
  char rank;
  signed char hand;
};

struct relRanksType
{
  absRankType absRank[15][DDS_SUITS];
};

// Constants and lookup tables from utility library
extern const int partner[DDS_HANDS];
extern const int rho[DDS_HANDS];
extern const int lho[DDS_HANDS];
extern const unsigned short int bitMapRank[15];
extern const unsigned char relRank[16384][15];

// From utility/LookupTables.h
extern const int highestRank[16384];
extern const int lowestRank[16384];
extern const unsigned char counttable[8192];

// Internal helper functions for the heuristic sorting library
void WeightAllocTrump0(HeuristicContext& context);
void WeightAllocNT0(HeuristicContext& context);
void WeightAllocTrumpNotvoid1(HeuristicContext& context);
void WeightAllocNTNotvoid1(HeuristicContext& context);
void WeightAllocTrumpVoid1(HeuristicContext& context);
void WeightAllocNTVoid1(HeuristicContext& context);
void WeightAllocTrumpNotvoid2(HeuristicContext& context);
void WeightAllocNTNotvoid2(HeuristicContext& context);
void WeightAllocTrumpVoid2(HeuristicContext& context);
void WeightAllocNTVoid2(HeuristicContext& context);
void WeightAllocCombinedNotvoid3(HeuristicContext& context);
void WeightAllocTrumpVoid3(HeuristicContext& context);
void WeightAllocNTVoid3(HeuristicContext& context);
