/* 
   DDS 2.7.0   A bridge double dummy solver.
   Copyright (C) 2006-2014 by Bo Haglund   
   Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.
   The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd for ACBLmerge. He has kindly given
   permission to include a C++ adaptation in DDS.
   						
   The PlayAnalyser analyses the played cards of the deal and presents 
   their double dummy values. The par calculation function DealerPar 
   provides an alternative way of calculating and presenting par 
   results.  Both these functions have been written by Soren Hein.
   He has also made numerous contributions to the code, especially in 
   the initialization part.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
   http://www.apache.org/licenses/LICENSE-2.0
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or 
   implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/


#include "dds.h"
#include "Init.h"
#include "Moves.h"
#include "ABsearch.h"


int CheckDeal(
  struct gameInfo	* gamep);

int DumpInput(
  int			errCode,
  struct deal		dl,
  int			target,
  int			solutions,
  int			mode);

void PrintDeal(
  FILE 			* fp, 
  unsigned short 	ranks[][DDS_SUITS]);

int InvBitMapRank(
  unsigned short	bitMap);


extern int noOfThreads;  /* The number of entries to the transposition tables. There is
				    one entry per thread. */



bool (* AB_ptr_list[DDS_HANDS])( 
  struct pos 		* posPoint, 
  int 			target, 
  int 			depth, 
  struct localVarType	* thrp)
  = { ABsearch, ABsearch1, ABsearch2, ABsearch3 };


