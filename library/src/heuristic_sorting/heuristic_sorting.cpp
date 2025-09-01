#include "internal.h"

// Integration function for calling from moves.cpp
void CallHeuristic(
    const pos& tpos,
    const moveType& bestMove,
    const moveType& bestMoveTT,
    const relRanksType thrp_rel[],
    moveType* mply,
    int numMoves,
    int lastNumMoves,
    int trump,
    int suit,
    const trackType* trackp,
    int currTrick,
    int currHand,
    int leadHand,
    int leadSuit) {
  
  HeuristicContext context {
      tpos,
      bestMove,
      bestMoveTT,
      thrp_rel,
      mply,
      numMoves,
      lastNumMoves,
      trump,
      suit,
      trackp,
      currTrick,
      currHand,
      leadHand,
      leadSuit
  };
  
  SortMoves(context);
}

// Dispatcher function - routes to correct WeightAlloc function based on game state
void SortMoves(HeuristicContext& context) {
    const pos& tpos = context.tpos;
    
    // Determine which position in trick (0=leading, 1-3=following)
    int handRel = 0;
    if (context.currHand != context.leadHand) {
        // Calculate relative position: 1, 2, or 3 based on lead hand
        handRel = (context.currHand + 4 - context.leadHand) % 4;
    }
    
    // Leading hand (handRel == 0) - MoveGen0 logic
    if (handRel == 0) {
        // Check if trump game with trump winner available
        bool trumpGame = (context.trump != DDS_NOTRUMP) && 
                        (tpos.winner[context.trump].rank != 0);
        
        if (trumpGame) {
            WeightAllocTrump0(context);
        } else {
            WeightAllocNT0(context);
        }
        return;
    }
    
    // Following hands (handRel 1-3) - MoveGen123 logic
    // Check trump game condition
    int ftest = ((context.trump != DDS_NOTRUMP) &&
                 (tpos.winner[context.trump].rank != 0) ? 1 : 0);
    
    // Check if current hand can follow suit (not void)
    unsigned short ris = tpos.rankInSuit[context.currHand][context.leadSuit];
    bool canFollowSuit = (ris != 0);
    
    // Calculate function index using same logic as original
    int findex;
    if (canFollowSuit) {
        findex = 4 * handRel + ftest;
    } else {
        findex = 4 * handRel + ftest + 2;
    }
    
    // Dispatch based on findex (matching WeightList array indices)
    switch (findex) {
        case 4:  // handRel=1, NT, following suit
            WeightAllocNTNotvoid1(context);
            break;
        case 5:  // handRel=1, Trump, following suit  
            WeightAllocTrumpNotvoid1(context);
            break;
        case 6:  // handRel=1, NT, void
            WeightAllocNTVoid1(context);
            break;
        case 7:  // handRel=1, Trump, void
            WeightAllocTrumpVoid1(context);
            break;
            
        case 8:  // handRel=2, NT, following suit
            WeightAllocNTNotvoid2(context);
            break;
        case 9:  // handRel=2, Trump, following suit
            WeightAllocTrumpNotvoid2(context);
            break;
        case 10: // handRel=2, NT, void
            WeightAllocNTVoid2(context);
            break;
        case 11: // handRel=2, Trump, void
            WeightAllocTrumpVoid2(context);
            break;
            
        case 12: // handRel=3, NT, following suit
        case 13: // handRel=3, Trump, following suit (both use Combined)
            WeightAllocCombinedNotvoid3(context);
            break;
        case 14: // handRel=3, NT, void
            WeightAllocNTVoid3(context);
            break;
        case 15: // handRel=3, Trump, void
            WeightAllocTrumpVoid3(context);
            break;
            
        default:
            // Should never happen, but fall back to basic function
            if (ftest) {
                WeightAllocTrump0(context);
            } else {
                WeightAllocNT0(context);
            }
            break;
    }
}


// The following functions are extracted from Moves.cpp and refactored to be
// standalone. They now accept a HeuristicContext struct which contains all the
// necessary state that was previously accessed as members of the Moves class.

