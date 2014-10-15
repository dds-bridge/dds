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


extern struct relRanksType * relRanks;
extern int lho[4];
extern int rho[4];
extern int partner[4];
extern unsigned short int bitMapRank[16];

void MergeSort(int n, struct moveType *a);

int AdjustMoveList(struct localVarType * thrp);

int WeightAllocTrump(struct pos * posPoint, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, int trump, struct localVarType * thrp);
int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, struct localVarType * thrp);

inline bool WinningMove(
  struct moveType 	* mvp1, 
  struct moveType 	* mvp2, 
  int 			trump);

inline bool WinningMoveNT(
  struct moveType 	* mvp1, 
  struct moveType 	* mvp2);



int MoveGen(
  struct pos 		* posPoint,
  int 			depth,
  int 			trump,
  struct movePlyType	* mply,
  struct localVarType	* thrp)
{
  int k, state = MOVESVALID;
  unsigned short int bitmap_k, removed_ranks;
  char *relp;

  for (int s = 0; s < DDS_SUITS; s++)
    thrp->lowestWin[depth][s] = 0;

  int m = 0;
  int r = posPoint->handRelFirst;
  int first = posPoint->first[depth];
  int q = handId(first, r);

  if (r != 0)
  {
    int s = thrp->movePly[depth + r].current; 
    /* Current move of first hand */
    int t = thrp->movePly[depth + r].move[s].suit;    
    /* Suit played by first hand */
    unsigned short ris = posPoint->rankInSuit[q][t];
    relp = relRank[ris];

    if (ris != 0)
    {
      /* Not first hand and not void in suit */
      removed_ranks = posPoint->removedRanks[t];
      k = 14;
      while (k >= 2)
      {
        if (state == MOVESLOCKED)
        {
          bitmap_k = bitMapRank[k];
          if (relp[k])
            /* If the card is in own hand */
            mply->move[m - 1].sequence |= bitmap_k;
          else if ((removed_ranks & bitmap_k) == 0)
            /* If the card still exists and it is not in own hand */
            state = MOVESVALID;
        }
	else if (relp[k])
        {
          /* Only first move in sequence is generated */
          mply->move[m].suit = t;
          mply->move[m].rank = k;
          mply->move[m].sequence = 0;
          m++;
          state = MOVESLOCKED;
        }
        k--;
      }
      if (m != 1)
      {
        if ((trump != DDS_NOTRUMP) && 
	    (posPoint->winner[trump].rank != 0))
        {
          for (k = 0; k <= m - 1; k++)
            mply->move[k].weight = WeightAllocTrump(posPoint,
              &(mply->move[k]), depth, ris, trump, thrp);
        }
        else
        {
          for (k = 0; k <= m - 1; k++)
            mply->move[k].weight = WeightAllocNT(posPoint,
              &(mply->move[k]), depth, ris, thrp);
        }
      }

      mply->last = m - 1;
      if (m != 1)
        MergeSort(m, mply->move);
      if (depth != thrp->iniDepth)
        return m;
      else
      {
        m = AdjustMoveList(thrp);
        return m;
      }
    }
  }


  /* First hand or void in suit */
  for (int suit = 0; suit < DDS_SUITS; suit++)
  {
    unsigned short ris = posPoint->rankInSuit[q][suit];
    relp = relRank[ris];
    removed_ranks = posPoint->removedRanks[suit];

    k = 14;
    state = MOVESVALID;
    while (k >= 2)
    {
      if (state == MOVESLOCKED)
      {
        bitmap_k = bitMapRank[k];
        if (relp[k])
          /* If the card is in own hand */
          mply->move[m - 1].sequence |= bitmap_k;
        else if ((removed_ranks & bitmap_k) == 0)
          /* If the card still exists and it is not in own hand */
          state = MOVESVALID;
      }
      else if (relp[k])
      {
        /* Only first move in sequence is generated */
        mply->move[m].suit = suit;
        mply->move[m].rank = k;
        mply->move[m].sequence = 0;
        m++;
        state = MOVESLOCKED;
      }
      k--;
    }
  }

  if ((trump != DDS_NOTRUMP) && (posPoint->winner[trump].rank != 0))
  {
    for (k = 0; k <= m - 1; k++)
      mply->move[k].weight = WeightAllocTrump(posPoint,
        &(mply->move[k]), depth, 0, trump, thrp);
  }
  else
  {
    for (k = 0; k <= m - 1; k++)
      mply->move[k].weight = WeightAllocNT(posPoint,
        &(mply->move[k]), depth, 0, thrp);
  }

  mply->last = m - 1;
  if (m != 1)
    MergeSort(m, mply->move);

  if (depth != thrp->iniDepth)
    return m;
  else
  {
    m = AdjustMoveList(thrp);
    return m;
  }
}


