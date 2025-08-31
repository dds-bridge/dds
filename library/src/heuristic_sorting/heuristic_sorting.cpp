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

// Dispatcher function
void SortMoves(HeuristicContext& context) {
    // This is a simplified dispatcher. The actual logic to choose the
    // correct WeightAlloc function will be more complex and based on the
    // logic in Moves::MoveGen0 and Moves::MoveGen123.
    // For now, we'll just call one of them as a placeholder.
    WeightAllocTrump0(context);
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
void WeightAllocTrumpNotvoid2(HeuristicContext& context) {}
void WeightAllocNTNotvoid2(HeuristicContext& context) {}
void WeightAllocTrumpVoid2(HeuristicContext& context) {}
void WeightAllocNTVoid2(HeuristicContext& context) {}
void WeightAllocCombinedNotvoid3(HeuristicContext& context) {}
void WeightAllocTrumpVoid3(HeuristicContext& context) {}
void WeightAllocNTVoid3(HeuristicContext& context) {}