void WeightAllocTrump0(HeuristicContext& context)
{
  const unsigned short suitCount = context.tpos.length[context.leadHand][context.suit];
  const unsigned short suitCountLH = context.tpos.length[lho[context.leadHand]][context.suit];
  const unsigned short suitCountRH = context.tpos.length[rho[context.leadHand]][context.suit];
  const unsigned short aggr = context.tpos.aggr[context.suit];

  // Why?
  int countLH = (suitCountLH == 0 ? context.currTrick + 1 : suitCountLH) << 2;
  int countRH = (suitCountRH == 0 ? context.currTrick + 1 : suitCountRH) << 2;

  int suitWeightD = - (((countLH + countRH) << 5) / 13);

  for (int k = context.lastNumMoves; k < context.numMoves; k++)
  {
    int suitBonus = 0;
    bool winMove = false;

    int rRank = relRank[aggr][context.mply[k].rank];

    /* Discourage suit if LHO or RHO can ruff. */
    if ((context.suit != context.trump) &&
        (((context.tpos.rankInSuit[lho[context.leadHand]][context.suit] == 0) &&
          (context.tpos.rankInSuit[lho[context.leadHand]][context.trump] != 0)) ||
         ((context.tpos.rankInSuit[rho[context.leadHand]][context.suit] == 0) &&
          (context.tpos.rankInSuit[rho[context.leadHand]][context.trump] != 0))))
      suitBonus = -12;

    /* Encourage suit if partner can ruff. */
    if ((context.suit != context.trump) &&
        (context.tpos.length[partner[context.leadHand]][context.suit] == 0) &&
        (context.tpos.length[partner[context.leadHand]][context.trump] > 0) &&
        (suitCountRH > 0))
      suitBonus += 17;

    /* Discourage suit if RHO has high card. */
    if ((context.tpos.winner[context.suit].hand == rho[context.leadHand]) ||
        (context.tpos.secondBest[context.suit].hand == rho[context.leadHand]))
    {
      if (suitCountRH != 1)
        suitBonus += -12;
    }

    /* Try suit if LHO has winning card and partner second best.
       Exception: partner has singleton. */

    else if ((context.tpos.winner[context.suit].hand == lho[context.leadHand]) &&
             (context.tpos.secondBest[context.suit].hand == partner[context.leadHand]))
    {
      /* This case was suggested by Joel Bradmetz. */
      if (context.tpos.length[partner[context.leadHand]][context.suit] != 1)
        suitBonus += 27;
    }

    /* Encourage play of suit where partner wins and
       returns the suit for a ruff. */
    if ((context.suit != context.trump) && (suitCount == 1) &&
        (context.tpos.length[context.leadHand][context.trump] > 0) &&
        (context.tpos.length[partner[context.leadHand]][context.suit] > 1) &&
        (context.tpos.winner[context.suit].hand == partner[context.leadHand]))
      suitBonus += 19;


    /* Discourage a suit selection where the search tree appears larger
       than for the altenative suits: the search is estimated to be
       small when the added number of alternative cards to play for
       the opponents is small. */

    int suitWeightDelta = suitBonus + suitWeightD;

    if (context.tpos.winner[context.suit].rank == context.mply[k].rank)
    {
      if ((context.suit != context.trump))
      {
        if ((context.tpos.length[partner[context.leadHand]][context.suit] != 0) ||
            (context.tpos.length[partner[context.leadHand]][context.trump] == 0))
        {
          if (((context.tpos.length[lho[context.leadHand]][context.suit] != 0) ||
               (context.tpos.length[lho[context.leadHand]][context.trump] == 0)) &&
              ((context.tpos.length[rho[context.leadHand]][context.suit] != 0) ||
               (context.tpos.length[rho[context.leadHand]][context.trump] == 0)))
            winMove = true;
        }
        else if (((context.tpos.length[lho[context.leadHand]][context.suit] != 0) ||
                  (context.tpos.rankInSuit[partner[context.leadHand]][context.trump] >
                   context.tpos.rankInSuit[lho[context.leadHand]][context.trump])) &&
                 ((context.tpos.length[rho[context.leadHand]][context.suit] != 0) ||
                  (context.tpos.rankInSuit[partner[context.leadHand]][context.trump] >
                   context.tpos.rankInSuit[rho[context.leadHand]][context.trump])))
          winMove = true;
      }
      else
        winMove = true;
    }
    else if (context.tpos.rankInSuit[partner[context.leadHand]][context.suit] >
             (context.tpos.rankInSuit[lho[context.leadHand]][context.suit] |
              context.tpos.rankInSuit[rho[context.leadHand]][context.suit]))
    {
      if (context.suit != context.trump)
      {
        if (((context.tpos.length[lho[context.leadHand]][context.suit] != 0) ||
             (context.tpos.length[lho[context.leadHand]][context.trump] == 0)) &&
            ((context.tpos.length[rho[context.leadHand]][context.suit] != 0) ||
             (context.tpos.length[rho[context.leadHand]][context.trump] == 0)))
          winMove = true;
      }
      else
        winMove = true;
    }
    else if (context.suit != context.trump)
    {
      if ((context.tpos.length[partner[context.leadHand]][context.suit] == 0) &&
          (context.tpos.length[partner[context.leadHand]][context.trump] != 0))
      {
        if ((context.tpos.length[lho[context.leadHand]][context.suit] == 0) &&
            (context.tpos.length[lho[context.leadHand]][context.trump] != 0) &&
            (context.tpos.length[rho[context.leadHand]][context.suit] == 0) &&
            (context.tpos.length[rho[context.leadHand]][context.trump] != 0))
        {
          if (context.tpos.rankInSuit[partner[context.leadHand]][context.trump] >
              (context.tpos.rankInSuit[lho[context.leadHand]][context.trump] |
               context.tpos.rankInSuit[rho[context.leadHand]][context.trump]))
            winMove = true;
        }
        else if ((context.tpos.length[lho[context.leadHand]][context.suit] == 0) &&
                 (context.tpos.length[lho[context.leadHand]][context.trump] != 0))
        {
          if (context.tpos.rankInSuit[partner[context.leadHand]][context.trump]
              > context.tpos.rankInSuit[lho[context.leadHand]][context.trump])
            winMove = true;
        }
        else if ((context.tpos.length[rho[context.leadHand]][context.suit] == 0) &&
                 (context.tpos.length[rho[context.leadHand]][context.trump] != 0))
        {
          if (context.tpos.rankInSuit[partner[context.leadHand]][context.trump]
              > context.tpos.rankInSuit[rho[context.leadHand]][context.trump])
            winMove = true;
        }
        else
          winMove = true;
      }
    }

    if (winMove)
    {
      /* Encourage ruffing LHO or RHO singleton, highest card. */
      if (((suitCountLH == 1) &&
           (context.tpos.winner[context.suit].hand == lho[context.leadHand]))
          || ((suitCountRH == 1) &&
              (context.tpos.winner[context.suit].hand == rho[context.leadHand])))
        context.mply[k].weight = suitWeightDelta + 35 + rRank;

      /* Lead hand has the highest card. */

      else if (context.tpos.winner[context.suit].hand == context.leadHand)
      {
        /* Also, partner has second highest card. */
        if (context.tpos.secondBest[context.suit].hand == partner[context.leadHand])
          context.mply[k].weight = suitWeightDelta + 48 + rRank;
        else if (context.tpos.winner[context.suit].rank == context.mply[k].rank)
          /* If the current card to play is the highest card. */
          context.mply[k].weight = suitWeightDelta + 31;
        else
          context.mply[k].weight = suitWeightDelta - 3 + rRank;
      }
      else if (context.tpos.winner[context.suit].hand == partner[context.leadHand])
      {
        /* If partner has highest card */
        if (context.tpos.secondBest[context.suit].hand == context.leadHand)
          context.mply[k].weight = suitWeightDelta + 42 + rRank;
        else
          context.mply[k].weight = suitWeightDelta + 28 + rRank;
      }
      /* Encourage playing second highest rank if hand also has
         third highest rank. */
      else if ((context.mply[k].sequence) &&
               (context.mply[k].rank == context.tpos.secondBest[context.suit].rank))
        context.mply[k].weight = suitWeightDelta + 40;
      else if (context.mply[k].sequence)
        context.mply[k].weight = suitWeightDelta + 22 + rRank;
      else
        context.mply[k].weight = suitWeightDelta + 11 + rRank;

      /* playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table entry
         match. */

      if ((context.bestMove.suit == context.suit) &&
          (context.bestMove.rank == context.mply[k].rank))
        context.mply[k].weight += 55;
      else if ((context.bestMoveTT.suit == context.suit) &&
               (context.bestMoveTT.rank == context.mply[k].rank))
        context.mply[k].weight += 18;
    }
    else
    {
      /* Encourage playing the suit if the hand together with partner
         have both the 2nd highest and the 3rd highest cards such that
         the side of the hand has the highest card in the next round
         playing this suit. */

      int thirdBestHand = context.thrp_rel[aggr].absRank[3][context.suit].hand;

      if ((context.tpos.secondBest[context.suit].hand == partner[context.leadHand]) &&
          (partner[context.leadHand] == thirdBestHand))
        suitWeightDelta += 20;
      else if (((context.tpos.secondBest[context.suit].hand == context.leadHand) &&
                (partner[context.leadHand] == thirdBestHand) &&
                (context.tpos.length[partner[context.leadHand]][context.suit] > 1)) ||
               ((context.tpos.secondBest[context.suit].hand == partner[context.leadHand]) &&
                (context.leadHand == thirdBestHand) &&
                (context.tpos.length[partner[context.leadHand]][context.suit] > 1)))
        suitWeightDelta += 13;

      /* Higher weight if LHO or RHO has the highest (winning) card as
         a singleton. */

      if (((suitCountLH == 1) &&
           (context.tpos.winner[context.suit].hand == lho[context.leadHand]))
          || ((suitCountRH == 1) &&
              (context.tpos.winner[context.suit].hand == rho[context.leadHand])))
        context.mply[k].weight = suitWeightDelta + rRank + 2;
      else if (context.tpos.winner[context.suit].hand == context.leadHand)
      {
        if (context.tpos.secondBest[context.suit].hand == partner[context.leadHand])
          /* Opponents win by ruffing */
          context.mply[k].weight = suitWeightDelta + 33 + rRank;
        else if (context.tpos.winner[context.suit].rank == context.mply[k].rank)
          /* Opponents win by ruffing */
          context.mply[k].weight = suitWeightDelta + 38;
        else
          context.mply[k].weight = suitWeightDelta - 14 + rRank;
      }
      else if (context.tpos.winner[context.suit].hand == partner[context.leadHand])
      {
        /* Opponents win by ruffing */
        context.mply[k].weight = suitWeightDelta + 34 + rRank;
      }
      /* Encourage playing second highest rank if hand also has
         third highest rank. */
      else if ((context.mply[k].sequence) &&
               (context.mply[k].rank == context.tpos.secondBest[context.suit].rank))
        context.mply[k].weight = suitWeightDelta + 35;
      else
        context.mply[k].weight = suitWeightDelta + 17 - (context.mply[k].rank);

      /* Encourage playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table
         entry match. */

      if ((context.bestMove.suit == context.suit) &&
          (context.bestMove.rank == context.mply[k].rank))
        context.mply[k].weight += 18;
    }
  }
}

