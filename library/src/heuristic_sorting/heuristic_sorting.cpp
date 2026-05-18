#include <heuristic_sorting/internal.hpp>
#include <utility/constants.h>
#include <lookup_tables/lookup_tables.hpp>

// New overload: accepts a pre-built HeuristicContext. This contains the
// same inline logic that used to be in the previous function body.
void call_heuristic(const HeuristicContext& context)
{
  // Determine which position in trick (0=leading, 1-3=following)
  int hand_rel = 0;
  if (context.curr_hand != context.lead_hand) {
    // Calculate relative position: 1, 2, or 3 based on lead hand
    hand_rel = (context.curr_hand + 4 - context.lead_hand) % 4;
  }

  // Leading hand (hand_rel == 0) - MoveGen0 logic
  if (hand_rel == 0) {
    // Check if trump game with trump winner available
    bool trump_game = (context.trump != DDS_NOTRUMP) && 
            (context.trump >= 0 && context.trump < DDS_SUITS) &&
            (context.tpos.winner[context.trump].rank != 0);
      
    if (trump_game) {
      weight_alloc_trump0(const_cast<HeuristicContext&>(context));
    } else {
      weight_alloc_nt0(const_cast<HeuristicContext&>(context));
    }
    return;
  }

  // Following hands (hand_rel 1-3) - MoveGen123 logic
  // Check trump game condition
  int ftest = ((context.trump != DDS_NOTRUMP) &&
         (context.trump >= 0 && context.trump < DDS_SUITS) &&
         (context.tpos.winner[context.trump].rank != 0) ? 1 : 0);

  // Check if current hand can follow suit (not void)
  unsigned short ris = context.tpos.rank_in_suit[context.curr_hand][context.lead_suit];
  bool can_follow_suit = (ris != 0);

  // Calculate function index using same logic as original
  int findex;
  if (can_follow_suit) {
    findex = 4 * hand_rel + ftest;
  } else {
    findex = 4 * hand_rel + ftest + 2;
  }

  // Following hands function dispatch table (MoveGen123 logic)
  switch (findex) {
    case 4:  weight_alloc_nt_notvoid1(const_cast<HeuristicContext&>(context)); break;  // hand_rel=1, can follow, no trump
    case 5:  weight_alloc_trump_notvoid1(const_cast<HeuristicContext&>(context)); break;  // hand_rel=1, can follow, trump
    case 6:  weight_alloc_nt_void1(const_cast<HeuristicContext&>(context)); break;  // hand_rel=1, void, no trump
    case 7:  weight_alloc_trump_void1(const_cast<HeuristicContext&>(context)); break;  // hand_rel=1, void, trump
    case 8:  weight_alloc_nt_notvoid2(const_cast<HeuristicContext&>(context)); break;  // hand_rel=2, can follow, no trump
    case 9:  weight_alloc_trump_notvoid2(const_cast<HeuristicContext&>(context)); break;  // hand_rel=2, can follow, trump
    case 10: weight_alloc_nt_void2(const_cast<HeuristicContext&>(context)); break; // hand_rel=2, void, no trump
    case 11: weight_alloc_trump_void2(const_cast<HeuristicContext&>(context)); break; // hand_rel=2, void, trump
    case 12: weight_alloc_combined_notvoid3(const_cast<HeuristicContext&>(context)); break; // hand_rel=3, can follow, no trump
    case 13: weight_alloc_combined_notvoid3(const_cast<HeuristicContext&>(context)); break; // hand_rel=3, can follow, trump
    case 14: weight_alloc_nt_void3(const_cast<HeuristicContext&>(context)); break; // hand_rel=3, void, no trump
    case 15: weight_alloc_trump_void3(const_cast<HeuristicContext&>(context)); break; // hand_rel=3, void, trump
    default: 
      // Should not happen, but default to basic sorting
      break;
  }
}

// The following functions are extracted from Moves.cpp and refactored to be
// standalone. They now accept a HeuristicContext struct which contains all the
// necessary state that was previously accessed as members of the Moves class.