int STDCALL SolveBoard(
  struct deal 		dl, 
  int 			target,
  int 			solutions, 
  int 			mode, 
  struct futureTricks 	* futp, 
  int 			thrId)
{
  int noStartMoves, noMoves, cardCount, totalTricks, handRelFirst;
  int noOfCardsPerHand[DDS_HANDS];
  int latestTrickSuit[DDS_HANDS];
  int latestTrickRank[DDS_HANDS];
  int maxHand = 0, maxSuit = 0, maxRank;
  int first, last;
  struct movePlyType temp;
  struct moveType mv;
  struct localVarType * thrp = &localVar[thrId];

// printf("SolveBoard\n");
// fflush(stdout);

  if ((thrId < 0) || (thrId >= noOfThreads))  	
  /* Fault corrected after suggestion by Dirk Willecke. */
  {
    DumpInput(RETURN_THREAD_INDEX, dl, target, solutions, mode);
    return RETURN_THREAD_INDEX;
  }

  for (int k = 0; k <= 13; k++)
  {
    thrp->forbiddenMoves[k].rank = 0;
    thrp->forbiddenMoves[k].suit = 0;
  }

  if (target < -1)
  {
    DumpInput(RETURN_TARGET_WRONG_LO, dl, target, solutions, mode);
    return RETURN_TARGET_WRONG_LO;
  }

  if (target > 13)
  {
    DumpInput(RETURN_TARGET_WRONG_HI, dl, target, solutions, mode);
    return RETURN_TARGET_WRONG_HI;
  }

  if (solutions < 1)
  {
    DumpInput(RETURN_SOLNS_WRONG_LO, dl, target, solutions, mode);
    return RETURN_SOLNS_WRONG_LO;
  }

  if (solutions > 3)
  {
    DumpInput(RETURN_SOLNS_WRONG_HI, dl, target, solutions, mode);
    return RETURN_SOLNS_WRONG_HI;
  }

  for (int k = 0; k < DDS_HANDS; k++)
    noOfCardsPerHand[handId(dl.first, k)] = 0;

  for (int k = 0; k <= 2; k++)
  {
    if (dl.currentTrickRank[k] != 0)
    {
      noOfCardsPerHand[handId(dl.first, k)] = 1;
      unsigned short int aggrRemain = 0;
      for (int h = 0; h < DDS_HANDS; h++)
        aggrRemain |= (dl.remainCards[h][dl.currentTrickSuit[k]] >> 2);
      if ((aggrRemain & bitMapRank[dl.currentTrickRank[k]]) != 0)
      {
        DumpInput(RETURN_PLAYED_CARD, dl, target, solutions, mode);
        return RETURN_PLAYED_CARD;
      }
    }
  }

  if (target == -1)
    thrp->tricksTarget = 99;
  else
    thrp->tricksTarget = target;

  thrp->newDeal  = false;
  thrp->newTrump = false;
  thrp->diffDeal = 0;
  thrp->aggDeal  = 0;
  cardCount      = 0;

  for (int h = 0; h < DDS_HANDS; h++)
  {
    for (int s = 0; s < DDS_SUITS; s++)
    {
      int c = dl.remainCards[h][s] >> 2;

      cardCount      += counttable[c];
      thrp->diffDeal += (c ^ (thrp->game.suit[h][s]));
      thrp->aggDeal  += c;

      if (thrp->game.suit[h][s] != c)
      {
        thrp->game.suit[h][s] = c;
        thrp->newDeal         = true;
      }
    }
  }

  if (thrp->newDeal)
  {
    if (thrp->diffDeal == 0)
      thrp->similarDeal = true;
    else if ((thrp->aggDeal / thrp->diffDeal) > SIMILARDEALLIMIT)
      thrp->similarDeal = true;
    else
      thrp->similarDeal = false;
  }
  else
    thrp->similarDeal = false;

  if (dl.trump != thrp->trump)
    thrp->newTrump = true;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      noOfCardsPerHand[h] += counttable[thrp->game.suit[h][s]];

  for (int h = 1; h < DDS_HANDS; h++)
  {
    if (noOfCardsPerHand[h] != noOfCardsPerHand[0])
    {
      DumpInput(RETURN_CARD_COUNT, dl, target, solutions, mode);
      return RETURN_CARD_COUNT;
    }
  }

  if (dl.currentTrickRank[2])
  {
    if ((dl.currentTrickRank[2] <  2) || 
        (dl.currentTrickRank[2] > 14) || 
	(dl.currentTrickSuit[2] <  0) || 
	(dl.currentTrickSuit[2] >  3))
    {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }

    thrp->handToPlay = handId(dl.first, 3);
    handRelFirst = 3;
    noStartMoves = 3;

    if (cardCount <= 4)
    {
      int htp = thrp->handToPlay;
      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[htp][s] != 0)
        {
          latestTrickSuit[htp] = s;
          latestTrickRank[htp] = InvBitMapRank(thrp->game.suit[htp][s]);
          break;
        }
      }
      latestTrickSuit[handId(dl.first, 2)] = dl.currentTrickSuit[2];
      latestTrickRank[handId(dl.first, 2)] = dl.currentTrickRank[2];
      latestTrickSuit[handId(dl.first, 1)] = dl.currentTrickSuit[1];
      latestTrickRank[handId(dl.first, 1)] = dl.currentTrickRank[1];
      latestTrickSuit[dl.first] = dl.currentTrickSuit[0];
      latestTrickRank[dl.first] = dl.currentTrickRank[0];
    }
  }
  else if (dl.currentTrickRank[1])
  {
    if ((dl.currentTrickRank[1] <  2) || 
        (dl.currentTrickRank[1] > 14) || 
	(dl.currentTrickSuit[1] <  0) || 
	(dl.currentTrickSuit[1] >  3))
    {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }

    thrp->handToPlay = handId(dl.first, 2);
    handRelFirst = 2;
    noStartMoves = 2;

    if (cardCount <= 4)
    {
      int htp = thrp->handToPlay;
      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[htp][s] != 0)
        {
          latestTrickSuit[htp] = s;
          latestTrickRank[htp] = InvBitMapRank(thrp->game.suit[htp][s]);
          break;
        }
      }

      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[handId(dl.first, 3)][s] != 0)
        {
          latestTrickSuit[handId(dl.first, 3)] = s;
          latestTrickRank[handId(dl.first, 3)] =
            InvBitMapRank(thrp->game.suit[handId(dl.first, 3)][s]);
          break;
        }
      }

      latestTrickSuit[handId(dl.first, 1)] = dl.currentTrickSuit[1];
      latestTrickRank[handId(dl.first, 1)] = dl.currentTrickRank[1];
      latestTrickSuit[dl.first] = dl.currentTrickSuit[0];
      latestTrickRank[dl.first] = dl.currentTrickRank[0];
    }
  }
  else if (dl.currentTrickRank[0])
  {
    if ((dl.currentTrickRank[0] <  2) || 
        (dl.currentTrickRank[0] > 14) || 
	(dl.currentTrickSuit[0] <  0) || 
	(dl.currentTrickSuit[0] >  3))
    {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }

    thrp->handToPlay = handId(dl.first, 1);
    handRelFirst = 1;
    noStartMoves = 1;
    if (cardCount <= 4)
    {
      for (int s = 0; s < DDS_SUITS; s++)
      {
        int htp = thrp->handToPlay;
        if (thrp->game.suit[htp][s] != 0)
        {
          latestTrickSuit[htp] = s;
          latestTrickRank[htp] = InvBitMapRank(thrp->game.suit[htp][s]);
          break;
        }
      }

      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[handId(dl.first, 3)][s] != 0)
        {
          latestTrickSuit[handId(dl.first, 3)] = s;
          latestTrickRank[handId(dl.first, 3)] =
            InvBitMapRank(thrp->game.suit[handId(dl.first, 3)][s]);
          break;
        }
      }

      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[handId(dl.first, 2)][s] != 0)
        {
          latestTrickSuit[handId(dl.first, 2)] = s;
          latestTrickRank[handId(dl.first, 2)] =
            InvBitMapRank(thrp->game.suit[handId(dl.first, 2)][s]);
          break;
        }
      }

      latestTrickSuit[dl.first] = dl.currentTrickSuit[0];
      latestTrickRank[dl.first] = dl.currentTrickRank[0];
    }
  }
  else
  {
    thrp->handToPlay = dl.first;
    handRelFirst = 0;
    noStartMoves = 0;
    if (cardCount <= 4)
    {
      for (int s = 0; s < DDS_SUITS; s++)
      {
        int htp = thrp->handToPlay;
        if (thrp->game.suit[htp][s] != 0)
        {
          latestTrickSuit[htp] = s;
          latestTrickRank[htp] = InvBitMapRank(thrp->game.suit[htp][s]);
          break;
        }
      }

      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[handId(dl.first, 3)][s] != 0)
        {
          latestTrickSuit[handId(dl.first, 3)] = s;
          latestTrickRank[handId(dl.first, 3)] =
            InvBitMapRank(thrp->game.suit[handId(dl.first, 3)][s]);
          break;
        }
      }

      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[handId(dl.first, 2)][s] != 0)
        {
          latestTrickSuit[handId(dl.first, 2)] = s;
          latestTrickRank[handId(dl.first, 2)] =
            InvBitMapRank(thrp->game.suit[handId(dl.first, 2)][s]);
          break;
        }
      }

      for (int s = 0; s < DDS_SUITS; s++)
      {
        if (thrp->game.suit[handId(dl.first, 1)][s] != 0)
        {
          latestTrickSuit[handId(dl.first, 1)] = s;
          latestTrickRank[handId(dl.first, 1)] =
            InvBitMapRank(thrp->game.suit[handId(dl.first, 1)][s]);
          break;
        }
      }
    }
  }

  thrp->trump = dl.trump;
  thrp->game.first = dl.first;
  first = dl.first;
  thrp->game.noOfCards = cardCount;
  if (dl.currentTrickRank[0] != 0)
  {
    thrp->game.leadHand = dl.first;
    thrp->game.leadSuit = dl.currentTrickSuit[0];
    thrp->game.leadRank = dl.currentTrickRank[0];
  }
  else
  {
    thrp->game.leadHand = 0;
    thrp->game.leadSuit = 0;
    thrp->game.leadRank = 0;
  }

  for (int k = 0; k <= 2; k++)
  {
    thrp->initialMoves[k].suit = 255;
    thrp->initialMoves[k].rank = 255;
  }

  for (int k = 0; k < noStartMoves; k++)
  {
    thrp->initialMoves[noStartMoves - 1 - k].suit = 
      dl.currentTrickSuit[k];
    thrp->initialMoves[noStartMoves - 1 - k].rank = 
      dl.currentTrickRank[k];
  }

  if (cardCount % 4)
    totalTricks = ((cardCount - 4) >> 2) + 2;
  else
    totalTricks = ((cardCount - 4) >> 2) + 1;

  if (thrp->game.noOfCards <= 0)
  {
    DumpInput(RETURN_ZERO_CARDS, dl, target, solutions, mode);
    return RETURN_ZERO_CARDS;
  }

  if (thrp->game.noOfCards > 52)
  {
    DumpInput(RETURN_TOO_MANY_CARDS, dl, target, solutions, mode);
    return RETURN_TOO_MANY_CARDS;
  }

  if (totalTricks < target)
  {
    DumpInput(RETURN_TARGET_TOO_HIGH, dl, target, solutions, mode);
    return RETURN_TARGET_TOO_HIGH;
  }

  if (CheckDeal(&thrp->game))
  {
    DumpInput(RETURN_DUPLICATE_CARDS, dl, target, solutions, mode);
    return RETURN_DUPLICATE_CARDS;
  }

  if (cardCount <= 4)
  {
    maxRank = 0;
    /* Highest trump? */
    if (dl.trump != DDS_NOTRUMP)
    {
      for (int h = 0; h < DDS_HANDS; h++)
      {
        if ((latestTrickSuit[h] == dl.trump) &&
            (latestTrickRank[h] > maxRank))
        {
          maxRank = latestTrickRank[h];
          maxSuit = dl.trump;
          maxHand = h;
        }
      }
    }

    /* Highest card in leading suit */
    if (maxRank == 0)
    {
      for (int h = 0; h < DDS_HANDS; h++)
      {
        if (h == dl.first)
        {
          maxSuit = latestTrickSuit[dl.first];
          maxHand = dl.first;
          maxRank = latestTrickRank[dl.first];
          break;
        }
      }
      for (int h = 0; h < DDS_HANDS; h++)
      {
        if ((h != dl.first) && (latestTrickSuit[h] == maxSuit) &&
            (latestTrickRank[h] > maxRank))
        {
          maxHand = h;
          maxRank = latestTrickRank[h];
        }
      }
    }
    futp->nodes      = 0;
    futp->cards      = 1;
    futp->suit[0]    = latestTrickSuit[thrp->handToPlay];
    futp->rank[0]    = latestTrickRank[thrp->handToPlay];
    futp->equals[0]  = 0;

    if ((target == 0) && (solutions < 3))
      futp->score[0] = 0;
    else if ((thrp->handToPlay == maxHand) ||
             (partner[thrp->handToPlay] == maxHand))
      futp->score[0] = 1;
    else
      futp->score[0] = 0;

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif
    return RETURN_NO_FAULT;
  }

  if ((mode != 2) &&
      (((thrp->newDeal) && (! thrp->similarDeal)) || 
         thrp->newTrump  ||
        (thrp->nodes > SIMILARMAXWINNODES)))
  {
    thrp->transTable.ResetMemory();

    InitGame(0, false, first, handRelFirst, thrId);
  }
  else
  {
    InitGame(0, true, first, handRelFirst, thrId);
  }

