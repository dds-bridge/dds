/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#include <api/calc_dd_table.hpp>
#include <api/dll.h>
#include <calc_tables.hpp>
#include <pbn.hpp>

auto calc_dd_table(
    const DdTableDeal& table_deal,
    DdTableResults* table_results) -> int
{
    // Create temporary context for single-use calculation
    SolverContext ctx;
    return calc_dd_table(ctx, table_deal, table_results);
}

auto calc_dd_table(
    SolverContext& ctx,
    const DdTableDeal& table_deal,
    DdTableResults* table_results) -> int
{
    Deal dl;
    Boards bo;
    SolvedBoards solved;

    // Convert DdTableDeal to Deal format
    for (int h = 0; h < DDS_HANDS; h++)
        for (int s = 0; s < DDS_SUITS; s++)
            dl.remainCards[h][s] = table_deal.cards[h][s];

    for (int k = 0; k <= 2; k++)
    {
        dl.currentTrickRank[k] = 0;
        dl.currentTrickSuit[k] = 0;
    }

    // Build Boards structure with all 5 strains
    int ind = 0;
    bo.no_of_boards = DDS_STRAINS;

    for (int tr = DDS_STRAINS-1; tr >= 0; tr--)
    {
        dl.trump = tr;
        bo.deals[ind] = dl;
        bo.target[ind] = -1;
        bo.solutions[ind] = 1;
        bo.mode[ind] = 1;
        ind++;
    }

    // Call context-aware internal calculation
    int res = calc_all_boards_n(ctx, &bo, &solved);
    if (res != RETURN_NO_FAULT)
        return res;

    // Populate result table from solved boards
    for (int index = 0; index < DDS_STRAINS; index++)
    {
        int strain = bo.deals[index].trump;

        for (int first = 0; first < DDS_HANDS; first++)
        {
            table_results->res_table[strain][ rho[first] ] =
                13 - solved.solved_board[index].score[first];
        }
    }
    return RETURN_NO_FAULT;
}

auto calc_dd_table_pbn(
    const DdTableDealPBN& table_deal_pbn,
    DdTableResults* table_results) -> int
{
    // Create temporary context for single-use calculation
    SolverContext ctx;
    return calc_dd_table_pbn(ctx, table_deal_pbn, table_results);
}

auto calc_dd_table_pbn(
    SolverContext& ctx,
    const DdTableDealPBN& table_deal_pbn,
    DdTableResults* table_results) -> int
{
    DdTableDeal table_deal;
    if (convert_from_pbn(table_deal_pbn.cards, table_deal.cards) != RETURN_NO_FAULT)
        return RETURN_PBN_FAULT;

    return calc_dd_table(ctx, table_deal, table_results);
}