// Placeholder for the rest of the functions to be moved
void WeightAllocNT0(HeuristicContext& context) {
  int aggr = context.tpos.aggr[context.suit];

  /* Discourage a suit selection where the search tree appears larger
     than for the alternative suits: the search is estimated to be
     small when the added number of alternative cards to play for
     the opponents is small. */

  unsigned short suitCountLH = context.tpos.length[lho[context.leadHand]][context.suit];
  unsigned short suitCountRH = context.tpos.length[rho[context.leadHand]][context.suit];

  // Why?
  int countLH = (suitCountLH == 0 ? context.currTrick + 1 : suitCountLH) << 2;
  int countRH = (suitCountRH == 0 ? context.currTrick + 1 : suitCountRH) << 2;

  int suitWeightD = - (((countLH + countRH) << 5) / 19);
  if (context.tpos.length[partner[context.leadHand]][context.suit] == 0)
    suitWeightD += -9;

  for (int k = context.lastNumMoves; k < context.numMoves; k++)
  {
    int suitWeightDelta = suitWeightD;
    int rRank = relRank[aggr][context.mply[k].rank];

    if (context.tpos.winner[context.suit].rank == context.mply[k].rank ||
        (context.tpos.rankInSuit[partner[context.leadHand]][context.suit] >
         (context.tpos.rankInSuit[lho[context.leadHand]][context.suit] |
          context.tpos.rankInSuit[rho[context.leadHand]][context.suit])))
    {
      // Can win trick, ourselves or partner.
      // FIX: No distinction?
      /* Discourage suit if RHO has second best card.
         Exception: RHO has singleton. */
      if (context.tpos.secondBest[context.suit].hand == rho[context.leadHand])
      {
        if (suitCountRH != 1)
          suitWeightDelta += -1;
      }
      /* Encourage playing suit if LHO has second highest rank. */
      else if (context.tpos.secondBest[context.suit].hand == lho[context.leadHand])
      {
        if (suitCountLH != 1)
          suitWeightDelta += 22;
        else
          suitWeightDelta += 16;
      }

      /* Higher weight if also second best rank is present on
         current side to play, or if second best is a singleton
         at LHO or RHO. */

      if (((context.tpos.secondBest[context.suit].hand != lho[context.leadHand])
           || (suitCountLH == 1)) &&
          ((context.tpos.secondBest[context.suit].hand != rho[context.leadHand])
           || (suitCountRH == 1)))
        context.mply[k].weight = suitWeightDelta + 45 + rRank;
      else
        context.mply[k].weight = suitWeightDelta + 18 + rRank;

      /* Encourage playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table
         entry match. */

      if ((context.bestMove.suit == context.suit) &&
          (context.bestMove.rank == context.mply[k].rank))
        context.mply[k].weight += 126;
      else if ((context.bestMoveTT.suit == context.suit) &&
               (context.bestMoveTT.rank == context.mply[k].rank))
        context.mply[k].weight += 32;
    }
    else
    {
      /* Discourage suit if RHO has winning or second best card.
         Exception: RHO has singleton. */

      if ((context.tpos.winner[context.suit].hand == rho[context.leadHand]) ||
          (context.tpos.secondBest[context.suit].hand == rho[context.leadHand]))
      {
        if (suitCountRH != 1)
          suitWeightDelta += -10;
      }

      /* Try suit if LHO has winning card and partner second best.
         Exception: partner has singleton. */

      else if ((context.tpos.winner[context.suit].hand == lho[context.leadHand]) &&
               (context.tpos.secondBest[context.suit].hand == partner[context.leadHand]))
      {
        /* This case was suggested by Joel Bradmetz. */
        if (context.tpos.length[partner[context.leadHand]][context.suit] != 1)
          suitWeightDelta += 31;
      }

      /* Encourage playing the suit if the hand together with partner
         have both the 2nd highest and the 3rd highest cards such
         that the side of the hand has the highest card in the
         next round playing this suit. */

      int thirdBestHand = context.thrp_rel[aggr].absRank[3][context.suit].hand;

      if ((context.tpos.secondBest[context.suit].hand == partner[context.leadHand]) &&
          (partner[context.leadHand] == thirdBestHand))
        suitWeightDelta += 35;
      else if (((context.tpos.secondBest[context.suit].hand == context.leadHand) &&
                (partner[context.leadHand] == thirdBestHand) &&
                (context.tpos.length[partner[context.leadHand]][context.suit] > 1)) ||
               ((context.tpos.secondBest[context.suit].hand == partner[context.leadHand]) &&
                (context.leadHand == thirdBestHand) &&
                (context.tpos.length[partner[context.leadHand]][context.suit] > 1)))
        suitWeightDelta += 25;

      /* Higher weight if LHO or RHO has the highest (winning) card
         as a singleton. */

      if (((suitCountLH == 1) &&
           (context.tpos.winner[context.suit].hand == lho[context.leadHand]))
          || ((suitCountRH == 1) &&
              (context.tpos.winner[context.suit].hand == rho[context.leadHand])))
        context.mply[k].weight = suitWeightDelta + 28 + rRank;
      else if (context.tpos.winner[context.suit].hand == context.leadHand)
        context.mply[k].weight = suitWeightDelta - 17 + rRank;
      else if (! context.mply[k].sequence)
        context.mply[k].weight = suitWeightDelta + 12 + rRank;
      else if (context.mply[k].rank == context.tpos.secondBest[context.suit].rank)
        context.mply[k].weight = suitWeightDelta + 48;
      else
        context.mply[k].weight = suitWeightDelta + 29 - rRank;

      /* Encourage playing cards that previously caused search cutoff
         or was stored as the best move in a transposition table
         entry match. */

      if ((context.bestMove.suit == context.suit) && (context.bestMove.rank == context.mply[k].rank))
        context.mply[k].weight += 47;
      else if ((context.bestMoveTT.suit == context.suit) &&
               (context.bestMoveTT.rank == context.mply[k].rank))
        context.mply[k].weight += 19;
    }
  }
}
void WeightAllocTrumpNotvoid1(HeuristicContext& ctx)
{
  const pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int currHand = ctx.currHand;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int partner_lh = partner[leadHand];
  const int rho_lh = rho[leadHand];
  
  unsigned short suitCount = tpos.length[currHand][suit];
  int suitAdd;

  if (suit == trump)
  {
    // We trump a non-trump card.
    
    if (tpos.length[partner_lh][leadSuit] != 0)
    {
      // 3rd hand will follow.
      if ((tpos.rankInSuit[rho_lh][leadSuit] >
           (tpos.rankInSuit[partner_lh][leadSuit] |
            bitMapRank[trackp->move[0].rank])) ||
          ((tpos.length[rho_lh][leadSuit] == 0) &&
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
            (tpos.secondBest[suit].hand == currHand))
          suitAdd += -4;
      }
    }
    else if ((tpos.length[rho_lh][leadSuit] == 0) &&
             (tpos.rankInSuit[rho_lh][trump] >
              tpos.rankInSuit[partner_lh][trump]))
    {
      // Partner can overruff 3rd hand.
      suitAdd = 60 + (suitCount << 6) / 44;
    }
    else if ((tpos.length[partner_lh][trump] == 0) &&
             (tpos.rankInSuit[rho_lh][leadSuit] >
              bitMapRank[trackp->move[0].rank]))
    {
      // 3rd hand has no trumps, and partner has suit winner.
      suitAdd = 60 + (suitCount << 6) / 44;
    }
    else
    {
      suitAdd = -2 + (suitCount << 6) / 36;
      // Don't ruff from Kx.
      if ((suitCount == 2) &&
          (tpos.secondBest[suit].hand == currHand))
        suitAdd += -4;
    }

    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else if (suit != trump)
  {
    // We discard on a side suit.

    if (tpos.length[partner_lh][leadSuit] != 0)
    {
      // 3rd hand will follow.
      if (tpos.rankInSuit[rho_lh][leadSuit] >
          (tpos.rankInSuit[partner_lh][leadSuit] |
           bitMapRank[trackp->move[0].rank]))
        // Partner has winning card.
        suitAdd = 60 + (suitCount << 6) / 44;
      else if ((tpos.length[rho_lh][leadSuit] == 0)
               && (tpos.length[rho_lh][trump] != 0))
        // Partner can ruff.
        suitAdd = 60 + (suitCount << 6) / 44;
      else
      {
        // FIX: No reason to differentiate here?
        suitAdd = -2 + (suitCount << 6) / 36;
        // Don't pitch from Kx.
        if ((suitCount == 2) &&
            (tpos.secondBest[suit].hand == currHand))
          suitAdd += -4;
      }
    }
    else if ((tpos.length[rho_lh][leadSuit] == 0)
             && (tpos.rankInSuit[rho_lh][trump] >
                 tpos.rankInSuit[partner_lh][trump]))
      // Partner can overruff 3rd hand.
      suitAdd = 60 + (suitCount << 6) / 44;
    else if ((tpos.length[partner_lh][trump] == 0)
             && (tpos.rankInSuit[rho_lh][leadSuit] >
                 bitMapRank[trackp->move[0].rank]))
      // 3rd hand has no trumps, and partner has suit winner.
      suitAdd = 60 + (suitCount << 6) / 44;
    else
    {
      // FIX: No reason to differentiate here?
      suitAdd = -2 + (suitCount << 6) / 36;
      // Don't pitch from Kx.
      if ((suitCount == 2) &&
          (tpos.secondBest[suit].hand == currHand))
        suitAdd += -4;
    }
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else if (tpos.length[partner_lh][leadSuit] != 0)
  {
    // 3rd hand follows suit while we ruff.
    // Could be ruffing partner's winner!
    suitAdd = (suitCount << 6) / 44;
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = 24 - (mply[k].rank) + suitAdd;
  }
  else if ((tpos.length[rho_lh][leadSuit] == 0)
           && (tpos.length[rho_lh][trump] != 0) &&
           (tpos.rankInSuit[rho_lh][trump] >
            tpos.rankInSuit[partner_lh][trump]))
  {
    // Everybody is void, and partner can overruff.
    suitAdd = (suitCount << 6) / 44;
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = 24 - (mply[k].rank) + suitAdd;
  }
  else
  {
    for (int k = lastNumMoves; k < numMoves; k++)
    {
      if (bitMapRank[mply[k].rank] >
          tpos.rankInSuit[partner_lh][trump])
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
            (tpos.secondBest[suit].hand == currHand))
          suitAdd += -4;
        mply[k].weight = 15 - (mply[k].rank) + suitAdd;
      }
    }
  }
}
void WeightAllocNTNotvoid1(HeuristicContext& ctx)
{
  const pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int currHand = ctx.currHand;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int partner_lh = partner[leadHand];
  const int rho_lh = rho[leadHand];

  // FIX:
  // Why the different penalties depending on partner?

  if (tpos.rankInSuit[rho_lh][leadSuit] >
      (tpos.rankInSuit[partner_lh][leadSuit] |
       bitMapRank[trackp->move[0].rank]))
  {
    // Partner can win.
    unsigned short suitCount = tpos.length[currHand][suit];
    int suitAdd = (suitCount << 6) / 23;
    // Discourage pitch from Kx or A stiff.
    if (suitCount == 2 && tpos.secondBest[suit].hand == currHand)
      suitAdd += -2;
    else if (suitCount == 1 && tpos.winner[suit].hand == currHand)
      suitAdd += -3;

    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else
  {
    unsigned short suitCount = tpos.length[currHand][suit];
    int suitAdd = (suitCount << 6) / 33;

    // Discourage pitch from Kx.
    if ((suitCount == 2) &&
        (tpos.secondBest[suit].hand == currHand))
      suitAdd += -6;

    /* Discourage suit discard of highest card. */
    else if ((suitCount == 1) &&
             (tpos.winner[suit].hand == currHand))
      suitAdd += -8;

    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
}
void WeightAllocTrumpVoid1(HeuristicContext& ctx)
{
  const pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int currHand = ctx.currHand;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int partner_lh = partner[leadHand];
  const int rho_lh = rho[leadHand];
  
  unsigned short suitCount = tpos.length[currHand][suit];
  int suitAdd;

  if (suit == trump)
  {
    // We trump a non-trump card.
    
    if (tpos.length[partner_lh][leadSuit] != 0)
    {
      // 3rd hand will follow.
      if ((tpos.rankInSuit[rho_lh][leadSuit] >
           (tpos.rankInSuit[partner_lh][leadSuit] |
            bitMapRank[trackp->move[0].rank])) ||
          ((tpos.length[rho_lh][leadSuit] == 0) &&
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
            (tpos.secondBest[suit].hand == currHand))
          suitAdd += -4;
      }
    }
    else if ((tpos.length[rho_lh][leadSuit] == 0) &&
             (tpos.rankInSuit[rho_lh][trump] >
              tpos.rankInSuit[partner_lh][trump]))
    {
      // Partner can overruff 3rd hand.
      suitAdd = 60 + (suitCount << 6) / 44;
    }
    else if ((tpos.length[partner_lh][trump] == 0) &&
             (tpos.rankInSuit[rho_lh][leadSuit] >
              bitMapRank[trackp->move[0].rank]))
    {
      // 3rd hand has no trumps, and partner has suit winner.
      suitAdd = 60 + (suitCount << 6) / 44;
    }
    else
    {
      suitAdd = -2 + (suitCount << 6) / 36;
      // Don't ruff from Kx.
      if ((suitCount == 2) &&
          (tpos.secondBest[suit].hand == currHand))
        suitAdd += -4;
    }

    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else if (suit != trump)
  {
    // We discard on a side suit.

    if (tpos.length[partner_lh][leadSuit] != 0)
    {
      // 3rd hand will follow.
      if (tpos.rankInSuit[rho_lh][leadSuit] >
          (tpos.rankInSuit[partner_lh][leadSuit] |
           bitMapRank[trackp->move[0].rank]))
        // Partner has winning card.
        suitAdd = 60 + (suitCount << 6) / 44;
      else if ((tpos.length[rho_lh][leadSuit] == 0)
               && (tpos.length[rho_lh][trump] != 0))
        // Partner can ruff.
        suitAdd = 60 + (suitCount << 6) / 44;
      else
      {
        // FIX: No reason to differentiate here?
        suitAdd = -2 + (suitCount << 6) / 36;
        // Don't pitch from Kx.
        if ((suitCount == 2) &&
            (tpos.secondBest[suit].hand == currHand))
          suitAdd += -4;
      }
    }
    else if ((tpos.length[rho_lh][leadSuit] == 0)
             && (tpos.rankInSuit[rho_lh][trump] >
                 tpos.rankInSuit[partner_lh][trump]))
      // Partner can overruff 3rd hand.
      suitAdd = 60 + (suitCount << 6) / 44;
    else if ((tpos.length[partner_lh][trump] == 0)
             && (tpos.rankInSuit[rho_lh][leadSuit] >
                 bitMapRank[trackp->move[0].rank]))
      // 3rd hand has no trumps, and partner has suit winner.
      suitAdd = 60 + (suitCount << 6) / 44;
    else
    {
      // FIX: No reason to differentiate here?
      suitAdd = -2 + (suitCount << 6) / 36;
      // Don't pitch from Kx.
      if ((suitCount == 2) &&
          (tpos.secondBest[suit].hand == currHand))
        suitAdd += -4;
    }
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else if (tpos.length[partner_lh][leadSuit] != 0)
  {
    // 3rd hand follows suit while we ruff.
    // Could be ruffing partner's winner!
    suitAdd = (suitCount << 6) / 44;
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = 24 - (mply[k].rank) + suitAdd;
  }
  else if ((tpos.length[rho_lh][leadSuit] == 0)
           && (tpos.length[rho_lh][trump] != 0) &&
           (tpos.rankInSuit[rho_lh][trump] >
            tpos.rankInSuit[partner_lh][trump]))
  {
    // Everybody is void, and partner can overruff.
    suitAdd = (suitCount << 6) / 44;
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = 24 - (mply[k].rank) + suitAdd;
  }
  else
  {
    for (int k = lastNumMoves; k < numMoves; k++)
    {
      if (bitMapRank[mply[k].rank] >
          tpos.rankInSuit[partner_lh][trump])
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
            (tpos.secondBest[suit].hand == currHand))
          suitAdd += -4;
        mply[k].weight = 15 - (mply[k].rank) + suitAdd;
      }
    }
  }
}
void WeightAllocNTVoid1(HeuristicContext& ctx)
{
  const pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int currHand = ctx.currHand;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int partner_lh = partner[leadHand];
  const int rho_lh = rho[leadHand];

  // FIX:
  // Why the different penalties depending on partner?

  if (tpos.rankInSuit[rho_lh][leadSuit] >
      (tpos.rankInSuit[partner_lh][leadSuit] |
       bitMapRank[trackp->move[0].rank]))
  {
    // Partner can win.
    unsigned short suitCount = tpos.length[currHand][suit];
    int suitAdd = (suitCount << 6) / 23;
    // Discourage pitch from Kx or A stiff.
    if (suitCount == 2 && tpos.secondBest[suit].hand == currHand)
      suitAdd += -2;
    else if (suitCount == 1 && tpos.winner[suit].hand == currHand)
      suitAdd += -3;

    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
  else
  {
    unsigned short suitCount = tpos.length[currHand][suit];
    int suitAdd = (suitCount << 6) / 33;

    // Discourage pitch from Kx.
    if ((suitCount == 2) &&
        (tpos.secondBest[suit].hand == currHand))
      suitAdd += -6;

    /* Discourage suit discard of highest card. */
    else if ((suitCount == 1) &&
             (tpos.winner[suit].hand == currHand))
      suitAdd += -8;

    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
  }
}


// Helper functions for level 2+ weight allocation
int RankForcesAce(const HeuristicContext& ctx, const int cards4th)
{
  // Figure out how high we have to play to force out the top.
  const moveGroupType& mp = groupData[cards4th];

  int g = mp.lastGroup;
  int removed = static_cast<int>(ctx.trackp->removedRanks[ctx.leadSuit]);

  while (g >= 1 && ((mp.gap[g] & removed) == mp.gap[g]))
    g--;

  if (! g)
    return -1;

  // RHO's second-highest rank.
  int secondRHO = (g == 0 ? 0 : mp.rank[g-1]);

  if (secondRHO > ctx.trackp->move[1].rank)
  {
    // Try to force out the top as cheaply as possible.
    int k = 0;
    while (k < ctx.numMoves && ctx.mply[k].rank > secondRHO)
      k++;

    if (k)
      return k - 1;
  }
  else if (ctx.trackp->high[1] == 1)
  {
    // Try to beat 2nd hand as cheaply as possible.
    int k = 0;
    while (k < ctx.numMoves && ctx.mply[k].rank > ctx.trackp->move[1].rank)
      k++;

    if (k)
      return k - 1;
  }

  return -1;
}


void GetTopNumber(const HeuristicContext& ctx, const int ris, const int prank, int& topNumber, int& mno)
{
  topNumber = -10;

  // Find the lowest move that still overtakes partner's card.
  mno = 0;
  while (mno < ctx.numMoves - 1 && ctx.mply[1 + mno].rank > prank)
    mno++;

  const moveGroupType& mp = groupData[ris];
  int g = mp.lastGroup;

  // Remove partner's card as well.
  int removed = static_cast<int>(ctx.trackp->removedRanks[ctx.leadSuit] |
                                 bitMapRank[prank]);

  int fullseq = mp.fullseq[g];

  while (g >= 1 && ((mp.gap[g] & removed) == mp.gap[g]))
    fullseq |= mp.fullseq[--g];

  topNumber = counttable[fullseq] - 1;
}
void WeightAllocTrumpNotvoid2(HeuristicContext& ctx)
{
  const pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int rho_lh = rho[leadHand];
  const int cards4th = tpos.rankInSuit[rho_lh][leadSuit];
  const int max4th = highestRank[cards4th];
  const int min4th = lowestRank[cards4th];
  const int max3rd = mply[0].rank;

  if (leadSuit == trump)
  {
    if (trackp->high[1] == 0 && trackp->move[0].rank > max4th)
    {
      // Partner has already beat his LHO and will beat his RHO.
      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }
    else if (max3rd < min4th || max3rd < trackp->move[1].rank)
    {
      // Our cards are too low to matter.
      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }
    else if (max3rd > max4th)
    {
      // We can win the trick.
      for (int k = 0; k < numMoves; k++)
      {
        if (mply[k].rank > max4th &&
            mply[k].rank > trackp->move[1].rank)
          mply[k].weight = 58 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
    }
    else
    {
      // Figure out how high we have to play to force out the top.
      int kBonus = RankForcesAce(ctx, cards4th);

      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;

      if (kBonus != -1) // Force out ace
        mply[kBonus].weight += 20;
      return;
    }
  }

  else if (trackp->move[1].suit == trump)
  {
    // 2nd hand ruffs, and we must follow suit.
    for (int k = 0; k < numMoves; k++)
      mply[k].weight = -mply[k].rank;
    return;
  }

  // So now leadSuit != trump and second hand didn't ruff.
  else if (trackp->high[1] == 0)
  {
    // Partner is winning so far.
    if (max4th == 0)
    {
      // 4th hand is either ruffing or not -- play low.
      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    // So 4th hand follows.
    else if (trackp->move[0].rank > max4th)
    {
      // Partner is already winning.
      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    else if (max3rd < min4th || max3rd < trackp->move[1].rank)
    {
      // Our cards are too low to matter.
      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    // So 4th hand can beat partner in the suit.
    else if (max3rd > max4th)
    {
      // We can win the trick.
      for (int k = 0; k < numMoves; k++)
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
      int kBonus = RankForcesAce(ctx, cards4th);

      for (int k = 0; k < numMoves; k++)
      {
        if (mply[k].rank > trackp->move[1].rank &&
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
      for (int k = 0; k < numMoves; k++)
      {
        if (mply[k].rank > trackp->move[1].rank)
          mply[k].weight = 20 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
      return;
    }

    // Our cards are too low to matter.
    else if (max3rd < min4th || max3rd < trackp->move[1].rank)
    {
      for (int k = 0; k < numMoves; k++)
        mply[k].weight = -mply[k].rank;
      return;
    }

    // We can win the trick.
    else if (max3rd > max4th)
    {
      for (int k = 0; k < numMoves; k++)
      {
        if (mply[k].rank > trackp->move[1].rank &&
            mply[k].rank > max4th)
          mply[k].weight = 58 - mply[k].rank;
        else
          mply[k].weight = -mply[k].rank;
      }
      return;
    }

    // Figure out how high we have to play to force out the top.
    int kBonus = RankForcesAce(ctx, cards4th);

    for (int k = 0; k < numMoves; k++)
    {
      if (mply[k].rank > trackp->move[1].rank &&
          mply[k].rank > max4th) // We will win
        mply[k].weight = 60 - mply[k].rank;

      else
        mply[k].weight = -mply[k].rank;
    }

    if (kBonus != -1) // Force out ace
      mply[kBonus].weight += 20;
  }
}
void WeightAllocNTNotvoid2(HeuristicContext& ctx)
{
  // One of the main remaining issues here is cashing out long
  // suits. Examples:
  // AKJ opposite Q, overtake.
  // KQx opposite Jxxxx, don't block on the ace.
  // KJTx opposite 9 with Qx in dummy, do win the T.

  const pos& tpos = ctx.tpos;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int currHand = ctx.currHand;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int rho_lh = rho[leadHand];
  const int lho_lh = lho[leadHand];
  const int partner_lh = partner[leadHand];
  
  const int cards4th = tpos.rankInSuit[rho_lh][leadSuit];
  const int max4th = highestRank[cards4th];
  const int min4th = lowestRank[cards4th];
  const int max3rd = mply[0].rank;

  if (trackp->high[1] == 0 && trackp->move[0].rank > max4th)
  {
    // Partner has already beat his LHO and will beat his RHO.
    // Generally we play low and let partner win.
    for (int k = 0; k < numMoves; k++)
      mply[k].weight = -mply[k].rank;

    // This doesn't help much, not sure why. It does work.

    // if (0 && tpos.length[leadHand][leadSuit] == 0 &&
    if (tpos.length[leadHand][leadSuit] == 0 &&
        tpos.winner[leadSuit].hand == currHand)
    {
      // Partner has a singleton, and we have the ace.
      // Maybe we should overtake to run the suit.
      int oppLen = tpos.length[rho_lh][leadSuit] - 1;
      int lhoLen = tpos.length[lho_lh][leadSuit];
      if (lhoLen > oppLen)
        oppLen = lhoLen;

      int topNumber, mno;
      GetTopNumber(ctx, tpos.rankInSuit[partner_lh][leadSuit],
                   trackp->move[0].rank, topNumber, mno);

      if (oppLen <= topNumber)
        mply[mno].weight += 20;
    }
    return;
  }
  else if (max3rd < min4th || max3rd < trackp->move[1].rank)
  {
    // Our cards are too low to matter.
    for (int k = 0; k < numMoves; k++)
      mply[k].weight = -mply[k].rank;
    return;
  }

  int kBonus = -1;
  if (max4th > max3rd && max4th > trackp->move[1].rank)
    kBonus = RankForcesAce(ctx, cards4th);

  for (int k = 0; k < numMoves; k++)
  {
    if (mply[k].rank > trackp->move[1].rank &&
        mply[k].rank > max4th) // We will win
      mply[k].weight = 60 - mply[k].rank;

    else
      mply[k].weight = -mply[k].rank;
  }

  if (kBonus != -1) // Force out ace
    mply[kBonus].weight += 20;
}
void WeightAllocTrumpVoid2(HeuristicContext& ctx)
{
  // Compared to "v2.8":
  // Moved a test for partner's win out of the k loop.

  const pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int leadHand = ctx.leadHand;
  const int leadSuit = ctx.leadSuit;
  const int currHand = ctx.currHand;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  const int rho_lh = rho[leadHand];
  
  int suitAdd;
  const unsigned short suitCount = tpos.length[currHand][suit];
  const int max4th = highestRank[tpos.rankInSuit[rho_lh][leadSuit]];

  if (leadSuit == trump || suit != trump)
  {
    // Discard small from a long suit.
    suitAdd = (suitCount << 6) / 40;
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + suitAdd;
    return;
  }

  else if (trackp->high[1] == 0 && trackp->move[0].rank > max4th &&
           (max4th != 0 || tpos.length[rho_lh][trump] == 0))
  {
    // Partner already beat 2nd and 4th hands.
    // Don't overruff partner's sure winner.
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank - 50;
    return;
  }

  // So now we're ruffing and partner is not already sure to win.

  for (int k = lastNumMoves; k < numMoves; k++)
  {
    if (trackp->move[1].suit == trump &&
        mply[k].rank < trackp->move[1].rank)
    {
      // Don't underruff.
      int rRank = relRank[tpos.aggr[suit]][mply[k].rank];
      suitAdd = (suitCount << 6) / 40;
      mply[k].weight = -32 + rRank + suitAdd;
    }

    else if (trackp->high[1] == 0)
    {
      // We ruff partner's winner over 2nd hand.
      if (max4th != 0)
      {
        if (tpos.secondBest[leadSuit].hand == leadHand)
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
      else if (bitMapRank[mply[k].rank] >
               tpos.rankInSuit[rho_lh][trump])
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

    else if (bitMapRank[mply[k].rank] >
             tpos.rankInSuit[rho_lh][trump])
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
void WeightAllocNTVoid2(HeuristicContext& ctx)
{
  // Compared to "v2.8":
  // Took only the second branch. The first branch (partner
  // has beat his LHO and will beat his RHO) was a bit different,
  // for no reason that I could see. This is the same or a tiny
  // bit better.

  const pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int currHand = ctx.currHand;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;

  const unsigned short suitCount = tpos.length[currHand][suit];
  int suitAdd = (suitCount << 6) / 24;

  // Try not to pitch from Kx or stiff ace.
  if (suitCount == 2 && tpos.secondBest[suit].hand == currHand)
    suitAdd -= 4;
  else if (suitCount == 1 && tpos.winner[suit].hand == currHand)
    suitAdd -= 4;

  for (int k = lastNumMoves; k < numMoves; k++)
    mply[k].weight = -(mply[k].rank) + suitAdd;
}
void WeightAllocCombinedNotvoid3(HeuristicContext& ctx)
{
  // We're always following suit.
  // This function is very good, but occasionally it is better
  // to beat partner's card in order to cash out a suit in NT.

  const int trump = ctx.trump;
  const int leadSuit = ctx.leadSuit;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  if (trackp->high[2] == 1 ||
      (leadSuit != trump && trackp->move[2].suit == trump))
  {
    // Partner is winning the trick so far, or an opponent
    // has ruffed while we must follow. Play low.

    for (int k = 0; k < numMoves; k++)
      mply[k].weight = -mply[k].rank;
  }
  else
  {
    // We're losing so far, and either trumps were led or
    // trumps don't matter in this trick.

    for (int k = 0; k < numMoves; k++)
    {
      if (mply[k].rank > trackp->move[2].rank)
        // Win as cheaply as possible.
        mply[k].weight = 30 - mply[k].rank;
      else
        mply[k].weight = -mply[k].rank;
    }
  }
}
void WeightAllocTrumpVoid3(HeuristicContext& ctx)
{
  // Compared to "v2.8":
  // val removed for trump plays (doesn't really matter, though).

  // To consider:
  // rRank vs rank

  const pos& tpos = ctx.tpos;
  const int trump = ctx.trump;
  const int suit = ctx.suit;
  const int leadSuit = ctx.leadSuit;
  const int currHand = ctx.currHand;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;
  const trackType* trackp = ctx.trackp;

  // Don't pitch from Kx or stiff ace.
  const int mylen = tpos.length[currHand][suit];
  int val = (mylen << 6) / 24;
  if ((mylen == 2) && (tpos.secondBest[suit].hand == currHand))
    val -= 2;

  if (leadSuit == trump)
  {
    // We're not following suit, so no hope.
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = -mply[k].rank + val;
  }
  else if (trackp->high[2] == 1) // Partner is winning so far
  {
    if (suit == trump) // Don't ruff
      for (int k = lastNumMoves; k < numMoves; k++)
        mply[k].weight = 2 - mply[k].rank + val;

    else // Discard from a long suit
      for (int k = lastNumMoves; k < numMoves; k++)
        mply[k].weight = 25 - mply[k].rank + val;
  }
  else if (trackp->move[2].suit == trump) // They've ruffed
  {
    if (suit == trump)
    {
      for (int k = lastNumMoves; k < numMoves; k++)
      {
        int rRank = relRank[tpos.aggr[suit]][mply[k].rank];
        if (mply[k].rank > trackp->move[2].rank)
          mply[k].weight = 33 + rRank; // Overruff
        else
          mply[k].weight = -13 + rRank; // Underruff
      }
    }
    else // We discard
      for (int k = lastNumMoves; k < numMoves; k++)
        mply[k].weight = 14 - (mply[k].rank) + val;
  }
  else if (suit == trump) // We ruff and win
  {
    for (int k = lastNumMoves; k < numMoves; k++)
    {
      int rRank = relRank[tpos.aggr[suit]][mply[k].rank];
      mply[k].weight = 33 + rRank;
    }
  }
  else // We discard and lose
  {
    for (int k = lastNumMoves; k < numMoves; k++)
      mply[k].weight = 14 - mply[k].rank + val;
  }
}
void WeightAllocNTVoid3(HeuristicContext& ctx)
{
  const pos& tpos = ctx.tpos;
  const int suit = ctx.suit;
  const int currHand = ctx.currHand;
  const int lastNumMoves = ctx.lastNumMoves;
  const int numMoves = ctx.numMoves;
  moveType* mply = ctx.mply;

  int mylen = tpos.length[currHand][suit];
  int val = (mylen << 6) / 27;
  // Try not to pitch from Kx, or to pitch a singleton winner.
  if ((mylen == 2) && (tpos.secondBest[suit].hand == currHand))
    val -= 6;
  else if ((mylen == 1) && (tpos.winner[suit].hand == currHand))
    val -= 8;

  for (int k = lastNumMoves; k < numMoves; k++)
    mply[k].weight = - mply[k].rank + val;
}