#ifdef DDS_AB_STATS
  thrp->ABstats.ResetCum();
#endif

#ifdef DDS_TOP_LEVEL
  thrp->nodes = 0;
#endif

  thrp->trickNodes = 0;
  thrp->iniDepth   = cardCount - 4;

  if (mode == 0)
  {
    int iniDepth = thrp->iniDepth;
    MoveGen(&thrp->lookAheadPos, 
             thrp->iniDepth, 
	     thrp->trump,
            &thrp->movePly[iniDepth], 
	    thrp);

    if (thrp->movePly[iniDepth].last == 0)
    {
      futp->nodes      = 0;
      futp->cards      = 1;
      futp->suit[0]    = thrp->movePly[iniDepth].move[0].suit;
      futp->rank[0]    = thrp->movePly[iniDepth].move[0].rank;
      futp->equals[0]  = thrp->movePly[iniDepth].move[0].sequence << 2;
      futp->score[0]   = -2;

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif
      return RETURN_NO_FAULT;
    }
  }
  if ((target == 0) && (solutions < 3))
  {
    MoveGen(&thrp->lookAheadPos, 
             thrp->iniDepth, 
	     thrp->trump,
            &thrp->movePly[thrp->iniDepth], 
	     thrp);

    futp->nodes       = 0;
    int iniDepth = thrp->iniDepth;
    for (int k = 0; k <= thrp->movePly[iniDepth].last; k++)
    {
      futp->suit[k]   = thrp->movePly[iniDepth].move[k].suit;
      futp->rank[k]   = thrp->movePly[iniDepth].move[k].rank;
      futp->equals[k] = thrp->movePly[iniDepth].move[k].sequence << 2;
      futp->score[k]  = 0;
    }
    if (solutions == 1)
      futp->cards = 1;
    else
      futp->cards = thrp->movePly[iniDepth].last + 1;

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif
    return RETURN_NO_FAULT;
  }

  if ((target != -1) && (solutions != 3))
  {
    int fnc = (48 - thrp->iniDepth) % 4;
    TIMER_START(TIMER_AB + thrp->iniDepth);
    thrp->val = (* AB_ptr_list[fnc])(&thrp->lookAheadPos,
                          thrp->tricksTarget, 
			  thrp->iniDepth, 
			  thrp);
    TIMER_END(TIMER_AB + thrp->iniDepth);

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp, thrp->tricksTarget, -1, -1, 0);
#endif

    temp = thrp->movePly[thrp->iniDepth];
    last = thrp->movePly[thrp->iniDepth].last;
    noMoves = last + 1;

    if (thrp->val)
      thrp->payOff = thrp->tricksTarget;
    else
      thrp->payOff = 0;
    futp->cards = 1;

    int n = thrp->game.noOfCards - 4;
    if (thrp->payOff <= 0)
    {
      futp->suit[0]    = thrp->movePly[n].move[0].suit;
      futp->rank[0]    = thrp->movePly[n].move[0].rank;
      futp->equals[0]  = thrp->movePly[n].move[0].sequence << 2;

      if (thrp->tricksTarget > 1)
        futp->score[0] = -1;
      else
        futp->score[0] = 0;
    }
    else
    {
      futp->suit[0]   = thrp->bestMove[n].suit;
      futp->rank[0]   = thrp->bestMove[n].rank;
      futp->equals[0] = thrp->bestMove[n].sequence << 2;
      futp->score[0]  = thrp->payOff;
    }
  }
  else
  {
    int g = thrp->estTricks[thrp->handToPlay];
    int upperbound = 13;
    int lowerbound = 0;
    do
    {
      int tricks;
      if (g == lowerbound)
        tricks = g + 1;
      else
        tricks = g;

      assert((thrp->lookAheadPos.handRelFirst >= 0) &&
             (thrp->lookAheadPos.handRelFirst <= 3));

      int fnc = (48 - thrp->iniDepth) % 4;
      TIMER_START(TIMER_AB + thrp->iniDepth);
      thrp->val = (* AB_ptr_list[fnc])(&thrp->lookAheadPos,
                            tricks,
                            thrp->iniDepth, 
			    thrp);
      TIMER_END(TIMER_AB + thrp->iniDepth);

      if (thrp->val)
        mv = thrp->bestMove[thrp->game.noOfCards - 4];

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp, tricks, lowerbound, upperbound, 1);
#endif

      if (! thrp->val)
      {
        upperbound = tricks - 1;
        g = upperbound;
      }
      else
      {
        lowerbound = tricks;
        g = lowerbound;
      }

      InitSearch(&thrp->iniPosition, 
                  thrp->game.noOfCards - 4,
                  thrp->initialMoves, 
		  first, 
		  true, 
		  thrId);
    }
    while (lowerbound < upperbound);

    thrp->payOff = g;
    temp = thrp->movePly[thrp->iniDepth];
    last = thrp->movePly[thrp->iniDepth].last;
    noMoves = last + 1;

    int n = thrp->game.noOfCards - 4;

    thrp->bestMove[n] = mv;
    futp->cards = 1;

    if (thrp->payOff <= 0)
    {
      futp->score[0]  = 0;
      futp->suit[0]   = thrp->movePly[n].move[0].suit;
      futp->rank[0]   = thrp->movePly[n].move[0].rank;
      futp->equals[0] = thrp->movePly[n].move[0].sequence << 2;
    }
    else
    {
      futp->score[0]  = thrp->payOff;
      futp->suit[0]   = thrp->bestMove[n].suit;
      futp->rank[0]   = thrp->bestMove[n].rank;
      futp->equals[0] = thrp->bestMove[n].sequence << 2;
    }
    thrp->tricksTarget = thrp->payOff;
  }

  if ((solutions == 2) && (thrp->payOff > 0))
  {
    int forb = 1;
    int ind = forb;

    while ((thrp->payOff == thrp->tricksTarget) && 
           (ind < (temp.last + 1)))
    {
      int n = thrp->game.noOfCards - 4;
      thrp->forbiddenMoves[forb].suit = thrp->bestMove[n].suit;
      thrp->forbiddenMoves[forb].rank = thrp->bestMove[n].rank;
      forb++;
      ind++;

      /* All moves before bestMove in the move list shall be
      moved to the forbidden moves list, since none of them reached
      the target */
      int iniDepth = thrp->iniDepth;
      int k;
      for (k = 0; k <= thrp->movePly[iniDepth].last; k++)
      {
        if ((thrp->bestMove[iniDepth].suit ==
             thrp->movePly[iniDepth].move[k].suit) && 
	     (thrp->bestMove[iniDepth].rank ==
                thrp->movePly[iniDepth].move[k].rank))
          break;
      }

      for (int i = 0; i < k; i++) /* All moves until best move */
      {
        bool flag = false;
        for (int j = 0; j < forb; j++)
        {
          if ((thrp->movePly[iniDepth].move[i].suit == 
	       thrp->forbiddenMoves[j].suit) && 
	      (thrp->movePly[iniDepth].move[i].rank == 
	       thrp->forbiddenMoves[j].rank))
          {
            /* If the move is already in the forbidden list */
            flag = true;
            break;
          }
        }

        if (! flag)
        {
          thrp->forbiddenMoves[forb] = thrp->movePly[iniDepth].move[i];
          forb++;
        }
      }

      InitSearch(&thrp->iniPosition, 
                  n,
                  thrp->initialMoves, 
		  first, 
		  true, 
		  thrId);

      int fnc = (48 - thrp->iniDepth) % 4;
      TIMER_START(TIMER_AB + thrp->iniDepth);
      thrp->val = (* AB_ptr_list[fnc])(&thrp->lookAheadPos,
                            thrp->tricksTarget,
                            thrp->iniDepth, 
			    thrp);
      TIMER_END(TIMER_AB + thrp->iniDepth);

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp, thrp->tricksTarget, -1, -1, 2);
#endif

      if (thrp->val)
      {
        int n = thrp->game.noOfCards - 4;
        thrp->payOff = thrp->tricksTarget;

        futp->cards           = ind;
        futp->suit[ind - 1]   = thrp->bestMove[n].suit;
        futp->rank[ind - 1]   = thrp->bestMove[n].rank;
        futp->equals[ind - 1] = thrp->bestMove[n].sequence << 2;
        futp->score[ind - 1]  = thrp->payOff;
      }
      else
        thrp->payOff = 0;
    }
  }
  else if ((solutions == 2) && 
           (thrp->payOff == 0) &&
           ((target == -1) || (thrp->tricksTarget == 1)))
  {
    futp->cards = noMoves;
    /* Find the cards that were in the initial move list
    but have not been listed in the current result */
    int n = 0;
    for (int i = 0; i < noMoves; i++)
    {
      bool found = false;
      if ((temp.move[i].suit == futp->suit[0]) &&
          (temp.move[i].rank == futp->rank[0]))
      {
        found = true;
      }
      if (! found)
      {
        futp->suit[1 + n]   = temp.move[i].suit;
        futp->rank[1 + n]   = temp.move[i].rank;
        futp->equals[1 + n] = temp.move[i].sequence << 2;
        futp->score[1 + n]  = 0;
        n++;
      }
    }
  }

  if ((solutions == 3) && (thrp->payOff > 0))
  {
    int forb = 1;
    int ind = forb;
    for (int i = 0; i < last; i++)
    {
      int n = thrp->game.noOfCards - 4;
      thrp->forbiddenMoves[forb].suit = thrp->bestMove[n].suit;
      thrp->forbiddenMoves[forb].rank = thrp->bestMove[n].rank;
      forb++;
      ind++;

      int g = thrp->payOff;
      int upperbound = g;
      int lowerbound = 0;

      InitSearch(&thrp->iniPosition, 
                  n,
                 thrp->initialMoves, 
		 first, 
		 true, 
		 thrId);

      do
      {
        int tricks;
        if (g == lowerbound)
          tricks = g + 1;
        else
          tricks = g;

        assert((thrp->lookAheadPos.handRelFirst >= 0) &&
               (thrp->lookAheadPos.handRelFirst <= 3));

        int fnc = (48 - thrp->iniDepth) % 4;
        TIMER_START(TIMER_AB + thrp->iniDepth);
        thrp->val = (* AB_ptr_list[fnc])(&thrp->lookAheadPos,
	                      tricks,
                              thrp->iniDepth, 
			      thrp);
        TIMER_END(TIMER_AB + thrp->iniDepth);

        if (thrp->val)
          mv = thrp->bestMove[thrp->game.noOfCards - 4];

#ifdef DDS_TOP_LEVEL
    DumpTopLevel(thrp, tricks, lowerbound, upperbound, 1);
#endif

        if (! thrp->val)
        {
          upperbound = tricks - 1;
          g = upperbound;
        }
        else
        {
          lowerbound = tricks;
          g = lowerbound;
        }

        InitSearch(&thrp->iniPosition, 
	            thrp->game.noOfCards - 4,
                    thrp->initialMoves, 
		    first, 
		    true, 
		    thrId);
      }
      while (lowerbound < upperbound);

      thrp->payOff = g;
      if (thrp->payOff == 0)
      {
        int n = thrp->game.noOfCards - 4;
	int iniDepth = thrp->iniDepth;
        last = thrp->movePly[iniDepth].last;
        futp->cards = temp.last + 1;

        for (int j = 0; j <= last; j++)
        {
          futp->suit[ind-1+j]   = thrp->movePly[n].move[j].suit;
          futp->rank[ind-1+j]   = thrp->movePly[n].move[j].rank;
          futp->equals[ind-1+j] = thrp->movePly[n].move[j].sequence << 2;
          futp->score[ind-1+j]  = thrp->payOff;
        }
        break;
      }
      else
      {
        thrp->bestMove[thrp->game.noOfCards - 4] = mv;

        futp->cards = ind;
        futp->suit[ind - 1]   = mv.suit;
        futp->rank[ind - 1]   = mv.rank;
        futp->equals[ind - 1] = mv.sequence << 2;
        futp->score[ind - 1]  = thrp->payOff;
      }
    }
  }
  else if ((solutions == 3) && (thrp->payOff == 0))
  {
    futp->cards = noMoves;
    /* Find the cards that were in the initial move list
       but have not been listed in the current result */
    int n = 0;
    for (int i = 0; i < noMoves; i++)
    {
      bool found = false;
      if ((temp.move[i].suit == futp->suit[0]) &&
          (temp.move[i].rank == futp->rank[0]))
      {
        found = true;
      }

      if (! found)
      {
        futp->suit[1 + n]   = temp.move[i].suit;
        futp->rank[1 + n]   = temp.move[i].rank;
        futp->equals[1 + n] = temp.move[i].sequence << 2;
        futp->score[1 + n]  = 0;
        n++;
      }
    }
  }

  for (int k = 0; k <= 13; k++)
  {
    thrp->forbiddenMoves[k].suit = 0;
    thrp->forbiddenMoves[k].rank = 0;
  }

  futp->nodes = thrp->trickNodes;

  // printf("%8d\n", tmp_cnt, thrp->transTable.BlocksInUse());
  // thrp->memUsed = thrp->transTable.MemoryInUse() +
    // ThreadMemoryUsed();