void weight_alloc_trump0(HeuristicContext& context)
{
  const unsigned short suitCount = context.tpos.length[context.lead_hand][context.suit];
  const unsigned short suit_count_lh = context.tpos.length[lho[context.lead_hand]][context.suit];
  const unsigned short suit_count_rh = context.tpos.length[rho[context.lead_hand]][context.suit];
  const unsigned short aggr = context.tpos.aggr[context.suit];

  // Why?
  int countLH = (suit_count_lh == 0 ? context.curr_trick + 1 : suit_count_lh) << 2;
  int countRH = (suit_count_rh == 0 ? context.curr_trick + 1 : suit_count_rh) << 2;

  int suit_weight_d = - (((countLH + countRH) << 5) / 13);

  for (int k = context.last_num_moves; k < context.num_moves; k++)
  {
    int suit_bonus = 0;
    bool win_move = false;

    int r_rank = rel_rank[aggr][context.mply[k].rank];

    /* Discourage suit if LHO or RHO can ruff. */
    if ((context.suit != context.trump) &&
        (((context.tpos.rank_in_suit[lho[context.lead_hand]][context.suit] == 0) &&
          (context.tpos.rank_in_suit[lho[context.lead_hand]][context.trump] != 0)) ||
         ((context.tpos.rank_in_suit[rho[context.lead_hand]][context.suit] == 0) &&
          (context.tpos.rank_in_suit[rho[context.lead_hand]][context.trump] != 0))))
      suit_bonus = -12;

    /* Encourage suit if partner can ruff. */
    if ((context.suit != context.trump) &&
        (context.tpos.length[partner[context.lead_hand]][context.suit] == 0) &&
        (context.tpos.length[partner[context.lead_hand]][context.trump] > 0) &&
        (suit_count_rh > 0))
      suit_bonus += 17;

    /* Discourage suit if RHO has high card. */
    if ((context.tpos.winner[context.suit].hand == rho[context.lead_hand]) ||
        (context.tpos.second_best[context.suit].hand == rho[context.lead_hand]))
    {
      if (suit_count_rh != 1)
        suit_bonus += -12;
    }

    /* Try suit if LHO has winning card and partner second best.
       Exception: partner has singleton. */

    else if ((context.tpos.winner[context.suit].hand == lho[context.lead_hand]) &&
             (context.tpos.second_best[context.suit].hand == partner[context.lead_hand]))
    {
      /* This case was suggested by Joel Bradmetz. */
      if (context.tpos.length[partner[context.lead_hand]][context.suit] != 1)
        suit_bonus += 27;
    }

    /* Encourage play of suit where partner wins and
       returns the suit for a ruff. */
    if ((context.suit != context.trump) && (suitCount == 1) &&
        (context.tpos.length[context.lead_hand][context.trump] > 0) &&
        (context.tpos.length[partner[context.lead_hand]][context.suit] > 1) &&
        (context.tpos.winner[context.suit].hand == partner[context.lead_hand]))
      suit_bonus += 19;


    /* Discourage a suit selection where the search tree appears larger
       than for the altenative suits: the search is estimated to be
       small when the added number of alternative cards to play for
       the opponents is small. */

    int suit_weight_delta = suit_bonus + suit_weight_d;

    if (context.tpos.winner[context.suit].rank == context.mply[k].rank)
    {
      if ((context.suit != context.trump))
      {
        if ((context.tpos.length[partner[context.lead_hand]][context.suit] != 0) ||
            (context.tpos.length[partner[context.lead_hand]][context.trump] == 0))
        {
          if (((context.tpos.length[lho[context.lead_hand]][context.suit] != 0) ||
               (context.tpos.length[lho[context.lead_hand]][context.trump] == 0)) &&
              ((context.tpos.length[rho[context.lead_hand]][context.suit] != 0) ||
               (context.tpos.length[rho[context.lead_hand]][context.trump] == 0)))
            win_move = true;
        }
        else if (((context.tpos.length[lho[context.lead_hand]][context.suit] != 0) ||
                  (context.tpos.rank_in_suit[partner[context.lead_hand]][context.trump] >
                   context.tpos.rank_in_suit[lho[context.lead_hand]][context.trump])) &&
                 ((context.tpos.length[rho[context.lead_hand]][context.suit] != 0) ||
                  (context.tpos.rank_in_suit[partner[context.lead_hand]][context.trump] >
                   context.tpos.rank_in_suit[rho[context.lead_hand]][context.trump])))
          win_move = true;
      }
      else
        win_move = true;
    }
    else if (context.tpos.rank_in_suit[partner[context.lead_hand]][context.suit] >
             (context.tpos.rank_in_suit[lho[context.lead_hand]][context.suit] |
              context.tpos.rank_in_suit[rho[context.lead_hand]][context.suit]))
    {
      if (context.suit != context.trump)
      {
        if (((context.tpos.length[lho[context.lead_hand]][context.suit] != 0) ||
             (context.tpos.length[lho[context.lead_hand]][context.trump] == 0)) &&
            ((context.tpos.length[rho[context.lead_hand]][context.suit] != 0) ||
             (context.tpos.length[rho[context.lead_hand]][context.trump] == 0)))
          win_move = true;
      }
      else
        win_move = true;
    }
    else if (context.suit != context.trump)
    {
      if ((context.tpos.length[partner[context.lead_hand]][context.suit] == 0) &&
          (context.tpos.length[partner[context.lead_hand]][context.trump] != 0))
      {
        if ((context.tpos.length[lho[context.lead_hand]][context.suit] == 0) &&
            (context.tpos.length[lho[context.lead_hand]][context.trump] != 0) &&
            (context.tpos.length[rho[context.lead_hand]][context.suit] == 0) &&
            (context.tpos.length[rho[context.lead_hand]][context.trump] != 0))
        {
          if (context.tpos.rank_in_suit[partner[context.lead_hand]][context.trump] >
              (context.tpos.rank_in_suit[lho[context.lead_hand]][context.trump] |
               context.tpos.rank_in_suit[rho[context.lead_hand]][context.trump]))
            win_move = true;
        }
        else if ((context.tpos.length[lho[context.lead_hand]][context.suit] == 0) &&
                 (context.tpos.length[lho[context.lead_hand]][context.trump] != 0))
        {
          if (context.tpos.rank_in_suit[partner[context.lead_hand]][context.trump]
              > context.tpos.rank_in_suit[lho[context.lead_hand]][context.trump])
            win_move = true;
        }
        else if ((context.tpos.length[rho[context.lead_hand]][context.suit] == 0) &&
                 (context.tpos.length[rho[context.lead_hand]][context.trump] != 0))
        {
          if (context.tpos.rank_in_suit[partner[context.lead_hand]][context.trump]
              > context.tpos.rank_in_suit[rho[context.lead_hand]][context.trump])
            win_move = true;
        }
        else
          win_move = true;
      }
    }

    if (win_move)
    {
      /* Encourage ruffing LHO or RHO singleton, highest card. */
      if (((suit_count_lh == 1) &&
           (context.tpos.winner[context.suit].hand == lho[context.lead_hand]))
          || ((suit_count_rh == 1) &&
              (context.tpos.winner[context.suit].hand == rho[context.lead_hand])))
        context.mply[k].weight = suit_weight_delta + 35 + r_rank;

      /* Lead hand has the highest card. */

      else if (context.tpos.winner[context.suit].hand == context.lead_hand)
      {
        /* Also, partner has second highest card. */
        if (context.tpos.second_best[context.suit].hand == partner[context.lead_hand])
          context.mply[k].weight = suit_weight_delta + 48 + r_rank;
        else if (context.tpos.winner[context.suit].rank == context.mply[k].rank)
          /* If the current card to play is the highest card. */
          context.mply[k].weight = suit_weight_delta + 31;
        else
          context.mply[k].weight = suit_weight_delta - 3 + r_rank;
      }
      else if (context.tpos.winner[context.suit].hand == partner[context.lead_hand])
      {
        /* If partner has highest card */
        if (context.tpos.second_best[context.suit].hand == context.lead_hand)
          context.mply[k].weight = suit_weight_delta + 42 + r_rank;
        else
          context.mply[k].weight = suit_weight_delta + 28 + r_rank;
      }
      /* Encourage playing second highest rank if hand also has
         third highest rank. */
      else if ((context.mply[k].sequence) &&
               (context.mply[k].rank == context.tpos.second_best[context.suit].rank))
        context.mply[k].weight = suit_weight_delta + 40;
      else if (context.mply[k].sequence)
        context.mply[k].weight = suit_weight_delta + 22 + r_rank;
      else
        context.mply[k].weight = suit_weight_delta + 11 + r_rank;

      /* playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table entry
         match. */

      // Only use best_move/best_move_tt if they're valid (non-empty)
      if ((context.best_move.rank > 0) && 
          (context.best_move.suit == context.suit) &&
          (context.best_move.rank == context.mply[k].rank))
        context.mply[k].weight += 55;
      else if ((context.best_move_tt.rank > 0) &&
               (context.best_move_tt.suit == context.suit) &&
               (context.best_move_tt.rank == context.mply[k].rank))
        context.mply[k].weight += 18;
    }
    else
    {
      /* Encourage playing the suit if the hand together with partner
         have both the 2nd highest and the 3rd highest cards such that
         the side of the hand has the highest card in the next round
         playing this suit. */

      int thirdBestHand = context.thrp_rel[aggr].abs_rank[3][context.suit].hand;

      if ((context.tpos.second_best[context.suit].hand == partner[context.lead_hand]) &&
          (partner[context.lead_hand] == thirdBestHand))
        suit_weight_delta += 20;
      else if (((context.tpos.second_best[context.suit].hand == context.lead_hand) &&
                (partner[context.lead_hand] == thirdBestHand) &&
                (context.tpos.length[partner[context.lead_hand]][context.suit] > 1)) ||
               ((context.tpos.second_best[context.suit].hand == partner[context.lead_hand]) &&
                (context.lead_hand == thirdBestHand) &&
                (context.tpos.length[partner[context.lead_hand]][context.suit] > 1)))
        suit_weight_delta += 13;

      /* Higher weight if LHO or RHO has the highest (winning) card as
         a singleton. */

      if (((suit_count_lh == 1) &&
           (context.tpos.winner[context.suit].hand == lho[context.lead_hand]))
          || ((suit_count_rh == 1) &&
              (context.tpos.winner[context.suit].hand == rho[context.lead_hand])))
        context.mply[k].weight = suit_weight_delta + r_rank + 2;
      else if (context.tpos.winner[context.suit].hand == context.lead_hand)
      {
        if (context.tpos.second_best[context.suit].hand == partner[context.lead_hand])
          /* Opponents win by ruffing */
          context.mply[k].weight = suit_weight_delta + 33 + r_rank;
        else if (context.tpos.winner[context.suit].rank == context.mply[k].rank)
          /* Opponents win by ruffing */
          context.mply[k].weight = suit_weight_delta + 38;
        else
          context.mply[k].weight = suit_weight_delta - 14 + r_rank;
      }
      else if (context.tpos.winner[context.suit].hand == partner[context.lead_hand])
      {
        /* Opponents win by ruffing */
        context.mply[k].weight = suit_weight_delta + 34 + r_rank;
      }
      /* Encourage playing second highest rank if hand also has
         third highest rank. */
      else if ((context.mply[k].sequence) &&
               (context.mply[k].rank == context.tpos.second_best[context.suit].rank))
        context.mply[k].weight = suit_weight_delta + 35;
      else
        context.mply[k].weight = suit_weight_delta + 17 - (context.mply[k].rank);

      /* Encourage playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table
         entry match. */

      if ((context.best_move.rank > 0) && 
          (context.best_move.suit == context.suit) &&
          (context.best_move.rank == context.mply[k].rank))
        context.mply[k].weight += 18;
    }
  }
}