int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
  unsigned short notVoidInSuit, struct localVarType * thrp) {
  int weight=0, k, l, kk, ll, suitAdd=0, leadSuit;
  int suitWeightDelta;
  int thirdBestHand;
  bool winMove=false;  /* If winMove is true, current move can win the current trick. */
  unsigned short suitCount, suitCountLH, suitCountRH;
  int countLH, countRH;

  int first=posPoint->first[depth];
  int q=handId(first, posPoint->handRelFirst);
  int suit=mp->suit;
  unsigned short aggr=0;
  for (int m=0; m<=3; m++)
    aggr|=posPoint->rankInSuit[m][suit];
  int rRank=relRank[aggr][mp->rank];
  

  switch (posPoint->handRelFirst) {
    case 0:
      suitCount=posPoint->length[q][suit];
      suitCountLH=posPoint->length[lho[q]][suit];
      suitCountRH=posPoint->length[rho[q]][suit];

      if (suitCountLH!=0) {
          countLH=(suitCountLH<<2);
      }
      else
        countLH=depth+4;

      if (suitCountRH!=0) {
          countRH=(suitCountRH<<2);
      }
      else
        countRH=depth+4;

      /* Discourage a suit selection where the search tree appears larger than for the
      altenative suits: the search is estimated to be small when the added number of
      alternative cards to play for the opponents is small. */ 

      suitWeightDelta=-((countLH+countRH)<<5)/19;

      if (posPoint->length[partner[q]][suit]==0)
	suitWeightDelta+=-9;	
	  
      if (posPoint->winner[suit].rank==mp->rank) 
        winMove=true;	/* May also have 2nd best, but this card will not be searched. */		   
      else if (posPoint->rankInSuit[partner[first]][suit] >
	(posPoint->rankInSuit[lho[first]][suit] |
	   posPoint->rankInSuit[rho[first]][suit])) {
	winMove=true;
      }			
              
      if (winMove) {
	/* Discourage suit if RHO has second best card.
	   Exception: RHO has singleton. */
	if (posPoint->secondBest[suit].hand==rho[q]) {
	  if (suitCountRH!=1)
	    suitWeightDelta+=-1;
        }
        /* Encourage playing suit if LHO has second highest rank. */
        else if (posPoint->secondBest[suit].hand==lho[q]) {
	  if (suitCountLH!=1)
            suitWeightDelta+=22;
	  else
	    suitWeightDelta+=16; 
	}
	  
        /* Higher weight if also second best rank is present on current side to play, or
        if second best is a singleton at LHO or RHO. */

        if (((posPoint->secondBest[suit].hand!=lho[first])
           ||(suitCountLH==1))&&
           ((posPoint->secondBest[suit].hand!=rho[first])
           ||(suitCountRH==1)))
          weight=suitWeightDelta+45+rRank;
        else
          weight=suitWeightDelta+18+rRank;

        /* Encourage playing cards that previously caused search cutoff
        or was stored as the best move in a transposition table entry match. */

        if ((thrp->bestMove[depth].suit==suit)&&
          (thrp->bestMove[depth].rank==mp->rank)) 
          weight+=126;
        else if ((thrp->bestMoveTT[depth].suit==suit)&&
          (thrp->bestMoveTT[depth].rank==mp->rank)) 
          weight+=32/*24*/;
      }
      else {
	/* Discourage suit if RHO has winning or second best card.
	   Exception: RHO has singleton. */

        if ((posPoint->winner[suit].hand==rho[q])||
          (posPoint->secondBest[suit].hand==rho[q])) {
	  if (suitCountRH!=1)
	    suitWeightDelta+=-10;	
        }


	/* Try suit if LHO has winning card and partner second best. 
	     Exception: partner has singleton. */ 

        else if ((posPoint->winner[suit].hand==lho[q])&&
	  (posPoint->secondBest[suit].hand==partner[q])) {

	/* This case was suggested by Joël Bradmetz. */

	  if (posPoint->length[partner[q]][suit]!=1)
	    suitWeightDelta+=31;
        }
     
	/* Encourage playing the suit if the hand together with partner have both the 2nd highest
	and the 3rd highest cards such that the side of the hand has the highest card in the
	next round playing this suit. */

	thirdBestHand=thrp->rel[aggr].absRank[3][suit].hand;

	if ((posPoint->secondBest[suit].hand==partner[first])&&(partner[first]==thirdBestHand))
	  suitWeightDelta+=35;
	else if(((posPoint->secondBest[suit].hand==first)&&(partner[first]==thirdBestHand)&&
	  (posPoint->length[partner[first]][suit]>1))||((posPoint->secondBest[suit].hand==partner[first])&&
	  (first==thirdBestHand)&&(posPoint->length[partner[first]][suit]>1)))
	  suitWeightDelta+=25;	

	/* Higher weight if LHO or RHO has the highest (winning) card as a singleton. */

	if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+28+rRank;
        else if (posPoint->winner[suit].hand==first) {
          weight=suitWeightDelta-17+rRank;
        }
        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank)) 
	  weight=suitWeightDelta+48;
	else if (mp->sequence)
          weight=suitWeightDelta+29-rRank;
        else 
          weight=suitWeightDelta+12+rRank; 
	
	/* Encourage playing cards that previously caused search cutoff
	or was stored as the best move in a transposition table entry match. */

	if ((thrp->bestMove[depth].suit==suit)&&
            (thrp->bestMove[depth].rank==mp->rank)) 
          weight+=47;
	else if ((thrp->bestMoveTT[depth].suit==suit)&&
            (thrp->bestMoveTT[depth].rank==mp->rank)) 
          weight+=19;
      }
        
      break;

    case 1:
      leadSuit=posPoint->move[depth+1].suit;
      if (leadSuit==suit) {
	if (bitMapRank[mp->rank]>
	    (bitMapRank[posPoint->move[depth+1].rank] |
	    posPoint->rankInSuit[partner[first]][suit])) 
          winMove=true;
	else if (posPoint->rankInSuit[rho[first]][suit]>
	   (bitMapRank[posPoint->move[depth+1].rank] |
	    posPoint->rankInSuit[partner[first]][suit])) 
          winMove=true;
      }
      else {
	/* Side with highest rank in leadSuit wins */

	if (posPoint->rankInSuit[rho[first]][leadSuit] >
           (posPoint->rankInSuit[partner[first]][leadSuit] |
            bitMapRank[posPoint->move[depth+1].rank]))
          winMove=true;			   			  
      }
      
      if (winMove) {
        if (!notVoidInSuit) { 
	  suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(23/*20*//*21*//*24*//*30*//*35*/);
	  if (posPoint->secondBest[suit].hand==q) {
	  /* Discourage suit discard if 2nd highest card becomes singleton. */
 
	    if (suitCount==2)
	      suitAdd+=-2;
	  }
	  /* Discourage suit discard of highest card. */

	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd+=-3;

	  /*Encourage discard of low cards in long suits. */
	    weight=-4-(mp->rank)+suitAdd;		
        }
	else {	
	  weight=81/*80*/+rRank;
        } 
      }
      else {
        if (!notVoidInSuit) {
	  suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/33; 
 
	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd+=-6;	
		  
          /* Discourage suit discard of highest card. */
	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd+=-8;

	  /*Encourage discard of low cards in long suits. */
          weight=2-(mp->rank)+suitAdd; 
        }
        else {

	  /* If lowest rank for either partner to leading hand 
	  or rho is higher than played card for lho,
	  lho should play as low card as possible */
	
	  kk=posPoint->rankInSuit[partner[first]][leadSuit];
          ll=posPoint->rankInSuit[rho[first]][leadSuit];
          k=kk & (-kk); l=ll & (-ll);  /* Only least significant 1 bit of
					bit map ranks for partner and RHO. */

	  if ((k > bitMapRank[mp->rank])||(l > bitMapRank[mp->rank])) 
	    weight=-3+rRank;		
          else if (mp->rank > posPoint->move[depth+1].rank) {
	    if (mp->sequence) { 
	      weight=/*0*/10+rRank;	
	    }
            else { 
              weight=13-(mp->rank);
	      /*weight=-5+rRank;*/
	    }
          }          
          else {
	    weight=-11/*15*/+rRank;		
	  }	
        }
      }

      break;

    case 2:
            
      leadSuit=posPoint->move[depth+2].suit;
      if (WinningMoveNT(mp, &(posPoint->move[depth+1]))) {
	if (bitMapRank[mp->rank] >
	  posPoint->rankInSuit[rho[first]][suit])
	  winMove=true;
      }	
      else if (posPoint->high[depth+1]==first) {
	if (posPoint->rankInSuit[rho[first]][leadSuit]
	      < bitMapRank[posPoint->move[depth+2].rank])	
	  winMove=true;
	
      }
      
      if (winMove) {
        if (!notVoidInSuit) {
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(17/*27*//*30*//*35*/);

	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(6/*2*//*5*/);	
          weight=-(mp->rank)+suitAdd;  
        }
        else { 
          weight=60+rRank;
	}
      }
      else {
        if (!notVoidInSuit) {
	  suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(24/*26*//*29*//*35*/);   
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(4/*5*/);	
	   /* Discourage suit discard of highest card. */
	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd-=(4/*5*/);	

          weight=-(mp->rank)+suitAdd;   

        }
        else {
		  
	  k=posPoint->rankInSuit[rho[first]][suit];
	  if ((k & (-k)) > bitMapRank[mp->rank])
	    weight=-(mp->rank);
          else if (WinningMoveNT(mp, &(posPoint->move[depth+1]))) {
            if ((mp->rank==posPoint->secondBest[leadSuit].rank)&&
				(mp->sequence))
              weight=25;		
            else if (mp->sequence)
              weight=20-(mp->rank);  
            else
              weight=10-(mp->rank);  
          }
          else  
            weight=-10-(mp->rank);  
        } 
      }
            
      break;

    case 3:
      if (!notVoidInSuit) {
	suitCount=posPoint->length[q][suit];
        suitAdd=(suitCount<<6)/(27/*35*/);   
        if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
          suitAdd-=(6/*5*/);				
	else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	  suitAdd-=(8/*9*//*8*//*5*/);	

        weight=30-(mp->rank)+suitAdd;
      }
      else if ((posPoint->high[depth+1])==(lho[first])) {
        /* If the current winning move is given by the partner */
        weight=30-(mp->rank);		
      }
      else if (WinningMoveNT(mp, &(posPoint->move[depth+1])))
        /* If present move is superior to current winning move and the
        current winning move is not given by the partner */
        weight=30-(mp->rank);		
      else {
        /* If present move is not superior to current winning move and the
        current winning move is not given by the partner */
        weight=14-(mp->rank);		
      }
  }
  return weight;
}



