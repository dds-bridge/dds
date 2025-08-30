#pragma once

#include "heuristic_sorting.h"

// These are the internal functions that implement the heuristic logic.
// They are exposed here for direct testing.

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