#ifdef DDS_TIMING
  thrp->timer.PrintStats();
#endif

#ifdef DDS_TT_STATS
  // thrp->transTable.PrintAllSuits();
  // thrp->transTable.PrintEntries(10, 0);
  thrp->transTable.PrintSummarySuitStats();
  thrp->transTable.PrintSummaryEntryStats();
  // thrp->transTable.PrintPageSummary();
#endif

#ifdef DDS_MEMORY_LEAKS_WIN32
  _CrtDumpMemoryLeaks();
#endif
  return RETURN_NO_FAULT;
}



int InvBitMapRank(unsigned short bitMap) {

  switch (bitMap) {
    case 0x1000: return 14;
    case 0x0800: return 13;
    case 0x0400: return 12;
    case 0x0200: return 11;
    case 0x0100: return 10;
    case 0x0080: return 9;
    case 0x0040: return 8;
    case 0x0020: return 7;
    case 0x0010: return 6;
    case 0x0008: return 5;
    case 0x0004: return 4;
    case 0x0002: return 3;
    case 0x0001: return 2;
    default: return 0;
  }
}


int CheckDeal(struct gameInfo *gamep) {
  int h, s, k;
  bool found;
  unsigned short int temp[DDS_HANDS][DDS_SUITS];

  for (h=0; h<DDS_HANDS; h++)
    for (s=0; s<DDS_SUITS; s++)
      temp[h][s]=gamep->suit[h][s];

  /* Check that all ranks appear only once within the same suit. */
  for (s=0; s<DDS_SUITS; s++)
    for (k=2; k<=14; k++) {
      found=false;
      for (h=0; h<DDS_HANDS; h++) {
        if ((temp[h][s] & bitMapRank[k])!=0) {
          if (found) {
            return 1;
          }
          else
            found=true;
        }
      }
    }

  return 0;
}