int WeightAllocTrump(struct pos * posPoint, struct moveType * mp, int depth,
  unsigned short notVoidInSuit, int trump, struct localVarType * thrp) {
  int weight=0, k, l, kk, ll, suitAdd=0, leadSuit;
  int suitWeightDelta, thirdBestHand;
  int suitBonus=0;
  bool winMove=false;	/* If winMove is true, current move can win the current trick. */
  unsigned short suitCount, suitCountLH, suitCountRH;
  int countLH, countRH;

  int first=posPoint->first[depth];
  int q=handId(first, posPoint->handRelFirst);
  int suit=mp->suit;
  unsigned short aggr=0;
  for (int m=0; m<=3; m++)
    aggr|=posPoint->rankInSuit[m][suit];
  int rRank=relRank[aggr][mp->rank];

  switch (posPoint->handRelFirst) {
    case 0:
      suitCount=posPoint->length[q][suit];
      suitCountLH=posPoint->length[lho[q]][suit];
      suitCountRH=posPoint->length[rho[q]][suit];

      /* Discourage suit if LHO or RHO can ruff. */

      if ((suit!=trump) &&
        (((posPoint->rankInSuit[lho[q]][suit]==0) &&
          (posPoint->rankInSuit[lho[q]][trump]!=0)) ||
          ((posPoint->rankInSuit[rho[q]][suit]==0) &&
          (posPoint->rankInSuit[rho[q]][trump]!=0))))
        suitBonus=-12/*9*//*17*/;
	    
      /* Encourage suit if partner can ruff. */

      if ((suit!=trump)&&(posPoint->length[partner[q]][suit]==0)&&
	     (posPoint->length[partner[q]][trump]>0)&&(suitCountRH>0))
	suitBonus+=17/*26*/;

      /* Discourage suit if RHO has high card. */

      if ((posPoint->winner[suit].hand==rho[q])||
          (posPoint->secondBest[suit].hand==rho[q])) {
	if (suitCountRH!=1)
	  suitBonus+=-12/*13*//*11*/;
      }

      /* Try suit if LHO has winning card and partner second best. 
      Exception: partner has singleton. */ 

      else if ((posPoint->winner[suit].hand==lho[q])&&
	(posPoint->secondBest[suit].hand==partner[q])) {

	/* This case was suggested by Joël Bradmetz. */

	if (posPoint->length[partner[q]][suit]!=1) 
	  suitBonus+=27/*25*//*30*/;
      }
 
      /* Encourage play of suit where partner wins and
      returns the suit for a ruff. */
      if ((suit!=trump)&&(suitCount==1)&&
	(posPoint->length[q][trump]>0)&&
	(posPoint->length[partner[q]][suit]>1)&&
	(posPoint->winner[suit].hand==partner[q]))
	suitBonus+=19/*26*//*23*/;

      if (suitCountLH!=0)
        countLH=(suitCountLH<<2);
      else
        countLH=depth+4;
      if (suitCountRH!=0)
        countRH=(suitCountRH<<2);
      else
        countRH=depth+4;

      /* Discourage a suit selection where the search tree appears larger than for the
	  altenative suits: the search is estimated to be small when the added number of
	  alternative cards to play for the opponents is small. */ 

      suitWeightDelta=suitBonus-
	((countLH+countRH)<<5)/13;

      if (posPoint->winner[suit].rank==mp->rank) {
        if ((suit!=trump)) {
	  if ((posPoint->length[partner[first]][suit]!=0)||
	    (posPoint->length[partner[first]][trump]==0)) {
	    if (((posPoint->length[lho[first]][suit]!=0)||
		  (posPoint->length[lho[first]][trump]==0))&&
		  ((posPoint->length[rho[first]][suit]!=0)||
		  (posPoint->length[rho[first]][trump]==0)))
	      winMove=true;
	  }
	  else if (((posPoint->length[lho[first]][suit]!=0)||
               (posPoint->rankInSuit[partner[first]][trump]>
                posPoint->rankInSuit[lho[first]][trump]))&&
		((posPoint->length[rho[first]][suit]!=0)||
		(posPoint->rankInSuit[partner[first]][trump]>
		 posPoint->rankInSuit[rho[first]][trump])))
	    winMove=true;
	}
        else 
          winMove=true;			   
      }
      else if (posPoint->rankInSuit[partner[first]][suit] >
	(posPoint->rankInSuit[lho[first]][suit] |
	posPoint->rankInSuit[rho[first]][suit])) {
	if (suit!=trump) {
	  if (((posPoint->length[lho[first]][suit]!=0)||
	      (posPoint->length[lho[first]][trump]==0))&&
	      ((posPoint->length[rho[first]][suit]!=0)||
	      (posPoint->length[rho[first]][trump]==0)))
	    winMove=true;
	}
	else
	  winMove=true;
      }			
      else if (suit!=trump) {
        if ((posPoint->length[partner[first]][suit]==0)&&
            (posPoint->length[partner[first]][trump]!=0)) {
	  if ((posPoint->length[lho[first]][suit]==0)&&
              (posPoint->length[lho[first]][trump]!=0)&&
		  (posPoint->length[rho[first]][suit]==0)&&
              (posPoint->length[rho[first]][trump]!=0)) {
	    if (posPoint->rankInSuit[partner[first]][trump]>
	       (posPoint->rankInSuit[lho[first]][trump] |
		posPoint->rankInSuit[rho[first]][trump]))
	      winMove=true;
	  }
	  else if ((posPoint->length[lho[first]][suit]==0)&&
              (posPoint->length[lho[first]][trump]!=0)) {
	    if (posPoint->rankInSuit[partner[first]][trump]
		    > posPoint->rankInSuit[lho[first]][trump])
	        winMove=true;
	  }	
	  else if ((posPoint->length[rho[first]][suit]==0)&&
              (posPoint->length[rho[first]][trump]!=0)) {
	    if (posPoint->rankInSuit[partner[first]][trump]
		    > posPoint->rankInSuit[rho[first]][trump])
	      winMove=true;
	  }	
          else
	    winMove=true;
	}
      }
              
      if (winMove) {

	/* Encourage ruffing LHO or RHO singleton, highest card. */

        if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+35/*37*//*39*/+rRank;

	/* Lead hand has the highest card. */

        else if (posPoint->winner[suit].hand==first) {

	/* Also, partner has second highest card. */

          if (posPoint->secondBest[suit].hand==partner[first])
            weight=suitWeightDelta+/*47*/48+rRank;
	  else if (posPoint->winner[suit].rank==mp->rank)

	    /* If the current card to play is the highest card. */

            weight=suitWeightDelta+31;
          else
            weight=suitWeightDelta-3+rRank;
        }
        else if (posPoint->winner[suit].hand==partner[first]) {
          /* If partner has highest card */
          if (posPoint->secondBest[suit].hand==first)
            weight=suitWeightDelta+42/*35*//*46*//*50*/+rRank;
          else 
            weight=suitWeightDelta+28/*24*/+rRank;  
        } 
	/* Encourage playing second highest rank if hand also has
	third highest rank. */

        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank))			
          weight=suitWeightDelta+40/*41*/;
	else if (mp->sequence)
	  weight=suitWeightDelta+22/*17*/+rRank;
        else
          weight=suitWeightDelta+11+rRank;

	/* Encourage playing cards that previously caused search cutoff
	or was stored as the best move in a transposition table entry match. */

	if ((thrp->bestMove[depth].suit==suit)&&
            (thrp->bestMove[depth].rank==mp->rank)) 
          weight+=55/*53*/;
	else if ((thrp->bestMoveTT[depth].suit==suit)&&
            (thrp->bestMoveTT[depth].rank==mp->rank)) 
          weight+=/*25*/18/*14*/;
      }
      else {

	/* Encourage playing the suit if the hand together with partner have both the 2nd highest
	and the 3rd highest cards such that the side of the hand has the highest card in the
	next round playing this suit. */

	thirdBestHand=thrp->rel[aggr].absRank[3][suit].hand;

	if ((posPoint->secondBest[suit].hand==partner[first])&&(partner[first]==thirdBestHand))
	   suitWeightDelta+=20/*22*/;
	else if(((posPoint->secondBest[suit].hand==first)&&(partner[first]==thirdBestHand)&&
	  (posPoint->length[partner[first]][suit]>1))||
	  ((posPoint->secondBest[suit].hand==partner[first])&&
	  (first==thirdBestHand)&&(posPoint->length[partner[first]][suit]>1)))
	   suitWeightDelta+=13/*20*//*24*/;
	
	/* Higher weight if LHO or RHO has the highest (winning) card as a singleton. */

        if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+rRank+2/*-2*/;
        else if (posPoint->winner[suit].hand==first) {
          if (posPoint->secondBest[suit].hand==partner[first])

	  /* Opponents win by ruffing */

            weight=suitWeightDelta+33+rRank;
          else if (posPoint->winner[suit].rank==mp->rank) 

	  /* Opponents win by ruffing */

            weight=suitWeightDelta+38/*36*/;
          else
            weight=suitWeightDelta-14/*17*/+rRank;
        }
        else if (posPoint->winner[suit].hand==partner[first]) {

          /* Opponents win by ruffing */

          weight=suitWeightDelta+34/*33*/+rRank;
        } 
	/* Encourage playing second highest rank if hand also has
	third highest rank. */

        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank)) 
          weight=suitWeightDelta+35/*31*/;
        else 
	  weight=suitWeightDelta+17/*13*/-(mp->rank);
	
	/* Encourage playing cards that previously caused search cutoff
	or was stored as the best move in a transposition table entry match. */

	if ((thrp->bestMove[depth].suit==suit)&&
            (thrp->bestMove[depth].rank==mp->rank)) 
          weight+=18/*17*/;
	/*else if ((thrp->bestMoveTT[depth].suit==suit)&&
            (thrp->bestMoveTT[depth].rank==mp->rank)) 
          weight+=4;*/
      }
        
      break;

    case 1:
      leadSuit=posPoint->move[depth+1].suit;
      if (leadSuit==suit) {
	if (bitMapRank[mp->rank]>
	    (bitMapRank[posPoint->move[depth+1].rank] |
	    posPoint->rankInSuit[partner[first]][suit])) {
	  if (suit!=trump) {
	    if ((posPoint->length[partner[first]][suit]!=0)||
		  (posPoint->length[partner[first]][trump]==0))
	      winMove=true;
	    else if ((posPoint->length[rho[first]][suit]==0)
                &&(posPoint->length[rho[first]][trump]!=0)
                &&(posPoint->rankInSuit[rho[first]][trump]>
                 posPoint->rankInSuit[partner[first]][trump]))
	      winMove=true;
	  }
          else
            winMove=true;
        }
	else if (posPoint->rankInSuit[rho[first]][suit]>
	    (bitMapRank[posPoint->move[depth+1].rank] |
	    posPoint->rankInSuit[partner[first]][suit])) {	 
	  if (suit!=trump) {
	    if ((posPoint->length[partner[first]][suit]!=0)||
		  (posPoint->length[partner[first]][trump]==0))
	      winMove=true;
	  }
          else
            winMove=true;
	} 
	else if (bitMapRank[posPoint->move[depth+1].rank] >
	    (posPoint->rankInSuit[rho[first]][suit] |
	     posPoint->rankInSuit[partner[first]][suit] |
	     bitMapRank[mp->rank])) {  
	  if (suit!=trump) {
	    if ((posPoint->length[rho[first]][suit]==0)&&
		  (posPoint->length[rho[first]][trump]!=0)) {
	      if ((posPoint->length[partner[first]][suit]!=0)||
		    (posPoint->length[partner[first]][trump]==0))
		winMove=true;
	      else if (posPoint->rankInSuit[rho[first]][trump]
                  > posPoint->rankInSuit[partner[first]][trump])
		winMove=true;
	    }	  
	  }
	}	
	else {   /* winnerHand is partner to first */
	  if (suit!=trump) {
	    if ((posPoint->length[rho[first]][suit]==0)&&
		  (posPoint->length[rho[first]][trump]!=0))
	       winMove=true;
	  }  
	}
      }
      else {

	 /* Leading suit differs from suit played by LHO */

	if (suit==trump) {
	  if (posPoint->length[partner[first]][leadSuit]!=0)
	    winMove=true;
	  else if (bitMapRank[mp->rank]>
		posPoint->rankInSuit[partner[first]][trump]) 
	    winMove=true;
	  else if ((posPoint->length[rho[first]][leadSuit]==0)
              &&(posPoint->length[rho[first]][trump]!=0)&&
              (posPoint->rankInSuit[rho[first]][trump] >
              posPoint->rankInSuit[partner[first]][trump]))
            winMove=true;
        }	
        else if (leadSuit!=trump) {

          /* Neither suit nor leadSuit is trump */

          if (posPoint->length[partner[first]][leadSuit]!=0) {
            if (posPoint->rankInSuit[rho[first]][leadSuit] >
              (posPoint->rankInSuit[partner[first]][leadSuit] |
              bitMapRank[posPoint->move[depth+1].rank]))
              winMove=true;
	    else if ((posPoint->length[rho[first]][leadSuit]==0)
		  &&(posPoint->length[rho[first]][trump]!=0))
	      winMove=true;
	  }

	  /* Partner to leading hand is void in leading suit */

	  else if ((posPoint->length[rho[first]][leadSuit]==0)
		&&(posPoint->rankInSuit[rho[first]][trump]>
	      posPoint->rankInSuit[partner[first]][trump]))
	    winMove=true;
	  else if ((posPoint->length[partner[first]][trump]==0)
	      &&(posPoint->rankInSuit[rho[first]][leadSuit] >
		bitMapRank[posPoint->move[depth+1].rank]))
	    winMove=true;
        }
        else {
	  /* Either no trumps or leadSuit is trump, side with
		highest rank in leadSuit wins */
	  if (posPoint->rankInSuit[rho[first]][leadSuit] >
            (posPoint->rankInSuit[partner[first]][leadSuit] |
             bitMapRank[posPoint->move[depth+1].rank]))
            winMove=true;			   
        }			  
      }
      
	  
      kk=posPoint->rankInSuit[partner[first]][leadSuit];
      ll=posPoint->rankInSuit[rho[first]][leadSuit];
      k=kk & (-kk); l=ll & (-ll);  /* Only least significant 1 bit of
				   bit map ranks for partner and RHO. */

      if (winMove) {
        if (!notVoidInSuit) { 
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(44/*36*/);

	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 

          /*if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=2;*/

          if (suit==trump)  
	        weight=24-(mp->rank)+suitAdd;
          else
            weight=60-(mp->rank)+suitAdd;  /* Better discard than ruff since rho
								wins anyway */		
        } 
        else if (k > bitMapRank[mp->rank])
	  weight=40+rRank;

          /* If lowest card for partner to leading hand 
	    is higher than lho played card, playing as low as 
	    possible will give the cheapest win */

        else if ((ll > bitMapRank[posPoint->move[depth+1].rank])&&
          (posPoint->rankInSuit[first][leadSuit] > ll))
	      weight=41+rRank;

	  /* If rho has a card in the leading suit that
             is higher than the trick leading card but lower
             than the highest rank of the leading hand, then
             lho playing the lowest card will be the cheapest
             win */

	else if (mp->rank > posPoint->move[depth+1].rank) {
          if (bitMapRank[mp->rank] < ll) 
            weight=78-(mp->rank);  /* If played card is lower than any of the cards of
					rho, it will be the cheapest win */		
          else if (bitMapRank[mp->rank] > kk)
            weight=73-(mp->rank);  /* If played card is higher than any cards at partner
				    of the leading hand, rho can play low, under the
                                    condition that he has a lower card than lho played */    
          else {
            if (mp->sequence)
              weight=62-(mp->rank);   
            else
              weight=49-(mp->rank);  
          }
        } 
        else if (posPoint->length[rho[first]][leadSuit]>0) {
          /*if (mp->sequence)*/
            weight=47-(mp->rank);  /* Playing a card in a sequence may promote a winner */
												
          /*else
            weight=47-(mp->rank);*/	
        }
        else
          weight=40-(mp->rank);
      }
      else {
        if (!notVoidInSuit) { 
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/36;
	
	  /* Discourage suit discard if 2nd highest card becomes singleton. */
 
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd+=-4;
  
	  if (suit==trump) {
            weight=15-(mp->rank)+suitAdd;  /* Ruffing is preferred, makes the trick
						costly for the opponents */
	  }
          else
            weight=-2-(mp->rank)+suitAdd;
        }
        else if ((k > bitMapRank[mp->rank])||
          (l > bitMapRank[mp->rank]))
	  weight=-9+rRank;

          /* If lowest rank for either partner to leading hand 
	  or rho is higher than played card for lho,
	  lho should play as low card as possible */
			
        else if (mp->rank > posPoint->move[depth+1].rank) {		  
          if (mp->sequence) 
            weight=22-(mp->rank);	
          else 
            weight=10-(mp->rank);
        }          
        else
	  weight=-16+rRank;
      }
      break;

    case 2:
            
      leadSuit=posPoint->move[depth+2].suit;
      if (WinningMove(mp, &(posPoint->move[depth+1]),trump)) {
	if (suit==leadSuit) {
	  if (leadSuit!=trump) {
	    if (((posPoint->length[rho[first]][suit]!=0)||
		(posPoint->length[rho[first]][trump]==0))&&
		  (bitMapRank[mp->rank] >
		   posPoint->rankInSuit[rho[first]][suit]))
	      winMove=true;
	  }	
	  else if (bitMapRank[mp->rank] >
	    posPoint->rankInSuit[rho[first]][suit])
	    winMove=true;
	}
	else {  /* Suit is trump */
	  if (posPoint->length[rho[first]][leadSuit]==0) {
	    if (bitMapRank[mp->rank] >
		  posPoint->rankInSuit[rho[first]][trump])
	      winMove=true;
	  }
	  else
	    winMove=true;
	}
      }	
      else if (posPoint->high[depth+1]==first) {
	if (posPoint->length[rho[first]][leadSuit]!=0) {
	  if (posPoint->rankInSuit[rho[first]][leadSuit]
		 < bitMapRank[posPoint->move[depth+2].rank])	
	    winMove=true;
	}
	else if (leadSuit==trump)
          winMove=true;
	else if ((leadSuit!=trump) &&
	    (posPoint->length[rho[first]][trump]==0))
	  winMove=true;
      }
      
      if (winMove) {
        if (!notVoidInSuit) {
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(50/*36*/);

	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 

          /*if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(3);*/
        
          if (posPoint->high[depth+1]==first) {
            if (suit==trump) 
              weight=48-(mp->rank)+suitAdd;  /* Ruffs partner's winner */  
	    else 
              weight=61-(mp->rank)+suitAdd;  
          } 
          else if (WinningMove(mp, &(posPoint->move[depth+1]),trump))

             /* Own hand on top by ruffing */

            weight=72-(mp->rank)+suitAdd;  
        }
        else 
          weight=58-(mp->rank);	
      }
      else {
        if (!notVoidInSuit) {
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/40;

	  /* Discourage suit discard if 2nd highest card becomes singleton. */
 
	  /*if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(4);*/	
          
          if (WinningMove(mp, &(posPoint->move[depth+1]),trump))

             /* Own hand on top by ruffing */

            weight=40-(mp->rank)+suitAdd;  
          else if (suit==trump)

            /* Discard a trump but still losing */

	    weight=-32+rRank+suitAdd;
          else
            weight=-2-(mp->rank)+suitAdd;
        }
        else {
	  k=posPoint->rankInSuit[rho[first]][suit];
	  if ((k & (-k)) > bitMapRank[mp->rank])

	    /* If least bit map rank of RHO to lead hand is higher than bit map rank
		of current card move. */

	    weight=-3-(mp->rank);

          else if (WinningMove(mp, &(posPoint->move[depth+1]),trump)) {

	    /* If current card move is highest so far. */

            if (mp->rank==posPoint->secondBest[leadSuit].rank)
              weight=29;		
            else if (mp->sequence)
              weight=26/*20*/-(mp->rank);
            else
              weight=18-(mp->rank);
          }
          else  
            weight=-12-(mp->rank);  
        } 
      }
            
      break;

    case 3:
      if (!notVoidInSuit) {
        suitCount=posPoint->length[q][suit];
        suitAdd=(suitCount<<6)/(24/*36*/);
        if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
          suitAdd-=(2);

        if ((posPoint->high[depth+1])==lho[first]) {

          /* If the current winning move is given by the partner */

          if (suit==trump)

            /* Ruffing partners winner? */

            weight=2/*17*/-(mp->rank)+suitAdd;
          else 
            weight=25-(mp->rank)+suitAdd;
        }
        else if (WinningMove(mp, &(posPoint->move[depth+1]),trump)) 

          /* Own hand ruffs */

	  weight=33/*27*/+rRank+suitAdd;			
        else if (suit==trump) 
	  weight=-13+rRank;					
        else 
          weight=14-(mp->rank)+suitAdd;  
      }
      else if ((posPoint->high[depth+1])==(lho[first])) {

        /* If the current winning move is given by the partner */

        if (suit==trump)

        /* Ruffs partners winner */

	  weight=11+rRank;					
        else 
	  weight=17+rRank;
      }
      else if (WinningMove(mp, &(posPoint->move[depth+1]),trump))

        /* If present move is superior to current winning move and the
        current winning move is not given by the partner */

	weight=22+rRank;		
      else {

        /* If present move is not superior to current winning move and the
        current winning move is not given by the partner */

        if (suit==trump)

          /* Ruffs but still loses */

	  weight=-13+rRank;			
        else 
	  weight=1+rRank;			
      }
  }
  return weight;
}


