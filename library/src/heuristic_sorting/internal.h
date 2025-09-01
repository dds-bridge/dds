#pragma once

#include "heuristic_sorting.h"

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

// Helper functions used by level 2+ weight allocation
int RankForcesAce(const HeuristicContext& context, const int cards4th);
void GetTopNumber(const HeuristicContext& context, const int ris, const int prank, int& topNumber, int& mno);

// Sorting function - sorts moves by weight using hardcoded sorting networks
void MergeSort(moveType* mply, int numMoves);
