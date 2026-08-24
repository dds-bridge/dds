/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/

#pragma once

#include <api/dll.h>


/**
 * @brief Validate a caller-supplied deal before a double dummy table is built.
 *
 * SolveBoard() rejects malformed deals in board_range_checks() and
 * board_value_checks() before the search runs. The CalcDDtable* entry points
 * did not, so a deal whose hands held unequal numbers of cards reached the
 * search and indexed the relative-rank tables out of bounds. This applies the
 * same three rules SolveBoard() already enforces, so nothing is rejected here
 * that the solver would have accepted:
 *
 *   - every holding is either empty or confined to the rank bits (2..A),
 *   - no card appears in more than one hand,
 *   - all four hands hold the same number of cards.
 *
 * Reachable through CalcDDtable(), CalcDDtablePBN() and CalcAllTables*(), so
 * in particular through any PBN file that is short of a card.
 *
 * @param table_deal Deal to validate.
 * @return RETURN_NO_FAULT when the deal is well formed, otherwise
 *         RETURN_SUIT_OR_RANK, RETURN_DUPLICATE_CARDS or RETURN_CARD_COUNT.
 */
inline auto table_deal_checks(DdTableDeal const & table_deal) -> int
{
  // Ranks 2..A occupy bits 2..14; see Deal::remainCards in dll.h.
  constexpr unsigned rank_mask = 0x7FFCu;

  int cards_in_hand[DDS_HANDS] = {0, 0, 0, 0};

  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      unsigned const holding = table_deal.cards[h][s];

      if ((holding & ~rank_mask) != 0)
        return RETURN_SUIT_OR_RANK;

      for (unsigned bit = holding; bit != 0; bit &= bit - 1)
        cards_in_hand[h]++;
    }
  }

  for (int s = 0; s < DDS_SUITS; s++)
  {
    unsigned seen = 0;
    for (int h = 0; h < DDS_HANDS; h++)
    {
      unsigned const holding = table_deal.cards[h][s];
      if ((seen & holding) != 0)
        return RETURN_DUPLICATE_CARDS;
      seen |= holding;
    }
  }

  for (int h = 1; h < DDS_HANDS; h++)
    if (cards_in_hand[h] != cards_in_hand[0])
      return RETURN_CARD_COUNT;

  return RETURN_NO_FAULT;
}
