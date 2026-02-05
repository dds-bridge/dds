#pragma once

#include "heuristic_sorting.hpp"

// Internal helper functions for the heuristic sorting library
void weight_alloc_trump0(HeuristicContext& context);
void weight_alloc_nt0(HeuristicContext& context);
void weight_alloc_trump_notvoid1(HeuristicContext& context);
void weight_alloc_nt_notvoid1(HeuristicContext& context);
void weight_alloc_trump_void1(HeuristicContext& context);
void weight_alloc_nt_void1(HeuristicContext& context);
void weight_alloc_trump_notvoid2(HeuristicContext& context);
void weight_alloc_nt_notvoid2(HeuristicContext& context);
void weight_alloc_trump_void2(HeuristicContext& context);
void weight_alloc_nt_void2(HeuristicContext& context);
void weight_alloc_combined_notvoid3(HeuristicContext& context);
void weight_alloc_trump_void3(HeuristicContext& context);
void weight_alloc_nt_void3(HeuristicContext& context);

// Helper functions used by level 2+ weight allocation
int rank_forces_ace(const HeuristicContext& context, const int cards4th);
void get_top_number(const HeuristicContext& context, const int ris, const int prank, int& top_number, int& mno);

