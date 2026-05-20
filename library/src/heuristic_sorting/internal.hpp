#pragma once

#include <heuristic_sorting/heuristic_sorting.hpp>

/// @defgroup heuristic_sorting_internals Internal Heuristic Sorting Functions
/// @brief Non-public helper functions used by heuristic move sorting.
/// @{

/// @brief Assign weights to moves when leading with trump available.
void weight_alloc_trump0(HeuristicContext& context);
/// @brief Assign weights to moves when leading without trump.
void weight_alloc_nt0(HeuristicContext& context);

/// @brief Assign weights when following hand can follow suit (trump game).
void weight_alloc_trump_notvoid1(HeuristicContext& context);
/// @brief Assign weights when following hand can follow suit (no trump).
void weight_alloc_nt_notvoid1(HeuristicContext& context);
/// @brief Assign weights when following hand is void in led suit (trump game).
void weight_alloc_trump_void1(HeuristicContext& context);
/// @brief Assign weights when following hand is void in led suit (no trump).
void weight_alloc_nt_void1(HeuristicContext& context);

/// @brief Assign weights for second following hand that can follow suit (trump).
void weight_alloc_trump_notvoid2(HeuristicContext& context);
/// @brief Assign weights for second following hand that can follow suit (no trump).
void weight_alloc_nt_notvoid2(HeuristicContext& context);
/// @brief Assign weights for second following hand that is void (trump).
void weight_alloc_trump_void2(HeuristicContext& context);
/// @brief Assign weights for second following hand that is void (no trump).
void weight_alloc_nt_void2(HeuristicContext& context);

/// @brief Assign weights for third hand (can follow suit in both trump/no-trump).
void weight_alloc_combined_notvoid3(HeuristicContext& context);
/// @brief Assign weights for third hand that is void in led suit (trump).
void weight_alloc_trump_void3(HeuristicContext& context);
/// @brief Assign weights for third hand that is void in led suit (no trump).
void weight_alloc_nt_void3(HeuristicContext& context);

/// @brief Rank forcing plays to ace for given rank set.
/// @param context Heuristic context with position data.
/// @param cards4th Fourth player's rank set for this suit.
/// @return Computed rank forcing ace score.
int rank_forces_ace(const HeuristicContext& context, const int cards4th);

/// @brief Calculate top-rank number and move number for rank forcing.
/// @param context Heuristic context with position data.
/// @param ris Rank index set (cards remaining in suit).
/// @param prank Pertinent rank to evaluate.
/// @param top_number [out] Computed top rank number.
/// @param mno [out] Corresponding move number.
void get_top_number(const HeuristicContext& context, const int ris, const int prank, int& top_number, int& mno);

/// @}  // end of heuristic_sorting_internals group