// Placeholder for the rest of the functions to be moved
void weight_alloc_nt0(HeuristicContext& context) {
  int aggr = context.tpos.aggr[context.suit];

  /* Discourage a suit selection where the search tree appears larger
     than for the alternative suits: the search is estimated to be
     small when the added number of alternative cards to play for
     the opponents is small. */

  unsigned short suit_count_lh = context.tpos.length[lho[context.lead_hand]][context.suit];
  unsigned short suit_count_rh = context.tpos.length[rho[context.lead_hand]][context.suit];

  // Why?
  int countLH = (suit_count_lh == 0 ? context.curr_trick + 1 : suit_count_lh) << 2;
  int countRH = (suit_count_rh == 0 ? context.curr_trick + 1 : suit_count_rh) << 2;

  int suit_weight_d = - (((countLH + countRH) << 5) / 19);
  if (context.tpos.length[partner[context.lead_hand]][context.suit] == 0)
    suit_weight_d += -9;

  for (int k = context.last_num_moves; k < context.num_moves; k++)
  {
    int suit_weight_delta = suit_weight_d;
    int r_rank = rel_rank[aggr][context.mply[k].rank];

    if (context.tpos.winner[context.suit].rank == context.mply[k].rank ||
        (context.tpos.rank_in_suit[partner[context.lead_hand]][context.suit] >
         (context.tpos.rank_in_suit[lho[context.lead_hand]][context.suit] |
          context.tpos.rank_in_suit[rho[context.lead_hand]][context.suit])))
    {
      // Can win trick, ourselves or partner.
      // FIX: No distinction?
      /* Discourage suit if RHO has second best card.
         Exception: RHO has singleton. */
      if (context.tpos.second_best[context.suit].hand == rho[context.lead_hand])
      {
        if (suit_count_rh != 1)
          suit_weight_delta += -1;
      }
      /* Encourage playing suit if LHO has second highest rank. */
      else if (context.tpos.second_best[context.suit].hand == lho[context.lead_hand])
      {
        if (suit_count_lh != 1)
          suit_weight_delta += 22;
        else
          suit_weight_delta += 16;
      }

      /* Higher weight if also second best rank is present on
         current side to play, or if second best is a singleton
         at LHO or RHO. */

      if (((context.tpos.second_best[context.suit].hand != lho[context.lead_hand])
           || (suit_count_lh == 1)) &&
          ((context.tpos.second_best[context.suit].hand != rho[context.lead_hand])
           || (suit_count_rh == 1)))
        context.mply[k].weight = suit_weight_delta + 45 + r_rank;
      else
        context.mply[k].weight = suit_weight_delta + 18 + r_rank;

      /* Encourage playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table
         entry match. */

      if ((context.best_move.rank > 0) && 
          (context.best_move.suit == context.suit) &&
          (context.best_move.rank == context.mply[k].rank))
        context.mply[k].weight += 126;
      else if ((context.best_move_tt.rank > 0) &&
               (context.best_move_tt.suit == context.suit) &&
               (context.best_move_tt.rank == context.mply[k].rank))
        context.mply[k].weight += 32;
    }
    else
    {
      /* Discourage suit if RHO has winning or second best card.
         Exception: RHO has singleton. */

      if ((context.tpos.winner[context.suit].hand == rho[context.lead_hand]) ||
          (context.tpos.second_best[context.suit].hand == rho[context.lead_hand]))
      {
        if (suit_count_rh != 1)
          suit_weight_delta += -10;
      }

      /* Try suit if LHO has winning card and partner second best.
         Exception: partner has singleton. */

      else if ((context.tpos.winner[context.suit].hand == lho[context.lead_hand]) &&
               (context.tpos.second_best[context.suit].hand == partner[context.lead_hand]))
      {
        /* This case was suggested by Joel Bradmetz. */
        if (context.tpos.length[partner[context.lead_hand]][context.suit] != 1)
          suit_weight_delta += 31;
      }

      /* Encourage playing the suit if the hand together with partner
         have both the 2nd highest and the 3rd highest cards such
         that the side of the hand has the highest card in the
         next round playing this suit. */

      int thirdBestHand = context.thrp_rel[aggr].abs_rank[3][context.suit].hand;

      if ((context.tpos.second_best[context.suit].hand == partner[context.lead_hand]) &&
          (partner[context.lead_hand] == thirdBestHand))
        suit_weight_delta += 35;
      else if (((context.tpos.second_best[context.suit].hand == context.lead_hand) &&
                (partner[context.lead_hand] == thirdBestHand) &&
                (context.tpos.length[partner[context.lead_hand]][context.suit] > 1)) ||
               ((context.tpos.second_best[context.suit].hand == partner[context.lead_hand]) &&
                (context.lead_hand == thirdBestHand) &&
                (context.tpos.length[partner[context.lead_hand]][context.suit] > 1)))
        suit_weight_delta += 25;

      /* Higher weight if LHO or RHO has the highest (winning) card
         as a singleton. */

      if (((suit_count_lh == 1) &&
           (context.tpos.winner[context.suit].hand == lho[context.lead_hand]))
          || ((suit_count_rh == 1) &&
              (context.tpos.winner[context.suit].hand == rho[context.lead_hand])))
        context.mply[k].weight = suit_weight_delta + 28 + r_rank;
      else if (context.tpos.winner[context.suit].hand == context.lead_hand)
        context.mply[k].weight = suit_weight_delta - 17 + r_rank;
      else if (! context.mply[k].sequence)
        context.mply[k].weight = suit_weight_delta + 12 + r_rank;
      else if (context.mply[k].rank == context.tpos.second_best[context.suit].rank)
        context.mply[k].weight = suit_weight_delta + 48;
      else
        context.mply[k].weight = suit_weight_delta + 29 - r_rank;

      /* Encourage playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table
         entry match. */

      if ((context.best_move.rank > 0) && 
          (context.best_move.suit == context.suit) && 
          (context.best_move.rank == context.mply[k].rank))
        context.mply[k].weight += 47;
      else if ((context.best_move_tt.rank > 0) &&
               (context.best_move_tt.suit == context.suit) &&
               (context.best_move_tt.rank == context.mply[k].rank))
        context.mply[k].weight += 19;
    }
  }
}
void weight_alloc_trump_notvoid1(HeuristicContext& ctx)
{
  const Pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;
  // trackp not needed here; use context snapshots for trick state.


  const int max3rd = highest_rank[
                 tpos.rank_in_suit[partner[lead_hand]][lead_suit]];
  const int maxpd = highest_rank[
                 tpos.rank_in_suit[rho[lead_hand] ][lead_suit]];
  const int min3rd = lowest_rank [
                 tpos.rank_in_suit[partner[lead_hand]][lead_suit]];
  const int minpd = lowest_rank [
                 tpos.rank_in_suit[rho[lead_hand] ][lead_suit]];

  for (int k = 0; k < num_moves; k++)
  {
    bool win_move = false; /* If true, current move can win trick. */
  int r_rank = rel_rank[ tpos.aggr[lead_suit] ][mply[k].rank];

    if (lead_suit == trump)
    {
      if (maxpd > ctx.lead0_rank && maxpd > max3rd)
        win_move = true;
      else if (mply[k].rank > ctx.lead0_rank &&
               mply[k].rank > max3rd)
        win_move = true;
    }
    else
    {
      if (mply[k].rank > ctx.lead0_rank && mply[k].rank > max3rd)
      {
        if ((max3rd != 0) ||
            (tpos.length[partner[lead_hand]][trump] == 0))
          win_move = true;
        else if ((maxpd == 0)
                 && (tpos.length[rho[lead_hand]][trump] != 0)
                 && (tpos.rank_in_suit[rho[lead_hand]][trump] >
                     tpos.rank_in_suit[partner[lead_hand]][trump]))
          win_move = true;
      }
  else if (maxpd > ctx.lead0_rank && maxpd > max3rd)
      {
        if ((max3rd != 0) ||
            (tpos.length[partner[lead_hand]][trump] == 0))
          win_move = true;
      }
  else if (ctx.lead0_rank > maxpd &&
           ctx.lead0_rank > max3rd &&
           ctx.lead0_rank > mply[k].rank)
      {
        if ((maxpd == 0) && (tpos.length[rho[lead_hand]][trump] != 0))
        {
          if ((max3rd != 0) ||
              (tpos.length[partner[lead_hand]][trump] == 0))
            win_move = true;
          else if (tpos.rank_in_suit[rho[lead_hand]][trump]
                   > tpos.rank_in_suit[partner[lead_hand]][trump])
            win_move = true;
        }
      }
      else if (maxpd == 0 && tpos.length[rho[lead_hand]][trump] != 0)
        /* winnerHand is partner to first */
        win_move = true;
    }

    if (win_move)
    {
      if (min3rd > mply[k].rank)
        // Partner must be winning -- we can't.
        mply[k].weight = 40 + r_rank;
      else if ((maxpd > ctx.lead0_rank) &&
               (tpos.rank_in_suit[lead_hand][lead_suit] >
                tpos.rank_in_suit[rho[lead_hand]][lead_suit]))
        mply[k].weight = 41 + r_rank;

      /* If rho has a card in the leading suit that
         is higher than the trick leading card but lower
         than the highest rank of the leading hand, then
         lho playing the lowest card will be the cheapest win */

      // FIX: Don't follow

  else if (mply[k].rank > ctx.lead0_rank)
      {
        if (mply[k].rank < maxpd)
          mply[k].weight = 78 - (mply[k].rank);
        /* If played card is lower than any of the cards of
           rho, it will be the cheapest win */
        else if (mply[k].rank > max3rd)
          mply[k].weight = 73 - (mply[k].rank);
        /* If played card is higher than any cards at partner
           of the leading hand, rho can play low, under the
           condition that he has a lower card than lho played */
        else if (mply[k].sequence) // May establish a winner
          mply[k].weight = 62 - (mply[k].rank);
        else
          mply[k].weight = 49 - (mply[k].rank);
      }
      else if (maxpd > 0)
        mply[k].weight = 47 - (mply[k].rank);
      else
        mply[k].weight = 40 - (mply[k].rank);
    }
    else if (mply[k].rank < min3rd || mply[k].rank < minpd)
      // Will be beaten anyway.
      mply[k].weight = -9 + r_rank;
    else if (mply[k].rank < ctx.lead0_rank)
      // Already beaten.
      mply[k].weight = -16 + r_rank;
    else if (mply[k].sequence)
      // May establish a winner
      mply[k].weight = 22 - (mply[k].rank);
    else
      mply[k].weight = 10 - (mply[k].rank);
  }
}