int DumpInput(int errCode, struct deal dl, int target,
    int solutions, int mode) {

  FILE *fp;
  int i, j, k;
  unsigned short ranks[4][4];

  fp=fopen("dump.txt", "w");
  if (fp==NULL)
    return RETURN_UNKNOWN_FAULT;
  fprintf(fp, "Error code=%d\n", errCode);
  fprintf(fp, "\n");
  fprintf(fp, "Deal data:\n");
  if (dl.trump!=4)
    fprintf(fp, "trump=%c\n", cardSuit[dl.trump]);
  else
    fprintf(fp, "trump=N\n");
  fprintf(fp, "first=%c\n", cardHand[dl.first]);
  for (k=0; k<=2; k++)
    if (dl.currentTrickRank[k]!=0)
      fprintf(fp, "index=%d currentTrickSuit=%c currentTrickRank=%c\n",
        k, cardSuit[dl.currentTrickSuit[k]], cardRank[dl.currentTrickRank[k]]);
  for (i=0; i<=3; i++)
    for (j=0; j<=3; j++) {
      fprintf(fp, "index1=%d index2=%d remainCards=%d\n",
        i, j, dl.remainCards[i][j]);
      ranks[i][j]=dl.remainCards[i][/*3-*/j]>>2;
    }
  fprintf(fp, "\n");
  fprintf(fp, "target=%d\n", target);
  fprintf(fp, "solutions=%d\n", solutions);
  fprintf(fp, "mode=%d\n", mode);
  fprintf(fp, "\n");
  PrintDeal(fp, ranks);
  fclose(fp);
  return 0;
}