void MergeSort(int n, struct moveType *a) {

  switch (n) {
  case 12:
	CMP_SWAP(0, 1); CMP_SWAP(2, 3); CMP_SWAP(4, 5); CMP_SWAP(6, 7); CMP_SWAP(8, 9); CMP_SWAP(10, 11);
	CMP_SWAP(1, 3); CMP_SWAP(5, 7); CMP_SWAP(9, 11);
	CMP_SWAP(0, 2); CMP_SWAP(4, 6); CMP_SWAP(8, 10);
	CMP_SWAP(1, 2); CMP_SWAP(5, 6); CMP_SWAP(9, 10);
	CMP_SWAP(1, 5); CMP_SWAP(6, 10);
	CMP_SWAP(5, 9);
	CMP_SWAP(2, 6);
	CMP_SWAP(1, 5); CMP_SWAP(6, 10);
	CMP_SWAP(0, 4); CMP_SWAP(7, 11);
	CMP_SWAP(3, 7);
	CMP_SWAP(4, 8);
	CMP_SWAP(0, 4); CMP_SWAP(7, 11);
	CMP_SWAP(1, 4); CMP_SWAP(7, 10);
	CMP_SWAP(3, 8);
	CMP_SWAP(2, 3); CMP_SWAP(8, 9);
	CMP_SWAP(2, 4); CMP_SWAP(7, 9);
	CMP_SWAP(3, 5); CMP_SWAP(6, 8);
	CMP_SWAP(3, 4); CMP_SWAP(5, 6); CMP_SWAP(7, 8);
  	break;
  case 11:
	CMP_SWAP(0, 1); CMP_SWAP(2, 3); CMP_SWAP(4, 5); CMP_SWAP(6, 7); CMP_SWAP(8, 9);
	CMP_SWAP(1, 3); CMP_SWAP(5, 7);
	CMP_SWAP(0, 2); CMP_SWAP(4, 6); CMP_SWAP(8, 10);
	CMP_SWAP(1, 2); CMP_SWAP(5, 6); CMP_SWAP(9, 10);
	CMP_SWAP(1, 5); CMP_SWAP(6, 10);
	CMP_SWAP(5, 9);
	CMP_SWAP(2, 6);
	CMP_SWAP(1, 5); CMP_SWAP(6, 10);
	CMP_SWAP(0, 4);
	CMP_SWAP(3, 7);
	CMP_SWAP(4, 8);
	CMP_SWAP(0, 4);
	CMP_SWAP(1, 4); CMP_SWAP(7, 10);
	CMP_SWAP(3, 8);
	CMP_SWAP(2, 3); CMP_SWAP(8, 9);
	CMP_SWAP(2, 4); CMP_SWAP(7, 9);
	CMP_SWAP(3, 5); CMP_SWAP(6, 8);
	CMP_SWAP(3, 4); CMP_SWAP(5, 6); CMP_SWAP(7, 8);
  	break;
  case 10:
	CMP_SWAP(1, 8);
	CMP_SWAP(0, 4); CMP_SWAP(5, 9);
	CMP_SWAP(2, 6);
	CMP_SWAP(3, 7);
	CMP_SWAP(0, 3); CMP_SWAP(6, 9);
	CMP_SWAP(2, 5);
	CMP_SWAP(0, 1); CMP_SWAP(3, 6); CMP_SWAP(8, 9);
	CMP_SWAP(4, 7);
	CMP_SWAP(0, 2); CMP_SWAP(4, 8);
    	CMP_SWAP(1, 5); CMP_SWAP(7, 9);
	CMP_SWAP(1, 2); CMP_SWAP(3, 4); CMP_SWAP(5, 6); CMP_SWAP(7, 8);
	CMP_SWAP(1, 3); CMP_SWAP(6, 8);
	CMP_SWAP(2, 4); CMP_SWAP(5, 7);
	CMP_SWAP(2, 3); CMP_SWAP(6, 7);
	CMP_SWAP(3, 5);
	CMP_SWAP(4, 6);
	CMP_SWAP(4, 5);
	break;
  case 9:
	CMP_SWAP(0, 1); CMP_SWAP(3, 4); CMP_SWAP(6, 7);
	CMP_SWAP(1, 2); CMP_SWAP(4, 5); CMP_SWAP(7, 8);
	CMP_SWAP(0, 1); CMP_SWAP(3, 4); CMP_SWAP(6, 7);
	CMP_SWAP(0, 3);
	CMP_SWAP(3, 6);
	CMP_SWAP(0, 3);
	CMP_SWAP(1, 4);
	CMP_SWAP(4, 7);
	CMP_SWAP(1, 4);
	CMP_SWAP(2, 5);
	CMP_SWAP(5, 8);
	CMP_SWAP(2, 5);
	CMP_SWAP(1, 3); CMP_SWAP(5, 7);
	CMP_SWAP(2, 6);
	CMP_SWAP(4, 6);
	CMP_SWAP(2, 4);
	CMP_SWAP(2, 3); CMP_SWAP(5, 6);
    	break;
  case 8:
    	CMP_SWAP(0, 1); CMP_SWAP(2, 3); CMP_SWAP(4, 5); CMP_SWAP(6, 7);
    	CMP_SWAP(0, 2); CMP_SWAP(4, 6); CMP_SWAP(1, 3); CMP_SWAP(5, 7);
    	CMP_SWAP(1, 2); CMP_SWAP(5, 6); CMP_SWAP(0, 4); CMP_SWAP(1, 5);
    	CMP_SWAP(2, 6); CMP_SWAP(3, 7); CMP_SWAP(2, 4); CMP_SWAP(3, 5);
    	CMP_SWAP(1, 2); CMP_SWAP(3, 4); CMP_SWAP(5, 6);
	break;
  case 7:
	CMP_SWAP(0, 1); CMP_SWAP(2, 3); CMP_SWAP(4, 5);
    	CMP_SWAP(0, 2); CMP_SWAP(4, 6);
	CMP_SWAP(1, 3);
    	CMP_SWAP(1, 2); CMP_SWAP(5, 6);
	CMP_SWAP(0, 4); CMP_SWAP(1, 5); CMP_SWAP(2, 6);
	CMP_SWAP(2, 4); CMP_SWAP(3, 5);
    	CMP_SWAP(1, 2); CMP_SWAP(3, 4); CMP_SWAP(5, 6);
	break;
  case 6:
	CMP_SWAP(0, 1); CMP_SWAP(2, 3); CMP_SWAP(4, 5);
    	CMP_SWAP(0, 2);
	CMP_SWAP(1, 3);
    	CMP_SWAP(1, 2);
	CMP_SWAP(0, 4); CMP_SWAP(1, 5);
    	CMP_SWAP(2, 4); CMP_SWAP(3, 5);
    	CMP_SWAP(1, 2); CMP_SWAP(3, 4);
	break;
  case 5:
	CMP_SWAP(0, 1); CMP_SWAP(2, 3);
    	CMP_SWAP(0, 2);
	CMP_SWAP(1, 3);
    	CMP_SWAP(1, 2);
	CMP_SWAP(0, 4);
    	CMP_SWAP(2, 4);
    	CMP_SWAP(1, 2); CMP_SWAP(3, 4);
	break;
  case 4:
	CMP_SWAP(0, 1); CMP_SWAP(2, 3);
    	CMP_SWAP(0, 2);
	CMP_SWAP(1, 3);
    	CMP_SWAP(1, 2);
	break;
  case 3:
	CMP_SWAP(0, 1);
    	CMP_SWAP(0, 2);
    	CMP_SWAP(1, 2);
	break;
  case 2:
	CMP_SWAP(0, 1);
	break;
  default:
    for (int i = 1; i < n; i++) {
        struct moveType tmp = a[i];
        int j = i;
        for (; j && tmp.weight > a[j - 1].weight ; --j)
            a[j] = a[j - 1];
        a[j] = tmp;
    }
  }

  return;
}