void weight_alloc_nt_notvoid1(HeuristicContext& ctx)
{
  // Faithful port of Moves::weight_alloc_nt_notvoid1(const Pos& tpos)
  const Pos& tpos = ctx.tpos;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;
  // trackp not needed; using snapshot lead0_rank.

  const int partner_lh = partner[lead_hand];
  const int rho_lh = rho[lead_hand];

  // Original logic from Moves::weight_alloc_nt_notvoid1
  const int max3rd = highest_rank[
    tpos.rank_in_suit[partner_lh][lead_suit]];
  const int maxpd = highest_rank[
    tpos.rank_in_suit[rho_lh][lead_suit] ];

  if (maxpd > ctx.lead0_rank && maxpd > max3rd)
  {
    // Partner can beat both opponents.
    for (int k = 0; k < num_moves; k++)
      mply[k].weight = -mply[k].rank;
  }
  else
  {
  int min3rd = lowest_rank [
                   tpos.rank_in_suit[partner_lh][lead_suit]];
  int minpd = lowest_rank [
                   tpos.rank_in_suit[rho_lh][lead_suit] ];

    for (int k = 0; k < num_moves; k++)
    {
      int r_rank = rel_rank[ tpos.aggr[lead_suit] ][mply[k].rank];

      if (mply[k].rank > ctx.lead0_rank && mply[k].rank > max3rd)
        // We can beat both opponents.
        mply[k].weight = 81 - mply[k].rank;

      else if ((min3rd > mply[k].rank) || (minpd > mply[k].rank))
        // Card can make no difference, so play very low.
        mply[k].weight = -3 + r_rank;

      else if (mply[k].rank < ctx.lead0_rank)
        // Can't beat the card led.
        mply[k].weight = -11 + r_rank;

      else if (mply[k].sequence)
        // Some willingness to split.
        mply[k].weight = 10 + r_rank;

      else
        mply[k].weight = 13 - mply[k].rank;
    }
  }
}
void weight_alloc_trump_void1(HeuristicContext& ctx)
{
  const Pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int curr_hand = ctx.curr_hand;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int last_num_moves = ctx.last_num_moves;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;
  // trackp not needed here; use context snapshots for trick state.

  const int partner_lh = partner[lead_hand];
  const int rho_lh = rho[lead_hand];
  
  unsigned short suitCount = tpos.length[curr_hand][suit];
  int suitAdd;

  if (suit == trump)
  {
    // We trump a non-trump card.
    
    if (tpos.length[partner_lh][lead_suit] != 0)
    {
      // 3rd hand will follow.
  if ((tpos.rank_in_suit[rho_lh][lead_suit] >
       (tpos.rank_in_suit[partner_lh][lead_suit] |
    bit_map_rank[ctx.lead0_rank])) ||
          ((tpos.length[rho_lh][lead_suit] == 0) &&
           (tpos.length[rho_lh][trump] != 0)))
      {
        // Partner can win with a card or by ruffing.
        suitAdd = 60 + (suitCount << 6) / 44;
      }
      else
      {
        suitAdd = -2 + (suitCount << 6) / 36;
        // Don't ruff from Kx.
        if ((suitCount == 2) &&
            (tpos.second_best[suit].hand == curr_hand))
          suitAdd += -4;
      }
    }
    else if ((tpos.length[rho_lh][lead_suit] == 0) &&
             (tpos.rank_in_suit[rho_lh][trump] >
              tpos.rank_in_suit[partner_lh][trump]))
    {
      // Partner can overruff 3rd hand.
      suitAdd = 60 + (suitCount << 6) / 44;
    }
  else if ((tpos.length[partner_lh][trump] == 0) &&
       (tpos.rank_in_suit[rho_lh][lead_suit] >
        bit_map_rank[ctx.lead0_rank]))
    {
      // 3rd hand has no trumps, and partner has suit winner.
      suitAdd = 60 + (suitCount << 6) / 44;
    }
    else
    {
      suitAdd = -2 + (suitCount << 6) / 36;
      // Don't ruff from Kx.
      if ((suitCount == 2) &&
          (tpos.second_best[suit].hand == curr_hand))
        suitAdd += -4;
    }

    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else if (suit != trump)
  {
    // We discard on a side suit.

    if (tpos.length[partner_lh][lead_suit] != 0)
    {
      // 3rd hand will follow.
    if (tpos.rank_in_suit[rho_lh][lead_suit] >
      (tpos.rank_in_suit[partner_lh][lead_suit] |
       bit_map_rank[ctx.lead0_rank]))
        // Partner has winning card.
        suitAdd = 60 + (suitCount << 6) / 44;
      else if ((tpos.length[rho_lh][lead_suit] == 0)
               && (tpos.length[rho_lh][trump] != 0))
        // Partner can ruff.
        suitAdd = 60 + (suitCount << 6) / 44;
      else
      {
        // FIX: No reason to differentiate here?
        suitAdd = -2 + (suitCount << 6) / 36;
        // Don't pitch from Kx.
        if ((suitCount == 2) &&
            (tpos.second_best[suit].hand == curr_hand))
          suitAdd += -4;
      }
    }
    else if ((tpos.length[rho_lh][lead_suit] == 0)
             && (tpos.rank_in_suit[rho_lh][trump] >
                 tpos.rank_in_suit[partner_lh][trump]))
      // Partner can overruff 3rd hand.
      suitAdd = 60 + (suitCount << 6) / 44;
  else if ((tpos.length[partner_lh][trump] == 0)
       && (tpos.rank_in_suit[rho_lh][lead_suit] >
         bit_map_rank[ctx.lead0_rank]))
      // 3rd hand has no trumps, and partner has suit winner.
      suitAdd = 60 + (suitCount << 6) / 44;
    else
    {
      // FIX: No reason to differentiate here?
      suitAdd = -2 + (suitCount << 6) / 36;
      // Don't pitch from Kx.
      if ((suitCount == 2) &&
          (tpos.second_best[suit].hand == curr_hand))
        suitAdd += -4;
    }
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else if (tpos.length[partner_lh][lead_suit] != 0)
  {
    // 3rd hand follows suit while we ruff.
    // Could be ruffing partner's winner!
    suitAdd = (suitCount << 6) / 44;
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = 24 - (mply[k].rank) + suitAdd;
  }
  else if ((tpos.length[rho_lh][lead_suit] == 0)
           && (tpos.length[rho_lh][trump] != 0) &&
           (tpos.rank_in_suit[rho_lh][trump] >
            tpos.rank_in_suit[partner_lh][trump]))
  {
    // Everybody is void, and partner can overruff.
    suitAdd = (suitCount << 6) / 44;
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = 24 - (mply[k].rank) + suitAdd;
  }
  else
  {
    for (int k = last_num_moves; k < num_moves; k++)
    {
      if (bit_map_rank[mply[k].rank] >
          tpos.rank_in_suit[partner_lh][trump])
      {
        // We can ruff, 3rd hand is void but can't overruff.
        suitAdd = (suitCount << 6) / 44;
        mply[k].weight = 24 - (mply[k].rank) + suitAdd;
      }
      else
      {
        // We're getting overruffed. Make trick costly for opponents.
        suitAdd = (suitCount << 6) / 36;
        // Don't ruff from Kx.
        if ((suitCount == 2) &&
            (tpos.second_best[suit].hand == curr_hand))
          suitAdd += -4;
        mply[k].weight = 15 - (mply[k].rank) + suitAdd;
      }
    }
  }
}
void weight_alloc_nt_void1(HeuristicContext& ctx)
{
  const Pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int curr_hand = ctx.curr_hand;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int last_num_moves = ctx.last_num_moves;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  const int partner_lh = partner[lead_hand];
  const int rho_lh = rho[lead_hand];

  // FIX:
  // Why the different penalties depending on partner?

  if (tpos.rank_in_suit[rho_lh][lead_suit] >
      (tpos.rank_in_suit[partner_lh][lead_suit] |
       bit_map_rank[ctx.lead0_rank]))
  {
    // Partner can win.
    unsigned short suitCount = tpos.length[curr_hand][suit];
    int suitAdd = (suitCount << 6) / 23;
    // Discourage pitch from Kx or A stiff.
    if (suitCount == 2 && tpos.second_best[suit].hand == curr_hand)
      suitAdd += -2;
    else if (suitCount == 1 && tpos.winner[suit].hand == curr_hand)
      suitAdd += -3;

    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else
  {
    unsigned short suitCount = tpos.length[curr_hand][suit];
    int suitAdd = (suitCount << 6) / 33;

    // Discourage pitch from Kx.
    if ((suitCount == 2) &&
        (tpos.second_best[suit].hand == curr_hand))
      suitAdd += -6;

    /* Discourage suit discard of highest card. */
    else if ((suitCount == 1) &&
             (tpos.winner[suit].hand == curr_hand))
      suitAdd += -8;

    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
}


// Helper functions for level 2+ weight allocation
int rank_forces_ace(const HeuristicContext& ctx, const int cards4th)
{
  // Figure out how high we have to play to force out the top.
  const MoveGroupType& mp = group_data[cards4th];

  int g = mp.last_group_;
  int removed = static_cast<int>(ctx.removed_ranks[ctx.lead_suit]);

  while (g >= 1 && ((mp.gap_[g] & removed) == mp.gap_[g]))
    g--;

  if (! g)
    return -1;

  // RHO's second-highest rank.
  int secondRHO = (g == 0 ? 0 : mp.rank_[g-1]);

  if (secondRHO > ctx.move1_rank)
  {
    // Try to force out the top as cheaply as possible.
    int k = 0;
    while (k < ctx.num_moves && ctx.mply[k].rank > secondRHO)
      k++;

    if (k)
      return k - 1;
  }
  else if (ctx.high1 == 1)
  {
    // Try to beat 2nd hand as cheaply as possible.
    int k = 0;
    while (k < ctx.num_moves && ctx.mply[k].rank > ctx.move1_rank)
      k++;

    if (k)
      return k - 1;
  }

  return -1;
}


// NOLINTNEXTLINE(bugprone-easily-swappable-parameters): Intentional ordering; mirrors legacy helper usage
void get_top_number(const HeuristicContext& ctx, const int ris, const int prank, int& top_number, int& mno)
{
  top_number = -10;

  // Find the lowest move that still overtakes partner's card.
  mno = 0;
  while (mno < ctx.num_moves - 1 && ctx.mply[1 + mno].rank > prank)
    mno++;

  const MoveGroupType& mp = group_data[ris];
  int g = mp.last_group_;

  // Remove partner's card as well.
  int removed = static_cast<int>(ctx.removed_ranks[ctx.lead_suit] |
                                 bit_map_rank[prank]);

  int fullseq = mp.fullseq_[g];

  while (g >= 1 && ((mp.gap_[g] & removed) == mp.gap_[g]))
    fullseq |= mp.fullseq_[--g];

  top_number = count_table[fullseq] - 1;
}
void weight_alloc_trump_notvoid2(HeuristicContext& ctx)
{
  const Pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  const int rho_lh = rho[lead_hand];
  const int cards4th = tpos.rank_in_suit[rho_lh][lead_suit];
  const int max4th = highest_rank[cards4th];
  const int min4th = lowest_rank[cards4th];
  const int max3rd = mply[0].rank;

  if (lead_suit == trump)
  {
  if (ctx.high1 == 0 && ctx.lead0_rank > max4th)
    {
      // Partner has already beat his LHO and will beat his RHO.
      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }
    else if (max3rd < min4th || max3rd < ctx.move1_rank)
    {
      // Our cards are too low to matter.
      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }
    else if (max3rd > max4th)
    {
      // We can win the trick.
      for (int k = 0; k < num_moves; k++)
      {
        if (mply[k].rank > max4th &&
            mply[k].rank > ctx.move1_rank)
          mply[k].weight = 58 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
    }
    else
    {
      // Figure out how high we have to play to force out the top.
      int kBonus = rank_forces_ace(ctx, cards4th);

      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;

      if (kBonus != -1) // Force out ace
        mply[kBonus].weight += 20;
      return;
    }
  }

  else if (ctx.move1_suit == trump)
  {
    // 2nd hand ruffs, and we must follow suit.
    for (int k = 0; k < num_moves; k++)
      mply[k].weight = -mply[k].rank;
    return;
  }

  // So now lead_suit != trump and second hand didn't ruff.
  else if (ctx.high1 == 0)
  {
    // Partner is winning so far.
    if (max4th == 0)
    {
      // 4th hand is either ruffing or not -- play low.
      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    // So 4th hand follows.
  else if (ctx.lead0_rank > max4th)
    {
      // Partner is already winning.
      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

  else if (max3rd < min4th || max3rd < ctx.move1_rank)
    {
      // Our cards are too low to matter.
      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    // So 4th hand can beat partner in the suit.
    else if (max3rd > max4th)
    {
      // We can win the trick.
      for (int k = 0; k < num_moves; k++)
      {
        if (mply[k].rank > max4th)
          mply[k].weight = 58 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
    }
    else
    {
      // We can't win the trick.
      // Figure out how high we have to play to force out the top.
      int kBonus = rank_forces_ace(ctx, cards4th);

      for (int k = 0; k < num_moves; k++)
      {
        if (mply[k].rank > ctx.move1_rank &&
            mply[k].rank > max4th) // We will win
          mply[k].weight = 60 - mply[k].rank;

        else
          mply[k].weight = -mply[k].rank;
      }

      if (kBonus != -1) // Force out ace
        mply[kBonus].weight += 20;
    }
  }
  else
  {
    // 2nd hand is winning so far. 4th hand is either ruffing
    // or not -- play high enough to beat 2nd hand.
    if (max4th == 0)
    {
      for (int k = 0; k < num_moves; k++)
      {
        if (mply[k].rank > ctx.move1_rank)
          mply[k].weight = 20 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
      return;
    }

    // Our cards are too low to matter.
  else if (max3rd < min4th || max3rd < ctx.move1_rank)
    {
      for (int k = 0; k < num_moves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    // We can win the trick.
    else if (max3rd > max4th)
    {
      for (int k = 0; k < num_moves; k++)
      {
        if (mply[k].rank > ctx.move1_rank &&
            mply[k].rank > max4th)
          mply[k].weight = 58 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
      return;
    }

    // Figure out how high we have to play to force out the top.
    int kBonus = rank_forces_ace(ctx, cards4th);

    for (int k = 0; k < num_moves; k++)
    {
      if (mply[k].rank > ctx.move1_rank &&
          mply[k].rank > max4th) // We will win
        mply[k].weight = 60 - mply[k].rank;

      else
        mply[k].weight = -mply[k].rank;
    }

    if (kBonus != -1) // Force out ace
      mply[kBonus].weight += 20;
  }
}
void weight_alloc_nt_notvoid2(HeuristicContext& ctx)
{
  // One of the main remaining issues here is cashing out long
  // suits. Examples:
  // AKJ opposite Q, overtake.
  // KQx opposite Jxxxx, don't block on the ace.
  // KJTx opposite 9 with Qx in dummy, do win the T.

  const Pos& tpos = ctx.tpos;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int curr_hand = ctx.curr_hand;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  const int rho_lh = rho[lead_hand];
  const int lho_lh = lho[lead_hand];
  const int partner_lh = partner[lead_hand];
  
  const int cards4th = tpos.rank_in_suit[rho_lh][lead_suit];
  const int max4th = highest_rank[cards4th];
  const int min4th = lowest_rank[cards4th];
  const int max3rd = mply[0].rank;

  if (ctx.high1 == 0 && ctx.lead0_rank > max4th)
  {
    // Partner has already beat his LHO and will beat his RHO.
    // Generally we play low and let partner win.
    for (int k = 0; k < num_moves; k++)
      mply[k].weight = -mply[k].rank;

    // This doesn't help much, not sure why. It does work.

    // if (0 && tpos.length[lead_hand][lead_suit] == 0 &&
    if (tpos.length[lead_hand][lead_suit] == 0 &&
        tpos.winner[lead_suit].hand == curr_hand)
    {
      // Partner has a singleton, and we have the ace.
      // Maybe we should overtake to run the suit.
      int oppLen = tpos.length[rho_lh][lead_suit] - 1;
      int lhoLen = tpos.length[lho_lh][lead_suit];
      if (lhoLen > oppLen)
        oppLen = lhoLen;

      int top_number, mno;
  get_top_number(ctx, tpos.rank_in_suit[partner_lh][lead_suit],
    ctx.lead0_rank, top_number, mno);

      if (oppLen <= top_number)
        mply[mno].weight += 20;
    }
    return;
  }
  else if (max3rd < min4th || max3rd < ctx.move1_rank)
  {
    // Our cards are too low to matter.
    for (int k = 0; k < num_moves; k++)
      mply[k].weight = -mply[k].rank;
    return;
  }

  int kBonus = -1;
  if (max4th > max3rd && max4th > ctx.move1_rank)
    kBonus = rank_forces_ace(ctx, cards4th);

  for (int k = 0; k < num_moves; k++)
  {
    if (mply[k].rank > ctx.move1_rank &&
        mply[k].rank > max4th) // We will win
      mply[k].weight = 60 - mply[k].rank;

    else
      mply[k].weight = -mply[k].rank;
  }

  if (kBonus != -1) // Force out ace
    mply[kBonus].weight += 20;
}
void weight_alloc_trump_void2(HeuristicContext& ctx)
{
  // Compared to "v2.8":
  // Moved a test for partner's win out of the k loop.

  const Pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int lead_hand = ctx.lead_hand;
  const int lead_suit = ctx.lead_suit;
  const int curr_hand = ctx.curr_hand;
  const int last_num_moves = ctx.last_num_moves;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  const int rho_lh = rho[lead_hand];
  
  int suitAdd;
  const unsigned short suitCount = tpos.length[curr_hand][suit];
  const int max4th = highest_rank[tpos.rank_in_suit[rho_lh][lead_suit]];

  if (lead_suit == trump || suit != trump)
  {
    // Discard small from a long suit.
    suitAdd = (suitCount << 6) / 40;
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
    return;
  }

  else if (ctx.high1 == 0 && ctx.lead0_rank > max4th &&
           (max4th != 0 || tpos.length[rho_lh][trump] == 0))
  {
    // Partner already beat 2nd and 4th hands.
    // Don't overruff partner's sure winner.
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank - 50;
    return;
  }

  // So now we're ruffing and partner is not already sure to win.

  for (int k = last_num_moves; k < num_moves; k++)
  {
  if (ctx.move1_suit == trump &&
    mply[k].rank < ctx.move1_rank)
    {
      // Don't underruff.
    unsigned char aggrSuit = static_cast<unsigned char>(tpos.aggr[suit]);
    unsigned char moveRank = static_cast<unsigned char>(mply[k].rank);
  unsigned char relRankValue = static_cast<unsigned char>(rel_rank[aggrSuit][moveRank]);
    int r_rank = static_cast<int>(relRankValue);
      suitAdd = (suitCount << 6) / 40;
      mply[k].weight = -32 + r_rank + suitAdd;
    }

  else if (ctx.high1 == 0)
    {
      // We ruff partner's winner over 2nd hand.
      if (max4th != 0)
      {
        if (tpos.second_best[lead_suit].hand == lead_hand)
        {
          // We'd like to know whether partner has KQ or just K,
          // but that information takes a bit of diggging. It's
          // easier just not to ruff the king.
          suitAdd = (suitCount << 6) / 50;
          mply[k].weight = 36 - mply[k].rank + suitAdd;
        }
        else
        {
          suitAdd = (suitCount << 6) / 50;
          mply[k].weight = 48 - mply[k].rank + suitAdd;
        }
      }
      else if (bit_map_rank[mply[k].rank] >
               tpos.rank_in_suit[rho_lh][trump])
      {
        // We ruff higher than 4th hand.
        suitAdd = (suitCount << 6) / 50;
        mply[k].weight = 48 - mply[k].rank + suitAdd;
      }
      else
      {
        // Force out a higher trump in 4th hand.
        suitAdd = (suitCount << 6) / 50;
        mply[k].weight = -12 - mply[k].rank + suitAdd;
      }
    }

    // 2nd hand was winning before we ruffed.
    else if (max4th != 0)
    {
      // Just ruff low.
      suitAdd = (suitCount << 6) / 50;
      mply[k].weight = 72 - mply[k].rank + suitAdd;
    }

    else if (bit_map_rank[mply[k].rank] >
             tpos.rank_in_suit[rho_lh][trump])
    {
      // Ruff higher than 4th hand can.
      suitAdd = (suitCount << 6) / 50;
      mply[k].weight = 48 - mply[k].rank + suitAdd;
    }

    else
    {
      // Force out a higher trump in 4th hand.
      suitAdd = (suitCount << 6) / 50;
      mply[k].weight = 36 - mply[k].rank + suitAdd;
    }
  }
}
void weight_alloc_nt_void2(HeuristicContext& ctx)
{
  // Compared to "v2.8":
  // Took only the second branch. The first branch (partner
  // has beat his LHO and will beat his RHO) was a bit different,
  // for no reason that I could see. This is the same or a tiny
  // bit better.

  const Pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int curr_hand = ctx.curr_hand;
  const int last_num_moves = ctx.last_num_moves;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  const unsigned short suitCount = tpos.length[curr_hand][suit];
  int suitAdd = (suitCount << 6) / 24;

  // Try not to pitch from Kx or stiff ace.
  if (suitCount == 2 && tpos.second_best[suit].hand == curr_hand)
    suitAdd -= 4;
  if (suitCount == 1 && tpos.winner[suit].hand == curr_hand)
    suitAdd -= 4;

  for (int k = last_num_moves; k < num_moves; k++)
    mply[k].weight = -(mply[k].rank) + suitAdd;
}
void weight_alloc_combined_notvoid3(HeuristicContext& ctx)
{
  // We're always following suit.
  // This function is very good, but occasionally it is better
  // to beat partner's card in order to cash out a suit in NT.

  const int trump = ctx.trump;
  const int lead_suit = ctx.lead_suit;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  if (ctx.high2 == 1 ||
    (lead_suit != trump && ctx.move2_suit == trump))
  {
    // Partner is winning the trick so far, or an opponent
    // has ruffed while we must follow. Play low.

    for (int k = 0; k < num_moves; k++)
      mply[k].weight = -mply[k].rank;
  }
  else
  {
    // We're losing so far, and either trumps were led or
    // trumps don't matter in this trick.

    for (int k = 0; k < num_moves; k++)
    {
      if (mply[k].rank > ctx.move2_rank)
        // Win as cheaply as possible.
        mply[k].weight = 30 - mply[k].rank;
      else
        mply[k].weight = -mply[k].rank;
    }
  }
}
void weight_alloc_trump_void3(HeuristicContext& ctx)
{
  // Compared to "v2.8":
  // val removed for trump plays (doesn't really matter, though).

  // To consider:
  // r_rank vs rank

  const Pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int lead_suit = ctx.lead_suit;
  const int curr_hand = ctx.curr_hand;
  const int last_num_moves = ctx.last_num_moves;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  // Don't pitch from Kx or stiff ace.
  const int mylen = tpos.length[curr_hand][suit];
  int val = (mylen << 6) / 24;
  if ((mylen == 2) && (tpos.second_best[suit].hand == curr_hand))
    val -= 2;

  if (lead_suit == trump)
  {
    // We're not following suit, so no hope.
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = -mply[k].rank + val;
  }
  else if (ctx.high2 == 1) // Partner is winning so far
  {
    if (suit == trump) // Don't ruff
      for (int k = last_num_moves; k < num_moves; k++)
        mply[k].weight = 2 - mply[k].rank + val;

    else // Discard from a long suit
      for (int k = last_num_moves; k < num_moves; k++)
        mply[k].weight = 25 - mply[k].rank + val;
  }
  else if (ctx.move2_suit == trump) // They've ruffed
  {
    if (suit == trump)
    {
      for (int k = last_num_moves; k < num_moves; k++)
      {
    int r_rank = static_cast<int>(
      static_cast<unsigned char>(
  rel_rank[static_cast<unsigned char>(tpos.aggr[suit])]
             [static_cast<unsigned char>(mply[k].rank)]));
        if (mply[k].rank > ctx.move2_rank)
          mply[k].weight = 33 + r_rank; // Overruff
        else
          mply[k].weight = -13 + r_rank; // Underruff
      }
    }
    else // We discard
      for (int k = last_num_moves; k < num_moves; k++)
        mply[k].weight = 14 - (mply[k].rank) + val;
  }
  else if (suit == trump) // We ruff and win
  {
    for (int k = last_num_moves; k < num_moves; k++)
    {
    int r_rank = static_cast<int>(
      static_cast<unsigned char>(
  rel_rank[static_cast<unsigned char>(tpos.aggr[suit])]
           [static_cast<unsigned char>(mply[k].rank)]));
      mply[k].weight = 33 + r_rank;
    }
  }
  else // We discard and lose
  {
    for (int k = last_num_moves; k < num_moves; k++)
      mply[k].weight = 14 - mply[k].rank + val;
  }
}
void weight_alloc_nt_void3(HeuristicContext& ctx)
{
  const Pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int curr_hand = ctx.curr_hand;
  const int last_num_moves = ctx.last_num_moves;
  const int num_moves = ctx.num_moves;
  MoveType* mply = ctx.mply;

  int mylen = tpos.length[curr_hand][suit];
  int val = (mylen << 6) / 27;
  // Try not to pitch from Kx, or to pitch a singleton winner.
  if ((mylen == 2) && (tpos.second_best[suit].hand == curr_hand))
    val -= 6;
  else if ((mylen == 1) && (tpos.winner[suit].hand == curr_hand))
    val -= 8;

  for (int k = last_num_moves; k < num_moves; k++)
    mply[k].weight = - mply[k].rank + val;
}