void PrintDeal(FILE *fp, unsigned short ranks[][4]) {
  int i, count, trickCount=0, s, r;
  bool ec[4];
  for (i=0; i<=3; i++) {
    count=counttable[ranks[3][i]];
    if (count>5)
      ec[i]=true;
    else
      ec[i]=false;
    trickCount=trickCount+count;
  }
  fprintf(fp, "\n");
  for (s=0; s<DDS_SUITS; s++) {
    fprintf(fp, "\t%c ", cardSuit[s]);
    if (!ranks[0][s])
      fprintf(fp, "--");
    else {
      for (r=14; r>=2; r--)
        if ((ranks[0][s] & bitMapRank[r])!=0)
          fprintf(fp, "%c", cardRank[r]);
    }
    fprintf(fp, "\n");
  }
  for (s=0; s<DDS_SUITS; s++) {
    fprintf(fp, "%c ", cardSuit[s]);
    if (!ranks[3][s])
      fprintf(fp, "--");
    else {
      for (r=14; r>=2; r--)
        if ((ranks[3][s] & bitMapRank[r])!=0)
          fprintf(fp, "%c", cardRank[r]);
    }
    if (ec[s])
      fprintf(fp, "\t%c ", cardSuit[s]);
    else
      fprintf(fp, "\t\t%c ", cardSuit[s]);
    if (!ranks[1][s])
      fprintf(fp, "--");
    else {
      for (r=14; r>=2; r--)
        if ((ranks[1][s] & bitMapRank[r])!=0)
            fprintf(fp, "%c", cardRank[r]);
    }
    fprintf(fp, "\n");
  }
  for (s=0; s<DDS_SUITS; s++) {
    fprintf(fp, "\t%c ", cardSuit[s]);
    if (!ranks[2][s])
      fprintf(fp, "--");
    else {
      for (r=14; r>=2; r--)
        if ((ranks[2][s] & bitMapRank[r])!=0)
          fprintf(fp, "%c", cardRank[r]);
    }
    fprintf(fp, "\n");
  }
  fprintf(fp, "\n");
  return;
}