int AdjustMoveList(struct localVarType * thrp) {
  int k, r, n, rank, suit;
  int iniDepth = thrp->iniDepth;

  for (k=1; k<=13; k++) {
    suit=thrp->forbiddenMoves[k].suit;
    rank=thrp->forbiddenMoves[k].rank;
    for (r=0; r<=thrp->movePly[iniDepth].last; r++) {
      if ((suit==thrp->movePly[iniDepth].move[r].suit)&&
        (rank!=0)&&(rank==thrp->movePly[iniDepth].move[r].rank)) {
        /* For the forbidden move r: */
        for (n=r; n<=thrp->movePly[iniDepth].last; n++)
          thrp->movePly[iniDepth].move[n]=
		  thrp->movePly[iniDepth].move[n+1];
        thrp->movePly[iniDepth].last--;
      }
    }
  }
  return thrp->movePly[thrp->iniDepth].last+1;
}


inline bool WinningMove(struct moveType * mvp1, struct moveType * mvp2, int trump) {
/* Return true if move 1 wins over move 2, with the assumption that
move 2 is the presently winning card of the trick */

  if (mvp1->suit==mvp2->suit) {
    if ((mvp1->rank)>(mvp2->rank))
      return true;
    else
      return false;
  }
  else if ((mvp1->suit)==trump)
    return true;
  else
    return false;
}


inline bool WinningMoveNT(struct moveType * mvp1, struct moveType * mvp2) {
/* Return true if move 1 wins over move 2, with the assumption that
move 2 is the presently winning card of the trick */

  if (mvp1->suit==mvp2->suit) {
    if ((mvp1->rank)>(mvp2->rank))
      return true;
    else
      return false;
  }
  else
    return false;
}


