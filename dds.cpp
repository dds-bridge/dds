
/* DDS 2.6.0   A bridge double dummy solver.		      		      */
/* Copyright (C) 2006-2014 by Bo Haglund                                      */
/* Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.        */
/* The code for calculation of par score / contracts is based upon the	      */
/* perl code written by Matthew Kidd for ACBLmerge. He has kindly given me    */
/* permission to include a C++ adaptation in DDS. 	      		      */
/* 									      */
/* The PlayAnalyser analyses the played cards of the deal and presents their  */
/* double dummy values. The par calculation function DealerPar provides an    */
/* alternative way of calculating and presenting par results.      	      */				      
/* Both these functions have been written by Soren Hein.                      */
/* He has also made numerous contributions to the code, especially in the     */
/* initialization part.							      */
/*								              */
/* Licensed under the Apache License, Version 2.0 (the "License");	      */
/* you may not use this file except in compliance with the License.	      */
/* You may obtain a copy of the License at				      */
/* http://www.apache.org/licenses/LICENSE-2.0				      */
/* Unless required by applicable law or agreed to in writing, software	      */
/* distributed under the License is distributed on an "AS IS" BASIS,	      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   */
/* See the License for the specific language governing permissions and	      */
/* limitations under the License.					      */


/*#include "stdafx.h"*/ 

#include "dll.h"
#include "dds.h"
#include "Par.h"

struct localVarType * localVar;

int max_lower;
struct relRanksType * relRanks;
int * counttable;
int * highestRank;
int lho[4];
int rho[4];
int partner[4];
unsigned short int bitMapRank[16];
unsigned char cardRank[15];
unsigned char cardSuit[5];
unsigned char cardHand[4];
void InitStart(int gb_ram, int ncores);

#ifdef STAT
  FILE * fp2;
#endif


int noOfThreads=MAXNOOFTHREADS;  /* The number of entries to the transposition tables. There is
				    one entry per thread. */
int noOfCores;			/* The number of processor cores, however cannot be higher than noOfThreads. */

#ifdef _MANAGED
#pragma managed(push, off)
#endif

void FreeMem(void) {

  if (localVar)
    free(localVar);
  localVar=NULL;

  #ifdef STAT
    fclose(fp2);
  #endif
   
  if (highestRank)
    free(highestRank);
  highestRank=NULL;
  if (counttable)
    free(counttable);
  counttable=NULL;
  if (relRanks)
    free(relRanks);
  relRanks=NULL;

  return;
}


#if defined(_WIN32) || defined(USES_DLLMAIN)

  extern "C" BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD ul_reason_for_call, LPVOID lpReserved) {

  if (ul_reason_for_call==DLL_PROCESS_ATTACH) {
    InitStart(0, 0);
  }
  else if (ul_reason_for_call==DLL_PROCESS_DETACH) {

    FreeMem();

	/*_CrtDumpMemoryLeaks();*/	/* MEMORY LEAK? */
  }
  return 1;
}

#elif defined(USES_CONSTRUCTOR)

void __attribute__ ((constructor)) libInit(void) {

  InitStart(0, 0);

}


void __attribute__ ((destructor)) libEnd(void) {

  FreeMem();
  
}

#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif

  int STDCALL SolveBoard(struct deal dl, int target,
    int solutions, int mode, struct futureTricks *futp, int threadIndex) {

  int k, n, cardCount, found, totalTricks, tricks, last, checkRes;
  int g, upperbound, lowerbound, first, i, j, h, forb, ind, flag, noMoves;
  int noStartMoves;
  int handRelFirst;
  int noOfCardsPerHand[4];
  int latestTrickSuit[4];
  int latestTrickRank[4];
  int maxHand=0, maxSuit=0, maxRank;
  unsigned short int aggrRemain;
  struct movePlyType temp;
  struct moveType mv;
  int hiwinSetSize=0, hinodeSetSize=0;
  int hilenSetSize=0;
  int MaxnodeSetSize=0;
  int MaxwinSetSize=0;
  int MaxlenSetSize=0;
  struct localVarType * thrp = &localVar[threadIndex];

  void InitGame(int gameNo, int moveTreeFlag, int first, int handRelFirst, 
    struct localVarType * thrp);
  void InitSearch(struct pos * posPoint, int depth, struct moveType startMoves[],
    int first, int mtd, struct localVarType * thrp);

  if ((threadIndex<0)||(threadIndex>=noOfThreads)) {	/* Fault corrected after suggestion by Dirk Willecke. */
    DumpInput(RETURN_THREAD_INDEX, dl, target, solutions, mode);
	return RETURN_THREAD_INDEX;
  }

  for (k=0; k<=13; k++) {
    thrp->forbiddenMoves[k].rank=0;
    thrp->forbiddenMoves[k].suit=0;
  }

  if (target<-1) {
    DumpInput(RETURN_TARGET_WRONG_LO, dl, target, solutions, mode);
    return RETURN_TARGET_WRONG_LO;
  }
  if (target>13) {
    DumpInput(RETURN_TARGET_WRONG_HI, dl, target, solutions, mode);
    return RETURN_TARGET_WRONG_HI;
  }
  if (solutions<1) {
    DumpInput(RETURN_SOLNS_WRONG_LO, dl, target, solutions, mode);
    return RETURN_SOLNS_WRONG_LO;
  }
  if (solutions>3) {
    DumpInput(RETURN_SOLNS_WRONG_HI, dl, target, solutions, mode);
    return RETURN_SOLNS_WRONG_HI;
  }

  for (k=0; k<=3; k++)
    noOfCardsPerHand[handId(dl.first, k)]=0;


  for (k=0; k<=2; k++) {
    if (dl.currentTrickRank[k]!=0) {
      noOfCardsPerHand[handId(dl.first, k)]=1;
      aggrRemain=0;
      for (h=0; h<=3; h++)
        aggrRemain|=(dl.remainCards[h][dl.currentTrickSuit[k]]>>2);
      if ((aggrRemain & bitMapRank[dl.currentTrickRank[k]])!=0) {
	DumpInput(RETURN_PLAYED_CARD, dl, target, solutions, mode);
	return RETURN_PLAYED_CARD;
      }
    }
  }

  if (target==-1)
    thrp->tricksTarget=99;
  else
    thrp->tricksTarget=target;

  thrp->newDeal=FALSE; thrp->newTrump=FALSE;
  thrp->diffDeal=0; thrp->aggDeal=0;
  cardCount=0;
  for (i=0; i<=3; i++) {
    for (j=0; j<=3; j++) {
      cardCount+=counttable[dl.remainCards[i][j]>>2];
      thrp->diffDeal+=((dl.remainCards[i][j]>>2)^
	      (thrp->game.suit[i][j]));
      thrp->aggDeal+=(dl.remainCards[i][j]>>2);
      if (thrp->game.suit[i][j]!=dl.remainCards[i][j]>>2) {
        thrp->game.suit[i][j]=dl.remainCards[i][j]>>2;
	thrp->newDeal=TRUE;
      }
    }
  }

  if (thrp->newDeal) {
    if (thrp->diffDeal==0)
      thrp->similarDeal=TRUE;
    else if ((thrp->aggDeal/thrp->diffDeal)
       > SIMILARDEALLIMIT)
      thrp->similarDeal=TRUE;
    else
      thrp->similarDeal=FALSE;
  }
  else
    thrp->similarDeal=FALSE;

  if (dl.trump!=thrp->trump)
    thrp->newTrump=TRUE;

  for (i=0; i<=3; i++)
    for (j=0; j<=3; j++)
      noOfCardsPerHand[i]+=counttable[thrp->game.suit[i][j]];

  for (i=1; i<=3; i++) {
    if (noOfCardsPerHand[i]!=noOfCardsPerHand[0]) {
      DumpInput(RETURN_CARD_COUNT, dl, target, solutions, mode);
      return RETURN_CARD_COUNT;
    }
  }

  if (dl.currentTrickRank[2]) {
    if ((dl.currentTrickRank[2]<2)||(dl.currentTrickRank[2]>14)
      ||(dl.currentTrickSuit[2]<0)||(dl.currentTrickSuit[2]>3)) {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }
    thrp->handToPlay=handId(dl.first, 3);
    handRelFirst=3;
    noStartMoves=3;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[thrp->handToPlay][k]!=0) {
          latestTrickSuit[thrp->handToPlay]=k;
          latestTrickRank[thrp->handToPlay]=
            InvBitMapRank(thrp->game.suit[thrp->handToPlay][k]);
          break;
        }
      }
      latestTrickSuit[handId(dl.first, 2)]=dl.currentTrickSuit[2];
      latestTrickRank[handId(dl.first, 2)]=dl.currentTrickRank[2];
      latestTrickSuit[handId(dl.first, 1)]=dl.currentTrickSuit[1];
      latestTrickRank[handId(dl.first, 1)]=dl.currentTrickRank[1];
      latestTrickSuit[dl.first]=dl.currentTrickSuit[0];
      latestTrickRank[dl.first]=dl.currentTrickRank[0];
    }
  }
  else if (dl.currentTrickRank[1]) {
    if ((dl.currentTrickRank[1]<2)||(dl.currentTrickRank[1]>14)
      ||(dl.currentTrickSuit[1]<0)||(dl.currentTrickSuit[1]>3)) {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }
    thrp->handToPlay=handId(dl.first, 2);
    handRelFirst=2;
    noStartMoves=2;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[thrp->handToPlay][k]!=0) {
          latestTrickSuit[thrp->handToPlay]=k;
          latestTrickRank[thrp->handToPlay]=
            InvBitMapRank(thrp->game.suit[thrp->handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
	if (thrp->game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(thrp->game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      latestTrickSuit[handId(dl.first, 1)]=dl.currentTrickSuit[1];
      latestTrickRank[handId(dl.first, 1)]=dl.currentTrickRank[1];
      latestTrickSuit[dl.first]=dl.currentTrickSuit[0];
      latestTrickRank[dl.first]=dl.currentTrickRank[0];
    }
  }
  else if (dl.currentTrickRank[0]) {
    if ((dl.currentTrickRank[0]<2)||(dl.currentTrickRank[0]>14)
      ||(dl.currentTrickSuit[0]<0)||(dl.currentTrickSuit[0]>3)) {
      DumpInput(RETURN_SUIT_OR_RANK, dl, target, solutions, mode);
      return RETURN_SUIT_OR_RANK;
    }
    thrp->handToPlay=handId(dl.first,1);
    handRelFirst=1;
    noStartMoves=1;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[thrp->handToPlay][k]!=0) {
          latestTrickSuit[thrp->handToPlay]=k;
          latestTrickRank[thrp->handToPlay]=
            InvBitMapRank(thrp->game.suit[thrp->handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
	if (thrp->game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(thrp->game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[handId(dl.first, 2)][k]!=0) {
          latestTrickSuit[handId(dl.first, 2)]=k;
          latestTrickRank[handId(dl.first, 2)]=
            InvBitMapRank(thrp->game.suit[handId(dl.first, 2)][k]);
          break;
        }
      }
      latestTrickSuit[dl.first]=dl.currentTrickSuit[0];
      latestTrickRank[dl.first]=dl.currentTrickRank[0];
    }
  }
  else {
    thrp->handToPlay=dl.first;
    handRelFirst=0;
    noStartMoves=0;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[thrp->handToPlay][k]!=0) {
          latestTrickSuit[thrp->handToPlay]=k;
          latestTrickRank[thrp->handToPlay]=
            InvBitMapRank(thrp->game.suit[thrp->handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(thrp->game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[handId(dl.first, 2)][k]!=0) {
          latestTrickSuit[handId(dl.first, 2)]=k;
          latestTrickRank[handId(dl.first, 2)]=
            InvBitMapRank(thrp->game.suit[handId(dl.first, 2)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (thrp->game.suit[handId(dl.first, 1)][k]!=0) {
          latestTrickSuit[handId(dl.first, 1)]=k;
          latestTrickRank[handId(dl.first, 1)]=
            InvBitMapRank(thrp->game.suit[handId(dl.first, 1)][k]);
          break;
        }
      }
    }
  }

  thrp->trump=dl.trump;
  thrp->game.first=dl.first;
  first=dl.first;
  thrp->game.noOfCards=cardCount;
  if (dl.currentTrickRank[0]!=0) {
    thrp->game.leadHand=dl.first;
    thrp->game.leadSuit=dl.currentTrickSuit[0];
    thrp->game.leadRank=dl.currentTrickRank[0];
  }
  else {
    thrp->game.leadHand=0;
    thrp->game.leadSuit=0;
    thrp->game.leadRank=0;
  }

  for (k=0; k<=2; k++) {
    thrp->initialMoves[k].suit=255;
    thrp->initialMoves[k].rank=255;
  }

  for (k=0; k<noStartMoves; k++) {
    thrp->initialMoves[noStartMoves-1-k].suit=dl.currentTrickSuit[k];
    thrp->initialMoves[noStartMoves-1-k].rank=dl.currentTrickRank[k];
  }

  if (cardCount % 4)
    totalTricks=((cardCount-4)>>2)+2;
  else
    totalTricks=((cardCount-4)>>2)+1;
  checkRes=CheckDeal(&localVar[threadIndex].cd, threadIndex);
  if (thrp->game.noOfCards<=0) {
    DumpInput(RETURN_ZERO_CARDS, dl, target, solutions, mode);
    return RETURN_ZERO_CARDS;
  }
  if (thrp->game.noOfCards>52) {
    DumpInput(RETURN_TOO_MANY_CARDS, dl, target, solutions, mode);
    return RETURN_TOO_MANY_CARDS;
  }
  if (totalTricks<target) {
    DumpInput(RETURN_TARGET_TOO_HIGH, dl, target, solutions, mode);
    return RETURN_TARGET_TOO_HIGH;
  }
  if (checkRes) {
    DumpInput(RETURN_DUPLICATE_CARDS, dl, target, solutions, mode);
    return RETURN_DUPLICATE_CARDS;
  }

  if (cardCount<=4) {
    maxRank=0;
    /* Highest trump? */
    if (dl.trump!=4) {
      for (k=0; k<=3; k++) {
        if ((latestTrickSuit[k]==dl.trump)&&
          (latestTrickRank[k]>maxRank)) {
          maxRank=latestTrickRank[k];
          maxSuit=dl.trump;
          maxHand=k;
        }
      }
    }
    /* Highest card in leading suit */
    if (maxRank==0) {
      for (k=0; k<=3; k++) {
        if (k==dl.first) {
          maxSuit=latestTrickSuit[dl.first];
          maxHand=dl.first;
          maxRank=latestTrickRank[dl.first];
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if ((k!=dl.first)&&(latestTrickSuit[k]==maxSuit)&&
          (latestTrickRank[k]>maxRank)) {
          maxHand=k;
          maxRank=latestTrickRank[k];
        }
      }
    }
    futp->nodes=0;
    futp->cards=1;
    futp->suit[0]=latestTrickSuit[thrp->handToPlay];
    futp->rank[0]=latestTrickRank[thrp->handToPlay];
    futp->equals[0]=0;
    if ((target==0)&&(solutions<3))
      futp->score[0]=0;
    else if ((thrp->handToPlay==maxHand)||
	(partner[thrp->handToPlay]==maxHand))
      futp->score[0]=1;
    else
      futp->score[0]=0;

	/*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
    return RETURN_NO_FAULT;
  }

  if ((mode!=2)&&
    (((thrp->newDeal)&&(!(thrp->similarDeal)))
      || thrp->newTrump  ||
	  (thrp->winSetSize > SIMILARMAXWINNODES))) {

    Wipe(&localVar[threadIndex]);
    thrp->winSetSizeLimit=WINIT;
    thrp->nodeSetSizeLimit=NINIT;
    thrp->lenSetSizeLimit=LINIT;
    thrp->allocmem=(WINIT+1)*sizeof(struct winCardType);
    thrp->allocmem+=(NINIT+1)*sizeof(struct nodeCardsType);
    thrp->allocmem+=(LINIT+1)*sizeof(struct posSearchType);
    thrp->winCards=thrp->pw[0];
    thrp->nodeCards=thrp->pn[0];
    thrp->posSearch=thrp->pl[0];
    thrp->wcount=0; thrp->ncount=0; thrp->lcount=0;
    InitGame(0, FALSE, first, handRelFirst, thrp);
  }
  else {
    InitGame(0, TRUE, first, handRelFirst, thrp);
  }

  #ifdef STAT
  thrp->nodes=0;
  #endif 
  thrp->trickNodes=0;
  thrp->iniDepth=cardCount-4;
  hiwinSetSize=0;
  hinodeSetSize=0;

  if (mode==0) {
    MoveGen(&(thrp->lookAheadPos), thrp->iniDepth, thrp->trump,
		&(thrp->movePly[thrp->iniDepth]), thrp);
    if (thrp->movePly[thrp->iniDepth].last==0) {
	futp->nodes=0;
	futp->cards=1;
	futp->suit[0]=thrp->movePly[thrp->iniDepth].move[0].suit;
	futp->rank[0]=thrp->movePly[thrp->iniDepth].move[0].rank;
	futp->equals[0]=thrp->movePly[thrp->iniDepth].move[0].sequence<<2;
	futp->score[0]=-2;

	/*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
	return RETURN_NO_FAULT;
    }
  }
  if ((target==0)&&(solutions<3)) {
    MoveGen(&(thrp->lookAheadPos), thrp->iniDepth, thrp->trump,
		&(thrp->movePly[thrp->iniDepth]), thrp);
    futp->nodes=0;
    for (k=0; k<=thrp->movePly[thrp->iniDepth].last; k++) {
	futp->suit[k]=thrp->movePly[thrp->iniDepth].move[k].suit;
	futp->rank[k]=thrp->movePly[thrp->iniDepth].move[k].rank;
	futp->equals[k]=thrp->movePly[thrp->iniDepth].move[k].sequence<<2;
	futp->score[k]=0;
    }
    if (solutions==1)
	futp->cards=1;
    else
	futp->cards=thrp->movePly[thrp->iniDepth].last+1;

	/*_CrtDumpMemoryLeaks(); */ /* MEMORY LEAK? */
    return RETURN_NO_FAULT;
  }

  if ((target!=-1)&&(solutions!=3)) {
    thrp->val=ABsearch(&(thrp->lookAheadPos),
		thrp->tricksTarget, thrp->iniDepth, thrp);

    temp=thrp->movePly[thrp->iniDepth];
    last=thrp->movePly[thrp->iniDepth].last;
    noMoves=last+1;
    hiwinSetSize=thrp->winSetSize;
    hinodeSetSize=thrp->nodeSetSize;
    hilenSetSize=thrp->lenSetSize;
    if (thrp->nodeSetSize>MaxnodeSetSize)
      MaxnodeSetSize=thrp->nodeSetSize;
    if (thrp->winSetSize>MaxwinSetSize)
      MaxwinSetSize=thrp->winSetSize;
    if (thrp->lenSetSize>MaxlenSetSize)
      MaxlenSetSize=thrp->lenSetSize;
    if (thrp->val==1)
      thrp->payOff=thrp->tricksTarget;
    else
      thrp->payOff=0;
    futp->cards=1;
    ind=2;

    if (thrp->payOff<=0) {
      futp->suit[0]=thrp->movePly[thrp->game.noOfCards-4].move[0].suit;
      futp->rank[0]=thrp->movePly[thrp->game.noOfCards-4].move[0].rank;
	futp->equals[0]=(thrp->movePly[thrp->game.noOfCards-4].move[0].sequence)<<2;
      if (thrp->tricksTarget>1)
        futp->score[0]=-1;
      else
	futp->score[0]=0;
    }
    else {
      futp->suit[0]=thrp->bestMove[thrp->game.noOfCards-4].suit;
      futp->rank[0]=thrp->bestMove[thrp->game.noOfCards-4].rank;
	futp->equals[0]=(thrp->bestMove[thrp->game.noOfCards-4].sequence)<<2;
      futp->score[0]=thrp->payOff;
    }
  }
  else {
    g=thrp->estTricks[thrp->handToPlay];
    upperbound=13;
    lowerbound=0;
    do {
      if (g==lowerbound)
        tricks=g+1;
      else
        tricks=g;
	  assert((thrp->lookAheadPos.handRelFirst>=0)&&
		(thrp->lookAheadPos.handRelFirst<=3));
      thrp->val=ABsearch(&(thrp->lookAheadPos), tricks, thrp->iniDepth, thrp);

      if (thrp->val==TRUE)
        mv=thrp->bestMove[thrp->game.noOfCards-4];
      hiwinSetSize=Max(hiwinSetSize, thrp->winSetSize);
      hinodeSetSize=Max(hinodeSetSize, thrp->nodeSetSize);
	hilenSetSize=Max(hilenSetSize, thrp->lenSetSize);
      if (thrp->nodeSetSize>MaxnodeSetSize)
        MaxnodeSetSize=thrp->nodeSetSize;
      if (thrp->winSetSize>MaxwinSetSize)
        MaxwinSetSize=thrp->winSetSize;
	if (thrp->lenSetSize>MaxlenSetSize)
        MaxlenSetSize=thrp->lenSetSize;
      if (thrp->val==FALSE) {
	upperbound=tricks-1;
	g=upperbound;
      }
      else {
        lowerbound=tricks;
        g=lowerbound;
      }
      InitSearch(&(thrp->iniPosition), thrp->game.noOfCards-4,
        thrp->initialMoves, first, TRUE, thrp);
    }
    while (lowerbound<upperbound);
    thrp->payOff=g;
    temp=thrp->movePly[thrp->iniDepth];
    last=thrp->movePly[thrp->iniDepth].last;
    noMoves=last+1;
    ind=2;
    thrp->bestMove[thrp->game.noOfCards-4]=mv;
    futp->cards=1;
    if (thrp->payOff<=0) {
      futp->score[0]=0;
      futp->suit[0]=thrp->movePly[thrp->game.noOfCards-4].move[0].suit;
      futp->rank[0]=thrp->movePly[thrp->game.noOfCards-4].move[0].rank;
      futp->equals[0]=(thrp->movePly[thrp->game.noOfCards-4].move[0].sequence)<<2;
    }
    else {
      futp->score[0]=thrp->payOff;
      futp->suit[0]=thrp->bestMove[thrp->game.noOfCards-4].suit;
      futp->rank[0]=thrp->bestMove[thrp->game.noOfCards-4].rank;
      futp->equals[0]=(thrp->bestMove[thrp->game.noOfCards-4].sequence)<<2;
    }
    thrp->tricksTarget=thrp->payOff;
  }

  if ((solutions==2)&&(thrp->payOff>0)) {
    forb=1;
    ind=forb;
    while ((thrp->payOff==thrp->tricksTarget)&&(ind<(temp.last+1))) {
      thrp->forbiddenMoves[forb].suit=thrp->bestMove[thrp->game.noOfCards-4].suit;
      thrp->forbiddenMoves[forb].rank=thrp->bestMove[thrp->game.noOfCards-4].rank;
      forb++; ind++;
      /* All moves before bestMove in the move list shall be
      moved to the forbidden moves list, since none of them reached
      the target */
      for (k=0; k<=thrp->movePly[thrp->iniDepth].last; k++)
        if ((thrp->bestMove[thrp->iniDepth].suit==
			thrp->movePly[thrp->iniDepth].move[k].suit)
          &&(thrp->bestMove[thrp->iniDepth].rank==
		    thrp->movePly[thrp->iniDepth].move[k].rank))
          break;
      for (i=0; i<k; i++) {  /* All moves until best move */
        flag=FALSE;
        for (j=0; j<forb; j++) {
          if ((thrp->movePly[thrp->iniDepth].move[i].suit==thrp->forbiddenMoves[j].suit)
            &&(thrp->movePly[thrp->iniDepth].move[i].rank==thrp->forbiddenMoves[j].rank)) {
            /* If the move is already in the forbidden list */
            flag=TRUE;
            break;
          }
        }
        if (!flag) {
          thrp->forbiddenMoves[forb]=thrp->movePly[thrp->iniDepth].move[i];
          forb++;
        }
      }
      InitSearch(&(thrp->iniPosition), thrp->game.noOfCards-4,
          thrp->initialMoves, first, TRUE, thrp);
      thrp->val=ABsearch(&(thrp->lookAheadPos), thrp->tricksTarget, 
	thrp->iniDepth, thrp);

      hiwinSetSize=thrp->winSetSize;
      hinodeSetSize=thrp->nodeSetSize;
      hilenSetSize=thrp->lenSetSize;
      if (thrp->nodeSetSize>MaxnodeSetSize)
        MaxnodeSetSize=thrp->nodeSetSize;
      if (thrp->winSetSize>MaxwinSetSize)
        MaxwinSetSize=localVar[threadIndex].winSetSize;
      if (thrp->lenSetSize>MaxlenSetSize)
        MaxlenSetSize=localVar[threadIndex].lenSetSize;
      if (thrp->val==TRUE) {
        thrp->payOff=thrp->tricksTarget;
        futp->cards=ind;
        futp->suit[ind-1]=thrp->bestMove[thrp->game.noOfCards-4].suit;
        futp->rank[ind-1]=thrp->bestMove[thrp->game.noOfCards-4].rank;
	futp->equals[ind-1]=(thrp->bestMove[thrp->game.noOfCards-4].sequence)<<2;
        futp->score[ind-1]=thrp->payOff;
      }
      else
        thrp->payOff=0;
    }
  }
  else if ((solutions==2)&&(thrp->payOff==0)&&
	((target==-1)||(thrp->tricksTarget==1))) {
    futp->cards=noMoves;
    /* Find the cards that were in the initial move list
    but have not been listed in the current result */
    n=0;
    for (i=0; i<noMoves; i++) {
      found=FALSE;
      if ((temp.move[i].suit==futp->suit[0])&&
        (temp.move[i].rank==futp->rank[0])) {
        found=TRUE;
      }
      if (!found) {
        futp->suit[1+n]=temp.move[i].suit;
        futp->rank[1+n]=temp.move[i].rank;
	  futp->equals[1+n]=(temp.move[i].sequence)<<2;
        futp->score[1+n]=0;
        n++;
      }
    }
  }

  if ((solutions==3)&&(thrp->payOff>0)) {
    forb=1;
    ind=forb;
    for (i=0; i<last; i++) {
      thrp->forbiddenMoves[forb].suit=thrp->bestMove[thrp->game.noOfCards-4].suit;
      thrp->forbiddenMoves[forb].rank=thrp->bestMove[thrp->game.noOfCards-4].rank;
      forb++; ind++;

      g=thrp->payOff;
      upperbound=thrp->payOff;
      lowerbound=0;

      InitSearch(&(thrp->iniPosition), thrp->game.noOfCards-4,
          thrp->initialMoves, first, TRUE, thrp);
      do {
        if (g==lowerbound)
          tricks=g+1;
        else
          tricks=g;
	assert((thrp->lookAheadPos.handRelFirst>=0)&&
		  (thrp->lookAheadPos.handRelFirst<=3));
        thrp->val=ABsearch(&(thrp->lookAheadPos), tricks, thrp->iniDepth, thrp);

        if (thrp->val==TRUE)
          mv=thrp->bestMove[thrp->game.noOfCards-4];
        hiwinSetSize=Max(hiwinSetSize, thrp->winSetSize);
        hinodeSetSize=Max(hinodeSetSize, thrp->nodeSetSize);
	hilenSetSize=Max(hilenSetSize, thrp->lenSetSize);
        if (thrp->nodeSetSize>MaxnodeSetSize)
          MaxnodeSetSize=thrp->nodeSetSize;
        if (thrp->winSetSize>MaxwinSetSize)
          MaxwinSetSize=thrp->winSetSize;
	if (thrp->lenSetSize>MaxlenSetSize)
          MaxlenSetSize=thrp->lenSetSize;
        if (thrp->val==FALSE) {
	  upperbound=tricks-1;
	  g=upperbound;
	}
	else {
	  lowerbound=tricks;
	  g=lowerbound;
	}

        InitSearch(&(thrp->iniPosition), thrp->game.noOfCards-4,
          thrp->initialMoves, first, TRUE, thrp);
      }
      while (lowerbound<upperbound);
      thrp->payOff=g;
      if (thrp->payOff==0) {
        last=thrp->movePly[thrp->iniDepth].last;
        futp->cards=temp.last+1;
        for (j=0; j<=last; j++) {
          futp->suit[ind-1+j]=thrp->movePly[thrp->game.noOfCards-4].move[j].suit;
          futp->rank[ind-1+j]=thrp->movePly[thrp->game.noOfCards-4].move[j].rank;
	    futp->equals[ind-1+j]=(thrp->movePly[thrp->game.noOfCards-4].move[j].sequence)<<2;
          futp->score[ind-1+j]=thrp->payOff;
        }
        break;
      }
      else {
        thrp->bestMove[thrp->game.noOfCards-4]=mv;

        futp->cards=ind;
        futp->suit[ind-1]=thrp->bestMove[thrp->game.noOfCards-4].suit;
        futp->rank[ind-1]=thrp->bestMove[thrp->game.noOfCards-4].rank;
	  futp->equals[ind-1]=(thrp->bestMove[thrp->game.noOfCards-4].sequence)<<2;
        futp->score[ind-1]=thrp->payOff;
      }
    }
  }
  else if ((solutions==3)&&(thrp->payOff==0)) {
    futp->cards=noMoves;
    /* Find the cards that were in the initial move list
    but have not been listed in the current result */
    n=0;
    for (i=0; i<noMoves; i++) {
      found=FALSE;
      if ((temp.move[i].suit==futp->suit[0])&&
        (temp.move[i].rank==futp->rank[0])) {
          found=TRUE;
      }
      if (!found) {
        futp->suit[1+n]=temp.move[i].suit;
        futp->rank[1+n]=temp.move[i].rank;
	  futp->equals[1+n]=(temp.move[i].sequence)<<2;
        futp->score[1+n]=0;
        n++;
      }
    }
  }

  for (k=0; k<=13; k++) {
    thrp->forbiddenMoves[k].suit=0;
    thrp->forbiddenMoves[k].rank=0;
  }

  futp->nodes=thrp->trickNodes;

  /*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */

  return RETURN_NO_FAULT;
}


int _initialized=0;

void InitStart(int gb_ram, int ncores) {
  int k, r, i, j, m, ind;
  unsigned short int res;
  unsigned int topBitRank = 1;
  unsigned long long pcmem;	/* kbytes */

  if (_initialized)
    return;
  _initialized = 1;

  if (gb_ram==0) {		/* Autoconfig */

  #ifdef _WIN32
    
    SYSTEM_INFO temp;

    MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);	/* Using GlobalMemoryStatusEx instead of GlobalMemoryStatus
					was suggested by Lorne Anderson. */

    pcmem=(unsigned long long)statex.ullTotalPhys/1024;

    #ifdef DDS_THREADS_SINGLE

    noOfThreads = 1;

    noOfCores = 1;

    #else

    if (pcmem < 1500000)
      noOfThreads=Min(MAXNOOFTHREADS, 2);
    else if (pcmem < 2500000)
      noOfThreads=Min(MAXNOOFTHREADS, 4);
    else
      noOfThreads=MAXNOOFTHREADS;

    GetSystemInfo(&temp);
    noOfCores=Min(noOfThreads, (int)temp.dwNumberOfProcessors);

    #endif

  #endif
  #ifdef __linux__   /* The code for linux was suggested by Antony Lee. */

    #ifdef DDS_THREADS_SINGLE

    noOfThreads = 1;

    noOfCores = 1;

    #else

    ncores = sysconf(_SC_NPROCESSORS_ONLN); 
    FILE* fifo = popen("free -k | tail -n+3 | head -n1 | awk '{print $NF}'", "r");
    fscanf(fifo, "%lld", &pcmem);
    fclose(fifo);

    #endif
  #endif

  }
  else {
    #ifdef DDS_THREADS_SINGLE 

    noOfThreads = 1;

    noOfCores = 1;

    #else

    if (gb_ram < 2)
      noOfThreads=Min(MAXNOOFTHREADS, 2);
    else if (gb_ram < 3)
      noOfThreads=Min(MAXNOOFTHREADS, 4);
    else
      noOfThreads=Min(MAXNOOFTHREADS, 8);

    noOfCores=Min(noOfThreads, ncores);

    #endif

    pcmem=(unsigned long long)(1000000 * gb_ram);
  }

  /*printf("noOfThreads: %d   noOfCores: %d\n", noOfThreads, noOfCores);*/

  localVar = (struct localVarType *)calloc(noOfThreads, sizeof(struct localVarType));

  for (k=0; k<noOfThreads; k++) {
    localVar[k].trump=-1;
    localVar[k].nodeSetSizeLimit=0;
    localVar[k].winSetSizeLimit=0;
    localVar[k].lenSetSizeLimit=0;
    localVar[k].clearTTflag=FALSE;
    localVar[k].windex=-1;

    localVar[k].nodeSetSize=0; /* Index with range 0 to nodeSetSizeLimit */
    localVar[k].winSetSize=0;  /* Index with range 0 to winSetSizeLimit */
    localVar[k].lenSetSize=0;  /* Index with range 0 to lenSetSizeLimit */

    localVar[k].nodeSetSizeLimit=NINIT;
    localVar[k].winSetSizeLimit=WINIT;
    localVar[k].lenSetSizeLimit=LINIT;

    if ((gb_ram!=0)&&(ncores!=0))
      localVar[k].maxmem=gb_ram * ((8000001*sizeof(struct nodeCardsType)+
		   25000001*sizeof(struct winCardType)+
		   400001*sizeof(struct posSearchType))/noOfThreads);
    else {
      localVar[k].maxmem = (unsigned long long)(pcmem-32768) * (700/noOfThreads);
	  /* Linear calculation of maximum memory, formula by Michiel de Bondt */

      localVar[k].maxmem = Min(localVar[k].maxmem, TT_MAXMEM);

      if (localVar[k].maxmem < 10485760) exit (1);
    }

    /*printf("thread no: %d  maxmem: %ld\n", k, localVar[k].maxmem);*/
  }

  bitMapRank[15]=0x2000;
  bitMapRank[14]=0x1000;
  bitMapRank[13]=0x0800;
  bitMapRank[12]=0x0400;
  bitMapRank[11]=0x0200;
  bitMapRank[10]=0x0100;
  bitMapRank[9]=0x0080;
  bitMapRank[8]=0x0040;
  bitMapRank[7]=0x0020;
  bitMapRank[6]=0x0010;
  bitMapRank[5]=0x0008;
  bitMapRank[4]=0x0004;
  bitMapRank[3]=0x0002;
  bitMapRank[2]=0x0001;
  bitMapRank[1]=0;
  bitMapRank[0]=0;

  lho[0]=1; lho[1]=2; lho[2]=3; lho[3]=0;
  rho[0]=3; rho[1]=0; rho[2]=1; rho[3]=2;
  partner[0]=2; partner[1]=3; partner[2]=0; partner[3]=1;

  cardRank[2]='2'; cardRank[3]='3'; cardRank[4]='4'; cardRank[5]='5';
  cardRank[6]='6'; cardRank[7]='7'; cardRank[8]='8'; cardRank[9]='9';
  cardRank[10]='T'; cardRank[11]='J'; cardRank[12]='Q'; cardRank[13]='K';
  cardRank[14]='A';

  cardSuit[0]='S'; cardSuit[1]='H'; cardSuit[2]='D'; cardSuit[3]='C';
  cardSuit[4]='N';

  cardHand[0]='N'; cardHand[1]='E'; cardHand[2]='S'; cardHand[3]='W';

  for (k=0; k<noOfThreads; k++) {
    localVar[k].summem=(WINIT+1)*sizeof(struct winCardType)+
	     (NINIT+1)*sizeof(struct nodeCardsType)+
		 (LINIT+1)*sizeof(struct posSearchType);
    localVar[k].wmem=(WSIZE+1)*sizeof(struct winCardType);
    localVar[k].nmem=(NSIZE+1)*sizeof(struct nodeCardsType);
    localVar[k].lmem=(LSIZE+1)*sizeof(struct posSearchType);
	localVar[k].maxIndex=(int)(
       (localVar[k].maxmem-localVar[k].summem)/((WSIZE+1) * sizeof(struct winCardType)));

    localVar[k].pw = (struct winCardType **)calloc(localVar[k].maxIndex+1, sizeof(struct winCardType *));
    if (localVar[k].pw==NULL)
      exit(1);
    localVar[k].pn = (struct nodeCardsType **)calloc(localVar[k].maxIndex+1, sizeof(struct nodeCardsType *));
    if (localVar[k].pn==NULL)
      exit(1);
    localVar[k].pl = (struct posSearchType **)calloc(localVar[k].maxIndex+1, sizeof(struct posSearchType *));
    if (localVar[k].pl==NULL)
      exit(1);
    for (i=0; i<=localVar[k].maxIndex; i++) {
      if (localVar[k].pw[i])
	free(localVar[k].pw[i]);
      localVar[k].pw[i]=NULL;
    }
    for (i=0; i<=localVar[k].maxIndex; i++) {
      if (localVar[k].pn[i])
	free(localVar[k].pn[i]);
      localVar[k].pn[i]=NULL;
    }
    for (i=0; i<=localVar[k].maxIndex; i++) {
      if (localVar[k].pl[i])
	free(localVar[k].pl[i]);
      localVar[k].pl[i]=NULL;
    }

    localVar[k].pw[0] = (struct winCardType *)calloc(localVar[k].winSetSizeLimit+1, sizeof(struct winCardType));
    if (localVar[k].pw[0]==NULL)
      exit(1);
    localVar[k].allocmem=(localVar[k].winSetSizeLimit+1)*sizeof(struct winCardType);
    localVar[k].winCards=localVar[k].pw[0];
    localVar[k].pn[0] = (struct nodeCardsType *)calloc(localVar[k].nodeSetSizeLimit+1, sizeof(struct nodeCardsType));
    if (localVar[k].pn[0]==NULL)
      exit(1);
    localVar[k].allocmem+=(localVar[k].nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
    localVar[k].nodeCards=localVar[k].pn[0];
    localVar[k].pl[0] =
	  (struct posSearchType *)calloc(localVar[k].lenSetSizeLimit+1, sizeof(struct posSearchType));
    if (localVar[k].pl[0]==NULL)
     exit(1);
    localVar[k].allocmem+=(localVar[k].lenSetSizeLimit+1)*sizeof(struct posSearchType);
    localVar[k].posSearch=localVar[k].pl[0];
    localVar[k].wcount=0; localVar[k].ncount=0; localVar[k].lcount=0;


    localVar[k].abs = (struct absRanksType *)calloc(8192, sizeof(struct absRanksType));
    if (localVar[k].abs==NULL)
      exit(1);

    localVar[k].adaptWins = (struct adaptWinRanksType *)calloc(8192,
	  sizeof(struct adaptWinRanksType));
    if (localVar[k].adaptWins==NULL)
      exit(1);
  }

  #ifdef STAT
    fp2=fopen("stat.txt","w");
  #endif

  highestRank = (int *)calloc(8192, sizeof(int));
  if (highestRank==NULL)
    exit(1);

  highestRank[0]=0;
  for (k=1; k<8192; k++) {
    for (r=14; r>=2; r--) {
      if ((k & bitMapRank[r])!=0) {
	highestRank[k]=r;
	  break;
      }
    }
  }

  relRanks = (relRanksType *)calloc(1, sizeof(relRanksType));

  /* The use of the counttable to give the number of bits set to
  one in an integer follows an implementation by Thomas Andrews. */

  counttable = (int *)calloc(8192, sizeof(int));
  if (counttable==NULL)
    exit(1);

  for (i=0; i<8192; i++) {
    counttable[i]=0;
    for (j=0; j<13; j++) {
      if (i & (1<<j)) {counttable[i]++;}
    }
  }


  for (i=0; i<8192; i++)
    for (j=0; j<14; j++) {
      res=0;
      if (j==0) {
	for (m=0; m<noOfThreads; m++)
	  localVar[m].adaptWins[i].winRanks[j]=0;
      }
      else {
	k=1;
	for (r=14; r>=2; r--) {
	  if ((i & bitMapRank[r])!=0) {
	    if (k <= j) {
	      res|=bitMapRank[r];
	      k++;
	    }
	    else
	      break;
	  }
	}
	for (m=0; m<noOfThreads; m++)
	  localVar[m].adaptWins[i].winRanks[j]=res;
      }
    }

  /* Init rel */

  /* Should not be necessary, as the memory was calloc'ed */ 

  for (r = 2; r <= 14; r++)  
    relRanks->relRank[0][r] = 0;

  for (ind = 1; ind < 8192; ind++) {   
    if (ind >= (topBitRank + topBitRank)) /* Next top bit */ 
      topBitRank <<= 1;
      
    for (r = 0; r <= 14; r++)  
      relRanks->relRank[ind][r] = relRanks->relRank[ind ^ topBitRank][r];
      
    int ord = 0;  
    for (r = 14; r >= 2; r--) {
   
      if ((ind & bitMapRank[r]) != 0) {
        ord++;	  
        relRanks->relRank[ind][r] = ord;
      }
    }
  }
  

  return;
}


void InitGame(int gameNo, int moveTreeFlag, int first, int handRelFirst, 
  struct localVarType * thrp) {

  int k, s, h, m, ord, r, ind;
  unsigned int topBitRank=1;
  unsigned int topBitNo=2;
  void InitSearch(struct pos * posPoint, int depth, 
    struct moveType startMoves[], int first, int mtd, 
    struct localVarType * thrp);

  if (thrp->newDeal) {

    /* Initialization of the abs structure is implemented
       according to a solution given by Thomas Andrews */

    for (k=0; k<=3; k++)
      for (m=0; m<=3; m++)
        thrp->iniPosition.rankInSuit[k][m]=thrp->game.suit[k][m];

    for (s=0; s<4; s++) {
      thrp->abs[0].aggrRanks[s]=0;
      thrp->abs[0].winMask[s]=0;
      for (ord=1; ord<=13; ord++) {
	thrp->abs[0].absRank[ord][s].hand=-1;
	thrp->abs[0].absRank[ord][s].rank=0;
      }
    }

    int hand_lookup[4][15];

    /* credit Soren Hein */

    for (s = 0; s < 4; s++) { 
      for (r = 14; r >= 2; r--) {     
        hand_lookup[s][r] = -1;
        for (h = 0; h < 4; h++) {  
          if ((thrp->game.suit[h][s] & bitMapRank[r]) != 0) {
            hand_lookup[s][r] = h; 
            break;
          }
        }
      }
    }

    struct absRanksType * absp;

    for (ind=1; ind<8192; ind++) {
      if (ind>=(topBitRank+topBitRank)) {
       /* Next top bit */
        topBitRank <<=1;
        topBitNo++;
      }

      thrp->abs[ind] = thrp->abs[ind ^ topBitRank];      
      absp = &thrp->abs[ind];
      int weight = counttable[ind];
      for (int c = weight; c >= 2; c--) { 
        for (int s = 0; s < 4; s++) {    
          absp->absRank[c][s].hand = absp->absRank[c-1][s].hand;
          absp->absRank[c][s].rank = absp->absRank[c-1][s].rank;
        }
      }
      

      for (s = 0; s < 4; s++) {
        absp->absRank[1][s].hand = hand_lookup[s][topBitNo];
        absp->absRank[1][s].rank = topBitNo;
        absp->aggrRanks[s] = (absp->aggrRanks[s] >> 2) | 
	                     (hand_lookup[s][topBitNo] << 24);
	absp->winMask[s]   = (absp->winMask[s]   >> 2) | (3 << 24);
      }
    }
  }


  thrp->iniPosition.first[thrp->game.noOfCards-4]=first;
  thrp->iniPosition.handRelFirst=handRelFirst;
  thrp->lookAheadPos=thrp->iniPosition;

  thrp->estTricks[1]=6;
  thrp->estTricks[3]=6;
  thrp->estTricks[0]=7;
  thrp->estTricks[2]=7;

  #ifdef STAT
  fprintf(fp2, "Estimated tricks for hand to play:\n");
  fprintf(fp2, "hand=%d  est tricks=%d\n",
	  thrp->handToPlay, thrp->estTricks[thrp->handToPlay]);
  #endif

  InitSearch(&(thrp->lookAheadPos), thrp->game.noOfCards-4,
	thrp->initialMoves, first, moveTreeFlag, thrp);
  return;
}


void InitSearch(struct pos * posPoint, int depth, struct moveType startMoves[],
	int first, int mtd, struct localVarType * thrp)  {

  int s, d, h, handRelFirst, maxAgg, maxHand=0;
  int k, noOfStartMoves;       /* Number of start moves in the 1st trick */
  int hand[3], suit[3], rank[3];
  struct moveType move;
  unsigned short int startMovesBitMap[4][4]; /* Indices are hand and suit */
  unsigned short int aggHand[4][4];

  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      startMovesBitMap[h][s]=0;

  handRelFirst=posPoint->handRelFirst;
  noOfStartMoves=handRelFirst;

  for (k=0; k<=2; k++) {
    hand[k]=handId(first, k);
    suit[k]=startMoves[k].suit;
    rank[k]=startMoves[k].rank;
    if (k<noOfStartMoves)
      startMovesBitMap[hand[k]][suit[k]]|=bitMapRank[rank[k]];
  }

  for (d=0; d<=49; d++) {
    thrp->bestMove[d].rank=0;
    thrp->bestMoveTT[d].rank=0;
  }

  if (((handId(first, handRelFirst))==0)||
    ((handId(first, handRelFirst))==2)) {
    thrp->nodeTypeStore[0]=MAXNODE;
    thrp->nodeTypeStore[1]=MINNODE;
    thrp->nodeTypeStore[2]=MAXNODE;
    thrp->nodeTypeStore[3]=MINNODE;
  }
  else {
    thrp->nodeTypeStore[0]=MINNODE;
    thrp->nodeTypeStore[1]=MAXNODE;
    thrp->nodeTypeStore[2]=MINNODE;
    thrp->nodeTypeStore[3]=MAXNODE;
  }

  k=noOfStartMoves;
  posPoint->first[depth]=first;
  posPoint->handRelFirst=k;
  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
  posPoint->tricksMAX=0;

  if (k>0) {
    posPoint->move[depth+k]=startMoves[k-1];
    move=startMoves[k-1];
  }

  posPoint->high[depth+k]=first;

  while (k>0) {
    thrp->movePly[depth+k].current=0;
    thrp->movePly[depth+k].last=0;
    thrp->movePly[depth+k].move[0].suit=startMoves[k-1].suit;
    thrp->movePly[depth+k].move[0].rank=startMoves[k-1].rank;
    if (k<noOfStartMoves) {     /* If there is more than one start move */
      if (WinningMove(&startMoves[k-1], &move, thrp->trump)) {
        posPoint->move[depth+k].suit=startMoves[k-1].suit;
        posPoint->move[depth+k].rank=startMoves[k-1].rank;
        posPoint->high[depth+k]=handId(first, noOfStartMoves-k);
        move=posPoint->move[depth+k];
      }
      else {
        posPoint->move[depth+k]=posPoint->move[depth+k+1];
        posPoint->high[depth+k]=posPoint->high[depth+k+1];
      }
    }
    k--;
  }

  for (s=0; s<=3; s++)
    posPoint->removedRanks[s]=0;

  for (s=0; s<=3; s++)       /* Suit */
    for (h=0; h<=3; h++)     /* Hand */
      posPoint->removedRanks[s]|=
        posPoint->rankInSuit[h][s];
  for (s=0; s<=3; s++)
    posPoint->removedRanks[s]=~(posPoint->removedRanks[s]);

  for (s=0; s<=3; s++)       /* Suit */
    for (h=0; h<=3; h++)     /* Hand */
      posPoint->removedRanks[s]&=
        (~startMovesBitMap[h][s]);

  for (s=0; s<=3; s++)
    thrp->iniRemovedRanks[s]=posPoint->removedRanks[s];

  /*for (d=0; d<=49; d++) {
    for (s=0; s<=3; s++)
      posPoint->winRanks[d][s]=0;
  }*/

  /* Initialize winning and second best ranks */
  for (s=0; s<=3; s++) {
    maxAgg=0;
    for (h=0; h<=3; h++) {
      aggHand[h][s]=startMovesBitMap[h][s] | thrp->game.suit[h][s];
      if (aggHand[h][s]>maxAgg) {
	 maxAgg=aggHand[h][s];
	 maxHand=h;
      }
    }
    if (maxAgg!=0) {
      posPoint->winner[s].hand=maxHand;
      k=highestRank[aggHand[maxHand][s]];
      posPoint->winner[s].rank=k;

      maxAgg=0;
      for (h=0; h<=3; h++) {
	aggHand[h][s]&=(~bitMapRank[k]);
        if (aggHand[h][s]>maxAgg) {
	  maxAgg=aggHand[h][s];
	  maxHand=h;
	}
      }
      if (maxAgg>0) {
	 posPoint->secondBest[s].hand=maxHand;
	 posPoint->secondBest[s].rank=highestRank[aggHand[maxHand][s]];
      }
      else {
	posPoint->secondBest[s].hand=-1;
        posPoint->secondBest[s].rank=0;
      }
    }
    else {
      posPoint->winner[s].hand=-1;
      posPoint->winner[s].rank=0;
      posPoint->secondBest[s].hand=-1;
      posPoint->secondBest[s].rank=0;
    }
  }


  for (s=0; s<=3; s++)
    for (h=0; h<=3; h++)
      posPoint->length[h][s]=
	(unsigned char)counttable[posPoint->rankInSuit[h][s]];

  #ifdef STAT
  for (d=0; d<=49; d++) {
    score1Counts[d]=0;
    score0Counts[d]=0;
    c1[d]=0;  c2[d]=0;  c3[d]=0;  c4[d]=0;  c5[d]=0;  c6[d]=0; c7[d]=0;
    c8[d]=0;
    thrp->no[d]=0;
  }
  #endif

  if (!mtd) {
    thrp->lenSetSize=0;
    for (k=0; k<=13; k++) {
      for (h=0; h<=3; h++) {
	thrp->rootnp[k][h]=&(thrp->posSearch[thrp->lenSetSize]);
        thrp->posSearch[thrp->lenSetSize].suitLengths=0;
        thrp->posSearch[thrp->lenSetSize].posSearchPoint=NULL;
        thrp->posSearch[thrp->lenSetSize].left=NULL;
        thrp->posSearch[thrp->lenSetSize].right=NULL;
        thrp->lenSetSize++;
      }
    }
    thrp->nodeSetSize=0;
    thrp->winSetSize=0;
  }

  return;
}

#ifdef STAT
int score1Counts[50], score0Counts[50];
int sumScore1Counts, sumScore0Counts, dd, suit, rank, order;
int c1[50], c2[50], c3[50], c4[50], c5[50], c6[50], c7[50], c8[50], c9[50];
int sumc1, sumc2, sumc3, sumc4, sumc5, sumc6, sumc7, sumc8, sumc9;
#endif

int ABsearch(struct pos * posPoint, int target, int depth, struct localVarType * thrp) {
    /* posPoint points to the current look-ahead position,
       target is number of tricks to take for the player,
       depth is the remaining search length, must be positive,
       the value of the subtree is returned.  */

  int moveExists, value, scoreFlag, found;
  int hh, ss, rr, qtricks, res, k;
  unsigned short int makeWinRank[4];
  struct nodeCardsType * cardsP;
  struct evalType evalData;
  struct winCardType * np;
  struct posSearchType * pp;
  struct nodeCardsType  * tempP;
  struct movePlyType *mply=&(thrp->movePly[depth]);
  unsigned short int aggr[4];
  long long suitLengths;
  
  struct evalType Evaluate(struct pos * posPoint, int trump, 
    struct localVarType * thrp);
  void Undo(struct pos * posPoint, int depth, struct movePlyType *mply, 
    struct localVarType * thrp);

  /*cardsP=NULL;*/
  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
  int trump=thrp->trump;
  int hand=handId(posPoint->first[depth], posPoint->handRelFirst);
#ifdef STAT
  thrp->nodes++;
#endif
  if (posPoint->handRelFirst==0) {
    /*thrp->trickNodes++;*/
    if (posPoint->tricksMAX>=target) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

        #ifdef STAT
        c1[depth]++;

        score1Counts[depth]++;
        if (depth==thrp->iniDepth) {
          fprintf(fp2, "score statistics:\n");
          for (dd=thrp->iniDepth; dd>=0; dd--) {
            fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
            score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
              c3[dd], c4[dd]);
            fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
              c6[dd], c7[dd], c8[dd]);
          }
        }
        #endif

      return TRUE;
    }
    if (((posPoint->tricksMAX+(depth>>2)+1)<target)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

 #ifdef STAT
        c2[depth]++;
        score0Counts[depth]++;
        if (depth==thrp->iniDepth) {
          fprintf(fp2, "score statistics:\n");
          for (dd=thrp->iniDepth; dd>=0; dd--) {
            fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
            score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
              c3[dd], c4[dd]);
            fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
              c6[dd], c7[dd], c8[dd]);
          }
        }
 #endif

      return FALSE;
    }

    if (thrp->nodeTypeStore[hand]==MAXNODE) {
      qtricks=QuickTricks(posPoint, hand, depth, target, trump, &res, thrp);
      if (res) {
	if (qtricks==0)
	  return FALSE;
	else
          return TRUE;
 #ifdef STAT
          c3[depth]++;
          score1Counts[depth]++;
          if (depth==thrp->iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=thrp->iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                c3[dd], c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
      }
      if (!LaterTricksMIN(posPoint,hand,depth,target,trump,thrp))
	return FALSE;
    }
    else {
      qtricks=QuickTricks(posPoint, hand, depth, target, trump, &res, thrp);
      if (res) {
        if (qtricks==0)
	  return TRUE;
	else
          return FALSE;
 #ifdef STAT
          c4[depth]++;
          score0Counts[depth]++;
          if (depth==thrp->iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=thrp->iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                c3[dd], c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
      }
      if (LaterTricksMAX(posPoint,hand,depth,target,trump,thrp))
	return TRUE;
    }
  }

  else if (posPoint->handRelFirst==1) {
    ss=posPoint->move[depth+1].suit;
    unsigned short ranks=posPoint->rankInSuit[hand][ss] |
      posPoint->rankInSuit[partner[hand]][ss];
    found=FALSE; rr=0; qtricks=0;

    if ((trump!=4) && (ss!=trump) &&
      (((posPoint->rankInSuit[hand][ss]==0)
	  && (posPoint->rankInSuit[hand][trump]!=0))||
	  ((posPoint->rankInSuit[partner[hand]][ss]==0)
	  && (posPoint->rankInSuit[partner[hand]][trump]!=0))))  {
	  /* Own side can ruff */
      if ((posPoint->rankInSuit[lho[hand]][ss]!=0)||
         (posPoint->rankInSuit[lho[hand]][trump]==0)) {
	found=TRUE;
        qtricks=1;
      }
    }

    else if ( ranks >(bitMapRank[posPoint->move[depth+1].rank] |
	  posPoint->rankInSuit[lho[hand]][ss])) {
	  /* Own side has highest card in suit */
      if ((trump==4) || ((ss==trump)||
        (posPoint->rankInSuit[lho[hand]][trump]==0)
	     || (posPoint->rankInSuit[lho[hand]][ss]!=0))) {
	rr=highestRank[ranks];
	if (rr!=0) {
	  found=TRUE;
	  qtricks=1;
	}
	else
	  found=FALSE;
      }
    }

    if ((found)&&(depth!=thrp->iniDepth)) {
      for (k=0; k<=3; k++)
	posPoint->winRanks[depth][k]=0;
      if (rr!=0)
	posPoint->winRanks[depth][ss]=bitMapRank[rr];

      if (thrp->nodeTypeStore[hand]==MAXNODE) {
        if (posPoint->tricksMAX+qtricks>=target) {
          return TRUE;
	}
	else if (trump==4) {
	  if (posPoint->rankInSuit[hand][ss] > posPoint->rankInSuit[partner[hand]][ss])
	    hh=hand;	/* Hand to lead next trick */
	  else
	    hh=partner[hand];

	  if ((posPoint->winner[ss].hand==hh)&&(posPoint->secondBest[ss].rank!=0)&&
		(posPoint->secondBest[ss].hand==hh)) {
	    qtricks++;
	    posPoint->winRanks[depth][ss]|=bitMapRank[posPoint->secondBest[ss].rank];
	    if (posPoint->tricksMAX+qtricks>=target) 
	      return TRUE;
	  }

	  for (k=0; k<=3; k++) {
	    if ((k!=ss)&&(posPoint->length[hh][k]!=0))  {	/* Not lead suit, not void in suit */
	      if ((posPoint->length[lho[hh]][k]==0)&&(posPoint->length[rho[hh]][k]==0)
		&&(posPoint->length[partner[hh]][k]==0)) {
		qtricks+=counttable[posPoint->rankInSuit[hh][k]];
		if (posPoint->tricksMAX+qtricks>=target) 
		  return TRUE;
	      }
	      else if ((posPoint->winner[k].rank!=0)&&(posPoint->winner[k].hand==hh)) {
		qtricks++;
		posPoint->winRanks[depth][k]|=bitMapRank[posPoint->winner[k].rank];
		if (posPoint->tricksMAX+qtricks>=target) 
		  return TRUE;
	      }
	    }
	  }
	}
      }
      else {
   	/* MIN node */
        if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) {
          return FALSE;
	}
	else if (trump==4) {
	  if (posPoint->rankInSuit[hand][ss] > posPoint->rankInSuit[partner[hand]][ss])
	    hh=hand;	/* Hand to lead next trick */
	  else
	    hh=partner[hand];

	  if ((posPoint->winner[ss].hand==hh)&&(posPoint->secondBest[ss].rank!=0)&&
	     (posPoint->secondBest[ss].hand==hh)) {
	    qtricks++;
	    posPoint->winRanks[depth][ss]|=bitMapRank[posPoint->secondBest[ss].rank];
	    if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) 
	      return FALSE;
	  }

	  for (k=0; k<=3; k++) {
	    if ((k!=ss)&&(posPoint->length[hh][k]!=0))  {	/* Not lead suit, not void in suit */
	      if ((posPoint->length[lho[hh]][k]==0)&&(posPoint->length[rho[hh]][k]==0)
		  &&(posPoint->length[partner[hh]][k]==0)) {
		qtricks+=counttable[posPoint->rankInSuit[hh][k]];
		if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) 
		  return FALSE;
	      }
	      else if ((posPoint->winner[k].rank!=0)&&(posPoint->winner[k].hand==hh)) {
		qtricks++;
		posPoint->winRanks[depth][k]|=bitMapRank[posPoint->winner[k].rank];
		if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) 
		  return FALSE;
	      }
	    }
	  }
	}
      }
    }
  }

  if ((posPoint->handRelFirst==0)&&
    (depth!=thrp->iniDepth)) {
    for (ss=0; ss<=3; ss++) {
      aggr[ss]=0;
      for (hh=0; hh<=3; hh++)
	aggr[ss]|=posPoint->rankInSuit[hh][ss];
      posPoint->orderSet[ss]=thrp->abs[aggr[ss]].aggrRanks[ss];
    }
    suitLengths=0;
    for (ss=0; ss<=2; ss++)
      for (hh=0; hh<=3; hh++) {
	suitLengths<<=4;
	suitLengths|=posPoint->length[hh][ss];
      }

    pp=SearchLenAndInsert(thrp->rootnp[depth>>2][hand],
	 suitLengths, FALSE, &res, thrp);
	/* Find node that fits the suit lengths */
    if (pp!=NULL) {
      np=pp->posSearchPoint;
      if (np==NULL)
        cardsP=NULL;
      else
        cardsP=FindSOP(posPoint, np, hand, target, depth>>2, &scoreFlag, thrp);

      if (cardsP!=NULL) {
        if (scoreFlag==1) {
	  for (ss=0; ss<=3; ss++)
	    posPoint->winRanks[depth][ss]=
	      thrp->adaptWins[aggr[ss]].winRanks[(int)cardsP->leastWin[ss]];

          if (cardsP->bestMoveRank!=0) {
            thrp->bestMoveTT[depth].suit=cardsP->bestMoveSuit;
            thrp->bestMoveTT[depth].rank=cardsP->bestMoveRank;
          }
 #ifdef STAT
          c5[depth]++;
          if (scoreFlag==1)
            score1Counts[depth]++;
          else
            score0Counts[depth]++;
          if (depth==thrp->iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=thrp->iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
                score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],c3[dd], c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
          return TRUE;
	}
        else {
	  for (ss=0; ss<=3; ss++)
	    posPoint->winRanks[depth][ss]=
	      thrp->adaptWins[aggr[ss]].winRanks[(int)cardsP->leastWin[ss]];

          if (cardsP->bestMoveRank!=0) {
            thrp->bestMoveTT[depth].suit=cardsP->bestMoveSuit;
            thrp->bestMoveTT[depth].rank=cardsP->bestMoveRank;
          }
 #ifdef STAT
          c6[depth]++;
          if (scoreFlag==1)
            score1Counts[depth]++;
          else
            score0Counts[depth]++;
          if (depth==thrp->iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=thrp->iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
                score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd],
                  c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                  c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
          return FALSE;
	}
      }
    }
  }

  if (depth==0) {                    /* Maximum depth? */
    evalData=Evaluate(posPoint, trump, thrp);        /* Leaf node */
    if (evalData.tricks>=target)
      value=TRUE;
    else
      value=FALSE;
    for (ss=0; ss<=3; ss++) {
      posPoint->winRanks[depth][ss]=evalData.winRanks[ss];

 #ifdef STAT
        c7[depth]++;
        if (value==1)
          score1Counts[depth]++;
        else
          score0Counts[depth]++;
        if (depth==thrp->iniDepth) {
          fprintf(fp2, "score statistics:\n");
          for (dd=thrp->iniDepth; dd>=0; dd--) {
            fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd],
              c4[dd]);
            fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
              c6[dd], c7[dd], c8[dd]);
          }
        }
 #endif
    }
    return value;
  }
  else {

    moveExists=MoveGen(posPoint, depth, trump, mply, thrp);

/*#if 0*/
    if ((posPoint->handRelFirst==3)&&(depth>=/*29*//*33*/37)
	&&(depth!=thrp->iniDepth)) {
      mply->current=0;
      int mexists=TRUE;
      int ready=FALSE;
      while (mexists) {
	Make(posPoint, makeWinRank, depth, trump, mply, thrp);
	depth--;

	for (ss=0; ss<=3; ss++) {
	  aggr[ss]=0;
	  for (hh=0; hh<=3; hh++)
	    aggr[ss]|=posPoint->rankInSuit[hh][ss];
	  posPoint->orderSet[ss]=thrp->abs[aggr[ss]].aggrRanks[ss];
	}
	int hfirst=posPoint->first[depth];
	suitLengths=0;
	for (ss=0; ss<=2; ss++)
          for (hh=0; hh<=3; hh++) {
	    suitLengths<<=4;
	    suitLengths|=posPoint->length[hh][ss];
	  }

	pp=SearchLenAndInsert(thrp->rootnp[depth>>2][hfirst],
	  suitLengths, FALSE, &res, thrp);
	/* Find node that fits the suit lengths */
	if (pp!=NULL) {
	  np=pp->posSearchPoint;
	  if (np==NULL)
	    tempP=NULL;
	  else
	    tempP=FindSOP(posPoint, np, hfirst, target, depth>>2, &scoreFlag, thrp);

	  if (tempP!=NULL) {
	    if ((thrp->nodeTypeStore[hand]==MAXNODE)&&(scoreFlag==1)) {
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]=
		  thrp->adaptWins[aggr[ss]].winRanks[(int)tempP->leastWin[ss]];
	      if (tempP->bestMoveRank!=0) {
		thrp->bestMoveTT[depth+1].suit=tempP->bestMoveSuit;
		thrp->bestMoveTT[depth+1].rank=tempP->bestMoveRank;
	      }
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]|=makeWinRank[ss];
	      Undo(posPoint, depth+1, mply/*&(thrp->movePly[depth+1])*/,thrp);
	      return TRUE;
	    }
	    else if ((thrp->nodeTypeStore[hand]==MINNODE)&&(scoreFlag==0)) {
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]=
		  thrp->adaptWins[aggr[ss]].winRanks[(int)tempP->leastWin[ss]];
	      if (tempP->bestMoveRank!=0) {
		thrp->bestMoveTT[depth+1].suit=tempP->bestMoveSuit;
		thrp->bestMoveTT[depth+1].rank=tempP->bestMoveRank;
	      }
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]|=makeWinRank[ss];
	      Undo(posPoint, depth+1, mply/*&(thrp->movePly[depth+1])*/, thrp);
		return FALSE;
	    }
	    else {
	      mply->move[mply->current].weight+=100;
	      /*thrp->movePly[depth+1].move[thrp->movePly[depth+1].current].weight+=100;*/
	      ready=TRUE;
	    }
	  }
	}
	depth++;
	Undo(posPoint, depth, mply, thrp);
	if (ready)
	  break;

	if (mply->current <= (mply->last-1)) {
	  mply->current++;
	  mexists=TRUE;
	}
	else
	  mexists=FALSE;
      }
      if (ready)
	MergeSort(mply->last+1, mply->move);
    }
/*#endif*/


    mply->current=0;
    if (thrp->nodeTypeStore[hand]==MAXNODE) {
      value=FALSE;
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

      while (moveExists)  {
        Make(posPoint, makeWinRank, depth, trump, mply, thrp);        /* Make current move */
	if (posPoint->handRelFirst==0)
	  thrp->trickNodes++;
        assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
	value=ABsearch(posPoint, target, depth-1, thrp);

        Undo(posPoint, depth, mply, thrp);      /* Retract current move */
        if (value==TRUE) {
        /* A cut-off? */
	  for (ss=0; ss<=3; ss++)
            posPoint->winRanks[depth][ss]=posPoint->winRanks[depth-1][ss] |
              makeWinRank[ss];
	  thrp->bestMove[depth]=mply->move[mply->current];
          goto ABexit;
        }
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth][ss] |
            posPoint->winRanks[depth-1][ss] | makeWinRank[ss];

        moveExists=NextMove(posPoint, depth, mply, thrp);
      }
    }
    else {                          /* A minnode */
      value=TRUE;
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

      while (moveExists)  {
        Make(posPoint, makeWinRank, depth, trump, mply, thrp);        /* Make current move */
	if (posPoint->handRelFirst==0)
	  thrp->trickNodes++;
        value=ABsearch(posPoint, target, depth-1, thrp);

        Undo(posPoint, depth, mply, thrp);       /* Retract current move */
        if (value==FALSE) {
        /* A cut-off? */
	  for (ss=0; ss<=3; ss++)
            posPoint->winRanks[depth][ss]=posPoint->winRanks[depth-1][ss] |
              makeWinRank[ss];
          thrp->bestMove[depth]=mply->move[mply->current];
          goto ABexit;
        }
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth][ss] |
            posPoint->winRanks[depth-1][ss] | makeWinRank[ss];

        moveExists=NextMove(posPoint, depth, mply, thrp);
      }
    }
  }
  ABexit:
  if (depth>=4) {
    if(posPoint->handRelFirst==0) {
      if (value)
	k=target;
      else
	k=target-1;
      if (depth!=thrp->iniDepth)
        BuildSOP(posPoint, suitLengths, depth>>2, hand, target, depth,
          value, k, thrp);
      if (thrp->clearTTflag) {
         /* Wipe out the TT dynamically allocated structures
	    except for the initially allocated structures.
	    Set the TT limits to the initial values.
	    Reset TT array indices to zero.
	    Reset memory chunk indices to zero.
	    Set allocated memory to the initial value. */
        /*fp2=fopen("dyn.txt", "a");
	fprintf(fp2, "Clear TT:\n");
	fprintf(fp2, "wcount=%d, ncount=%d, lcount=%d\n",
	       wcount, ncount, lcount);
        fprintf(fp2, "winSetSize=%d, nodeSetSize=%d, lenSetSize=%d\n",
	       winSetSize, nodeSetSize, lenSetSize);
	fprintf(fp2, "\n");
        fclose(fp2);*/

        Wipe(thrp);
	thrp->winSetSizeLimit=WINIT;
	thrp->nodeSetSizeLimit=NINIT;
	thrp->lenSetSizeLimit=LINIT;
	thrp->lcount=0;
	thrp->allocmem=(thrp->lenSetSizeLimit+1)*sizeof(struct posSearchType);
	thrp->lenSetSize=0;
	thrp->posSearch=thrp->pl[thrp->lcount];
	for (k=0; k<=13; k++) {
	  for (hh=0; hh<=3; hh++) {
	    thrp->rootnp[k][hh]=&(thrp->posSearch[thrp->lenSetSize]);
	    thrp->posSearch[thrp->lenSetSize].suitLengths=0;
	    thrp->posSearch[thrp->lenSetSize].posSearchPoint=NULL;
	    thrp->posSearch[thrp->lenSetSize].left=NULL;
	    thrp->posSearch[thrp->lenSetSize].right=NULL;
	    thrp->lenSetSize++;
	  }
	}
        thrp->nodeSetSize=0;
        thrp->winSetSize=0;
	thrp->wcount=0; thrp->ncount=0;
	thrp->allocmem+=(thrp->winSetSizeLimit+1)*sizeof(struct winCardType);
        thrp->winCards=thrp->pw[thrp->wcount];
	thrp->allocmem+=(thrp->nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
	thrp->nodeCards=thrp->pn[thrp->ncount];
	thrp->clearTTflag=FALSE;
	thrp->windex=-1;
      }
    }
  }

 #ifdef STAT
  c8[depth]++;
  if (value==1)
    score1Counts[depth]++;
  else
    score0Counts[depth]++;
  if (depth==thrp->iniDepth) {
    if (fp2==NULL)
      exit(0);
    fprintf(fp2, "\n");
    fprintf(fp2, "top level cards:\n");
    for (hh=0; hh<=3; hh++) {
      fprintf(fp2, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
        fprintf(fp2, "suit=%c", cardSuit[ss]);
        for (rr=14; rr>=2; rr--)
          if (posPoint->rankInSuit[hh][ss] & bitMapRank[rr])
            fprintf(fp2, " %c", cardRank[rr]);
        fprintf(fp2, "\n");
      }
      fprintf(fp2, "\n");
    }
    fprintf(fp2, "top level winning cards:\n");
    for (ss=0; ss<=3; ss++) {
      fprintf(fp2, "suit=%c", cardSuit[ss]);
      for (rr=14; rr>=2; rr--)
        if (posPoint->winRanks[depth][ss] & bitMapRank[rr])
          fprintf(fp2, " %c", cardRank[rr]);
      fprintf(fp2, "\n");
    }
    fprintf(fp2, "\n");
    fprintf(fp2, "\n");

    fprintf(fp2, "score statistics:\n");
    sumScore0Counts=0;
    sumScore1Counts=0;
    sumc1=0; sumc2=0; sumc3=0; sumc4=0;
    sumc5=0; sumc6=0; sumc7=0; sumc8=0; sumc9=0;
    for (dd=thrp->iniDepth; dd>=0; dd--) {
      fprintf(fp2, "depth=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
          score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd], c4[dd]);
      fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd], c6[dd],
          c7[dd], c8[dd]);
      sumScore0Counts=sumScore0Counts+score0Counts[dd];
      sumScore1Counts=sumScore1Counts+score1Counts[dd];
      sumc1=sumc1+c1[dd];
      sumc2=sumc2+c2[dd];
      sumc3=sumc3+c3[dd];
      sumc4=sumc4+c4[dd];
      sumc5=sumc5+c5[dd];
      sumc6=sumc6+c6[dd];
      sumc7=sumc7+c7[dd];
      sumc8=sumc8+c8[dd];
      sumc9=sumc9+c9[dd];
    }
    fprintf(fp2, "\n");
    fprintf(fp2, "score sum statistics:\n");
    fprintf(fp2, "\n");
    fprintf(fp2, "sumScore0Counts=%d sumScore1Counts=%d\n",
        sumScore0Counts, sumScore1Counts);
    fprintf(fp2, "nodeSetSize=%d  winSetSize=%d\n", thrp->nodeSetSize,
        thrp->winSetSize);
    fprintf(fp2, "sumc1=%d sumc2=%d sumc3=%d sumc4=%d\n",
        sumc1, sumc2, sumc3, sumc4);
    fprintf(fp2, "sumc5=%d sumc6=%d sumc7=%d sumc8=%d sumc9=%d\n",
        sumc5, sumc6, sumc7, sumc8, sumc9);
    fprintf(fp2, "\n");
    fprintf(fp2, "\n");
    fprintf(fp2, "No of searched nodes per depth:\n");
    for (dd=thrp->iniDepth; dd>=0; dd--)
      fprintf(fp2, "depth=%d  nodes=%d\n", dd, thrp->no[dd]);
    fprintf(fp2, "\n");
    fprintf(fp2, "Total nodes=%d\n", thrp->nodes);
  }
 #endif

  return value;
}


void Make(struct pos * posPoint, unsigned short int trickCards[4],
  int depth, int trump, struct movePlyType *mply, struct localVarType * thrp)  {
  int t, u, w;
  int mcurr, q;

  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
  for (int suit=0; suit<=3; suit++)
    trickCards[suit]=0;

  int firstHand=posPoint->first[depth];
  int r=mply->current;

  if (posPoint->handRelFirst==3)  {         /* This hand is last hand */
    if (mply->move[r].suit==posPoint->move[depth+1].suit) {
      if (mply->move[r].rank>posPoint->move[depth+1].rank) {
	posPoint->move[depth]=mply->move[r];
        posPoint->high[depth]=handId(firstHand, 3);
      }
      else {
        posPoint->move[depth]=posPoint->move[depth+1];
        posPoint->high[depth]=posPoint->high[depth+1];
      }
    }
    else if (mply->move[r].suit==trump) {
      posPoint->move[depth]=mply->move[r];
      posPoint->high[depth]=handId(firstHand, 3);
    }
    else {
      posPoint->move[depth]=posPoint->move[depth+1];
      posPoint->high[depth]=posPoint->high[depth+1];
    }

    /* Is the trick won by rank? */
    int s=posPoint->move[depth].suit;
    int count=0;
    if (mply->move[r].suit==s)
      count++;
    for (int e=1; e<=3; e++) {
      mcurr=thrp->movePly[depth+e].current;
      if (thrp->movePly[depth+e].move[mcurr].suit==s) {
        count++;
        /*if (++count>1)
          break;*/
      }
    }


    if (thrp->nodeTypeStore[posPoint->high[depth]]==MAXNODE)
      posPoint->tricksMAX++;
    posPoint->first[depth-1]=posPoint->high[depth];   /* Defines who is first
        in the next move */

    t=handId(firstHand, 3);
    posPoint->handRelFirst=0;      /* Hand pointed to by posPoint->first
                                    will lead the next trick */


    int done=FALSE;
    for (int d=3; d>=0; d--) {
      q=handId(firstHand, 3-d);
    /* Add the moves to removed ranks */
      r=thrp->movePly[depth+d].current;
      w=thrp->movePly[depth+d].move[r].rank;
      u=thrp->movePly[depth+d].move[r].suit;
      posPoint->removedRanks[u]|=bitMapRank[w];

      if (d==0)
        posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);

      if ((w==posPoint->winner[u].rank)||(w==posPoint->secondBest[u].rank)) {
	int aggr=0;
        for (int h=0; h<=3; h++)
	  aggr|=posPoint->rankInSuit[h][u];
	posPoint->winner[u].rank=thrp->abs[aggr].absRank[1][u].rank;
	posPoint->winner[u].hand=thrp->abs[aggr].absRank[1][u].hand;
	posPoint->secondBest[u].rank=thrp->abs[aggr].absRank[2][u].rank;
	posPoint->secondBest[u].hand=thrp->abs[aggr].absRank[2][u].hand;
      }


    /* Determine win-ranked cards */
      if ((q==posPoint->high[depth])&&(!done)) {
        done=TRUE;
        if (count>=2) {
          trickCards[u]=bitMapRank[w];
          /* Mark ranks as winning if they are part of a sequence */
          trickCards[u]|=thrp->movePly[depth+d].move[r].sequence;
        }
      }
    }
  }
  else if (posPoint->handRelFirst==0) {   /* Is it the 1st hand? */
    posPoint->first[depth-1]=firstHand;   /* First hand is not changed in
                                            next move */
    posPoint->high[depth]=firstHand;
    posPoint->move[depth]=mply->move[r];
    t=firstHand;
    posPoint->handRelFirst=1;
    r=mply->current;
    u=mply->move[r].suit;
    w=mply->move[r].rank;
    posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);
  }
  else {
    r=mply->current;
    u=mply->move[r].suit;
    w=mply->move[r].rank;
    if (u==posPoint->move[depth+1].suit) {
      if (w>posPoint->move[depth+1].rank) {
	posPoint->move[depth]=mply->move[r];
        posPoint->high[depth]=handId(firstHand, posPoint->handRelFirst);
      }
      else {
	posPoint->move[depth]=posPoint->move[depth+1];
        posPoint->high[depth]=posPoint->high[depth+1];
      }
    }
    else if (u==trump) {
      posPoint->move[depth]=mply->move[r];
      posPoint->high[depth]=handId(firstHand, posPoint->handRelFirst);
    }
    else {
      posPoint->move[depth]=posPoint->move[depth+1];
      posPoint->high[depth]=posPoint->high[depth+1];
    }

    t=handId(firstHand, posPoint->handRelFirst);
    posPoint->handRelFirst++;               /* Current hand is stepped */
    assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
    posPoint->first[depth-1]=firstHand;     /* First hand is not changed in
                                            next move */

    posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);
  }

  posPoint->length[t][u]--;

#ifdef STAT
  thrp->no[depth]++;
#endif

  return;
}



void Undo(struct pos * posPoint, int depth, struct movePlyType *mply, 
  struct localVarType * thrp)  {
  int r, t, u, w;

  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));

  switch (posPoint->handRelFirst) {
    case 3: case 2: case 1:
     posPoint->handRelFirst--;
     break;
    case 0:
     posPoint->handRelFirst=3;
  }

  int firstHand=posPoint->first[depth];

  if (posPoint->handRelFirst==0) {          /* 1st hand which won the previous
                                            trick */
    t=firstHand;
    r=mply->current;
    u=mply->move[r].suit;
    w=mply->move[r].rank;
  }
  else if (posPoint->handRelFirst==3)  {    /* Last hand */
    for (int d=3; d>=0; d--) {
    /* Delete the moves from removed ranks */
      r=thrp->movePly[depth+d].current;
      w=thrp->movePly[depth+d].move[r].rank;
      u=thrp->movePly[depth+d].move[r].suit;

      posPoint->removedRanks[u]&= (~bitMapRank[w]);

      if (w>posPoint->winner[u].rank) {
        /*posPoint->secondBest[u].rank=posPoint->winner[u].rank;
        posPoint->secondBest[u].hand=posPoint->winner[u].hand;*/
	posPoint->secondBest[u]=posPoint->winner[u];
        posPoint->winner[u].rank=w;
        posPoint->winner[u].hand=handId(firstHand, 3-d);

      }
      else if (w>posPoint->secondBest[u].rank) {
        posPoint->secondBest[u].rank=w;
        posPoint->secondBest[u].hand=handId(firstHand, 3-d);
      }
    }
    t=handId(firstHand, 3);


    if (thrp->nodeTypeStore[posPoint->first[depth-1]]==MAXNODE)
        /* First hand of next trick is winner of the current trick */
      posPoint->tricksMAX--;
  }
  else {
    t=handId(firstHand, posPoint->handRelFirst);
    r=mply->current;
    u=mply->move[r].suit;
    w=mply->move[r].rank;
  }

  posPoint->rankInSuit[t][u]|=bitMapRank[w];

  posPoint->length[t][u]++;

  return;
}



struct evalType Evaluate(struct pos * posPoint, int trump, 
  struct localVarType * thrp)  {
  int s, h, hmax=0, count=0, k=0;
  unsigned short rmax=0;
  struct evalType eval;

  int firstHand=posPoint->first[0];
  assert((firstHand >= 0)&&(firstHand <= 3));

  for (s=0; s<=3; s++)
    eval.winRanks[s]=0;

  /* Who wins the last trick? */
  if (trump!=4)  {            /* Highest trump card wins */
    for (h=0; h<=3; h++) {
      if (posPoint->rankInSuit[h][trump]!=0)
        count++;
      if (posPoint->rankInSuit[h][trump]>rmax) {
        hmax=h;
        rmax=posPoint->rankInSuit[h][trump];
      }
    }

    if (rmax>0) {        /* Trumpcard wins */
      if (count>=2)
        eval.winRanks[trump]=rmax;

      if (thrp->nodeTypeStore[hmax]==MAXNODE)
        goto maxexit;
      else
        goto minexit;
    }
  }

  /* Who has the highest card in the suit played by 1st hand? */

  k=0;
  while (k<=3)  {           /* Find the card the 1st hand played */
    if (posPoint->rankInSuit[firstHand][k]!=0)      /* Is this the card? */
      break;
    k++;
  }

  assert(k < 4);

  for (h=0; h<=3; h++)  {
    if (posPoint->rankInSuit[h][k]!=0)
      count++;
    if (posPoint->rankInSuit[h][k]>rmax)  {
      hmax=h;
      rmax=posPoint->rankInSuit[h][k];
    }
  }

  if (count>=2)
    eval.winRanks[k]=rmax;

  if (thrp->nodeTypeStore[hmax]==MAXNODE)
    goto maxexit;
  else
    goto minexit;

  maxexit:
  eval.tricks=posPoint->tricksMAX+1;
  return eval;

  minexit:
  eval.tricks=posPoint->tricksMAX;
  return eval;
}


int QuickTricks(struct pos * posPoint, int hand,
	int depth, int target, int trump, int *result, struct localVarType * thrp) {
  int suit, sum, commRank=0, commSuit=-1, s;
  int opps, res;
  int countLho, countRho, countPart, countOwn, lhoTrumpRanks, rhoTrumpRanks;
  int cutoff, ss, rr, lowestQtricks=0;

  int QtricksLeadHandNT(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int *lhoTrumpRanks, int *rhoTrumpRanks, int commPartner,
	int commSuit, int countOwn, int countPart, int suit, int qtricks, int trump, int *res);

  int QtricksLeadHandTrump(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int trump, int *res);

  int QuickTricksPartnerHandTrump(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, 
        struct localVarType * thrp);

  int QuickTricksPartnerHandNT(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, 
        struct localVarType * thrp);

  *result=TRUE;
  int qtricks=0;
  for (s=0; s<=3; s++)
    posPoint->winRanks[depth][s]=0;

  if ((depth<=0)||(depth==thrp->iniDepth)) {
    *result=FALSE;
    return qtricks;
  }

  if (thrp->nodeTypeStore[hand]==MAXNODE)
    cutoff=target-posPoint->tricksMAX;
  else
    cutoff=posPoint->tricksMAX-target+(depth>>2)+2;

  int commPartner=FALSE;
  for (s=0; s<=3; s++) {
    if ((trump!=4)&&(trump!=s)) {
      if (posPoint->winner[s].hand==partner[hand]) {
        /* Partner has winning card */
        if (posPoint->rankInSuit[hand][s]!=0) {
        /* Own hand has card in suit */
          if (((posPoint->rankInSuit[lho[hand]][s]!=0) ||
          /* LHO not void */
          (posPoint->rankInSuit[lho[hand]][trump]==0))
          /* LHO has no trump */
          && ((posPoint->rankInSuit[rho[hand]][s]!=0) ||
          /* RHO not void */
          (posPoint->rankInSuit[rho[hand]][trump]==0))) {
          /* RHO has no trump */
            commPartner=TRUE;
            commSuit=s;
            commRank=posPoint->winner[s].rank;
            break;
          }
        }
      }
      else if (posPoint->secondBest[s].hand==partner[hand]) {
        if ((posPoint->winner[s].hand==hand)&&
	  (posPoint->length[hand][s]>=2)&&(posPoint->length[partner[hand]][s]>=2)) {
	  if (((posPoint->rankInSuit[lho[hand]][s]!=0) ||
            (posPoint->rankInSuit[lho[hand]][trump]==0))
            && ((posPoint->rankInSuit[rho[hand]][s]!=0) ||
            (posPoint->rankInSuit[rho[hand]][trump]==0))) {
	    commPartner=TRUE;
            commSuit=s;
            commRank=posPoint->secondBest[s].rank;
            break;
	  }
	}
      }
    }
    else if (trump==4) {
      if (posPoint->winner[s].hand==partner[hand]) {
        /* Partner has winning card */
        if (posPoint->rankInSuit[hand][s]!=0) {
        /* Own hand has card in suit */
          commPartner=TRUE;
          commSuit=s;
          commRank=posPoint->winner[s].rank;
          break;
        }
      }
      else if (posPoint->secondBest[s].hand==partner[hand]) {
        if ((posPoint->winner[s].hand==hand)&&
	  (posPoint->length[hand][s]>=2)&&(posPoint->length[partner[hand]][s]>=2)) {
	  commPartner=TRUE;
          commSuit=s;
          commRank=posPoint->secondBest[s].rank;
          break;
	}
      }
    }
  }

  if ((trump!=4) && (!commPartner) &&
	  (posPoint->rankInSuit[hand][trump]!=0) &&
	  (posPoint->winner[trump].hand==partner[hand])) {
    commPartner=TRUE;
    commSuit=trump;
    commRank=posPoint->winner[trump].rank;
  }


  if (trump!=4) {
    suit=trump;
    lhoTrumpRanks=posPoint->length[lho[hand]][trump];
    rhoTrumpRanks=posPoint->length[rho[hand]][trump];
  }
  else
    suit=0;

  do {
    countOwn=posPoint->length[hand][suit];
    countLho=posPoint->length[lho[hand]][suit];
    countRho=posPoint->length[rho[hand]][suit];
    countPart=posPoint->length[partner[hand]][suit];
    opps=countLho | countRho;

    if (!opps && (countPart==0)) {
      if (countOwn==0) {
	/* Continue with next suit. */
	if ((trump!=4)&&(trump!=suit)) {
	  suit++;
          if ((trump!=4) && (suit==trump))
            suit++;
	}
	else {
	  if ((trump!=4) && (trump==suit)) {
            if (trump==0)
              suit=1;
            else
              suit=0;
          }
          else
            suit++;
	}
	continue;
      }

      /* Long tricks when only leading hand have cards in the suit. */
      if ((trump!=4) && (trump!=suit)) {
        if ((lhoTrumpRanks==0) && (rhoTrumpRanks==0)) {
          qtricks+=countOwn;
	  if (qtricks>=cutoff)
            return qtricks;
          suit++;
          if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
        }
        else {
          suit++;
          if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
        }
      }
      else {
        qtricks+=countOwn;
	if (qtricks>=cutoff)
          return qtricks;

        if ((trump!=4) && (suit==trump)) {
          if (trump==0)
            suit=1;
          else
            suit=0;
        }
        else {
          suit++;
          if ((trump!=4) && (suit==trump))
            suit++;
        }
	continue;
      }
    }
    else {
      if (!opps && (trump!=4) && (suit==trump)) {
	/* The partner but not the opponents have cards in the trump suit. */
        sum=Max(countOwn, countPart);
	for (s=0; s<=3; s++) {
	  if ((sum>0)&&(s!=trump)&&(countOwn>=countPart)&&(posPoint->length[hand][s]>0)&&
	    (posPoint->length[partner[hand]][s]==0)) {
	    sum++;
	    break;
	  }
	}
	/* If the additional trick by ruffing causes a cutoff. (qtricks not incremented.) */
	if (sum>=cutoff)
	  return sum;
      }
      else if (!opps) {
	/* The partner but not the opponents have cards in the suit. */
	sum=Min(countOwn,countPart);
	if (trump==4) {
	  if (sum>=cutoff)
	    return sum;
	}
	else if ((suit!=trump)&&(lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
	  if (sum>=cutoff)
	    return sum;
	}
      }

      if (commPartner) {
        if (!opps && (countOwn==0)) {
          if ((trump!=4) && (trump!=suit)) {
            if ((lhoTrumpRanks==0) && (rhoTrumpRanks==0)) {
              qtricks+=countPart;
	      posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
	      if (qtricks>=cutoff)
                return qtricks;
              suit++;
              if ((trump!=4) && (suit==trump))
                suit++;
	      continue;
            }
            else {
              suit++;
              if ((trump!=4) && (suit==trump))
                suit++;
	      continue;
            }
          }
          else {
            qtricks+=countPart;
            posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
	        if (qtricks>=cutoff)
              return qtricks;
            if ((trump!=4) && (suit==trump)) {
              if (trump==0)
                suit=1;
              else
                suit=0;
            }
            else {
              suit++;
              if ((trump!=4) && (suit==trump))
                suit++;
            }
	    continue;
          }
        }
	else {
	  if (!opps && (trump!=4) && (suit==trump)) {
	    sum=Max(countOwn, countPart);
	    for (s=0; s<=3; s++) {
	      if ((sum>0)&&(s!=trump)&&(countOwn<=countPart)&&(posPoint->length[partner[hand]][s]>0)&&
	         (posPoint->length[hand][s]==0)) {
	        sum++;
	        break;
	      }
	    }
            if (sum>=cutoff) {
	      posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
	      return sum;
	    }
	  }
	  else if (!opps) {
	    sum=Min(countOwn,countPart);
	    if (trump==4) {
	      if (sum>=cutoff)
	        return sum;
	    }
	    else if ((suit!=trump)&&(lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
	      if (sum>=cutoff)
	        return sum;
	    }
	  }
	}
      }
    }

    if (posPoint->winner[suit].rank==0) {
      if ((trump!=4) && (suit==trump)) {
        if (trump==0)
          suit=1;
        else
          suit=0;
      }
      else {
        suit++;
        if ((trump!=4) && (suit==trump))
          suit++;
      }
      continue;
    }

    if (posPoint->winner[suit].hand==hand) {
      if ((trump!=4)&&(trump!=suit)) {
	qtricks=QtricksLeadHandTrump(hand, posPoint, cutoff, depth,
	       countLho, countRho, lhoTrumpRanks, rhoTrumpRanks, countOwn,
	       countPart, suit, qtricks, trump, &res);
	if (res==1)
          return qtricks;
	else if (res==2) {
	  suit++;
          if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
        }
      }
      else {
        qtricks=QtricksLeadHandNT(hand, posPoint, cutoff, depth,
	  countLho, countRho, &lhoTrumpRanks, &rhoTrumpRanks,
          commPartner, commSuit, countOwn,
          countPart, suit, qtricks, trump, &res);
        if (res==1)
	  return qtricks;
        else if (res==2) {
          if ((trump!=4) && (trump==suit)) {
            if (trump==0)
              suit=1;
            else
              suit=0;
          }
          else
            suit++;
          continue;
        }
      }
    }

    /* It was not possible to take a quick trick by own winning card in
    the suit */
    else {
    /* Partner winning card? */
      if ((posPoint->winner[suit].hand==partner[hand])&&(1/*countPart>0*/)) {
        /* Winner found at partner*/
        if (commPartner) {
        /* There is communication with the partner */
          if ((trump!=4)&&(trump!=suit)) {
	    qtricks=QuickTricksPartnerHandTrump(hand, posPoint, cutoff, depth,
	      countLho, countRho, lhoTrumpRanks, rhoTrumpRanks, countOwn,
	      countPart, suit, qtricks, commSuit, commRank, trump, &res, thrp);
	    if (res==1)
	      return qtricks;
	    else if (res==2) {
	      suit++;
              if ((trump!=4) && (suit==trump))
                suit++;
	      continue;
	    }
          }
          else {
	    qtricks=QuickTricksPartnerHandNT(hand, posPoint, cutoff, depth,
	      countLho, countRho, countOwn,
	      countPart, suit, qtricks, commSuit, commRank, trump, &res, thrp);
	    if (res==1)
	      return qtricks;
	    else if (res==2) {
	      if ((trump!=4) && (trump==suit)) {
                if (trump==0)
                  suit=1;
                else
                  suit=0;
              }
              else
                suit++;
              continue;
	    }
          }
        }
      }
    }
    if ((trump!=4) &&(suit!=trump)&&
	(countOwn>0)&&(lowestQtricks==0)&&
	((qtricks==0)||((posPoint->winner[suit].hand!=hand)&&
	(posPoint->winner[suit].hand!=partner[hand])&&
	(posPoint->winner[trump].hand!=hand)&&
	(posPoint->winner[trump].hand!=partner[hand])))) {
      if ((countPart==0)&&(posPoint->length[partner[hand]][trump]>0)) {
        if (((countRho>0)||(posPoint->length[rho[hand]][trump]==0))&&
	  ((countLho>0)||(posPoint->length[lho[hand]][trump]==0))) {
	  lowestQtricks=1;
	  if (1>=cutoff)
	    return 1;
	  suit++;
	  if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
	}
        else if ((countRho==0)&&(countLho==0)) {
	  if ((posPoint->rankInSuit[lho[hand]][trump] |
	     posPoint->rankInSuit[rho[hand]][trump]) <
	     posPoint->rankInSuit[partner[hand]][trump]) {
            lowestQtricks=1;

            rr=highestRank[posPoint->rankInSuit[partner[hand]][trump]];
	    if (rr!=0) {
	      posPoint->winRanks[depth][trump]|=bitMapRank[rr];
	      if (1>=cutoff)
		return 1;
	    }
          }
	  suit++;
	  if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
	}
	else if (countLho==0) {
          if (posPoint->rankInSuit[lho[hand]][trump] <
		posPoint->rankInSuit[partner[hand]][trump]) {
	    lowestQtricks=1;
	    for (rr=14; rr>=2; rr--) {
	      if ((posPoint->rankInSuit[partner[hand]][trump] &
		bitMapRank[rr])!=0) {
		posPoint->winRanks[depth][trump]|=bitMapRank[rr];
		break;
	      }
	    }
	    if (1>=cutoff)
	      return 1;
	  }
	  suit++;
	  if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
        }
	else if (countRho==0) {
          if (posPoint->rankInSuit[rho[hand]][trump] <
	    posPoint->rankInSuit[partner[hand]][trump]) {
	    lowestQtricks=1;
	    for (rr=14; rr>=2; rr--) {
	      if ((posPoint->rankInSuit[partner[hand]][trump] &
		bitMapRank[rr])!=0) {
		posPoint->winRanks[depth][trump]|=bitMapRank[rr];
		break;
	      }
	    }
	    if (1>=cutoff)
	      return 1;
	  }
	  suit++;
	  if ((trump!=4) && (suit==trump))
            suit++;
	  continue;
	}
      }
    }
    if (qtricks>=cutoff)
      return qtricks;
    if ((trump!=4) && (suit==trump)) {
      if (trump==0)
        suit=1;
      else
        suit=0;
    }
    else {
      suit++;
      if ((trump!=4) && (suit==trump))
        suit++;
    }
  }
  while (suit<=3);

  if (qtricks==0) {
    if ((trump==4)||(posPoint->winner[trump].hand==-1)) {
      for (ss=0; ss<=3; ss++) {
	if (posPoint->winner[ss].hand==-1)
	  continue;
	if (posPoint->length[hand][ss]>0) {
	  posPoint->winRanks[depth][ss]=
	    bitMapRank[posPoint->winner[ss].rank];
	}
      }
      if (thrp->nodeTypeStore[hand]!=MAXNODE)
        cutoff=target-posPoint->tricksMAX;
      else
        cutoff=posPoint->tricksMAX-target+(depth>>2)+2;

      if (1>=cutoff)
	return 0;
    }
  }

  *result=FALSE;
  return qtricks;
}


int QtricksLeadHandTrump(int hand, struct pos *posPoint, int cutoff, int depth,
      int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
      int countPart, int suit, int qtricks, int trump, int *res) {
	/* res=0		Continue with same suit.
	   res=1		Cutoff.
	   res=2		Continue with next suit. */


  *res=1;
  int qt=qtricks;
  if (((countLho!=0) || (lhoTrumpRanks==0)) && ((countRho!=0) || (rhoTrumpRanks==0))) {
    posPoint->winRanks[depth][suit]|=
            bitMapRank[posPoint->winner[suit].rank];
    qt++;
    if (qt>=cutoff)
      return qt;

    if ((countLho<=1)&&(countRho<=1)&&(countPart<=1)&&
       (lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
      qt+=countOwn-1;
      if (qt>=cutoff)
        return qt;
      *res=2;
      return qt;
    }
  }

  if (posPoint->secondBest[suit].hand==hand) {
    if ((lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
      posPoint->winRanks[depth][suit]|=
           bitMapRank[posPoint->secondBest[suit].rank];
      qt++;
      if (qt>=cutoff)
	return qt;
      if ((countLho<=2)&&(countRho<=2)&&(countPart<=2)) {
        qt+=countOwn-2;
	if (qt>=cutoff)
          return qt;
	*res=2;
	return qt;
      }
      /*else {
	aggr=0;
        for (k=0; k<=3; k++)
	  aggr|=posPoint->rankInSuit[k][suit];
        if (rel[aggr].absRank[3][suit].hand==hand) {
	  qt++;
	  posPoint->winRanks[depth][suit]|=
          bitMapRank[rel[aggr].absRank[3][suit].rank];
	  if (qt>=cutoff)
            return qt;
	  if ((countLho<=3)&&(countRho<=3)&&(countPart<=3)) {
	    qt+=countOwn-3;
	    if (qt>=cutoff)
              return qt;
	  }
	  *res=2;
	  return qt;
	}
      }*/
    }
  }
  else if ((posPoint->secondBest[suit].hand==partner[hand])
    &&(countOwn>1)&&(countPart>1)) {
    /* Second best at partner and suit length of own
	   hand and partner > 1 */
    if ((lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
      posPoint->winRanks[depth][suit]|=
           bitMapRank[posPoint->secondBest[suit].rank];
      qt++;
      if (qt>=cutoff)
	return qt;
      if ((countLho<=2)&&(countRho<=2)&&((countPart<=2)||(countOwn<=2))) {
        qt+=Max(countOwn-2, countPart-2);
	if (qt>=cutoff)
          return qt;
	*res=2;
	return qt;
      }
    }
  }
  *res=0;
  return qt;
}

int QtricksLeadHandNT(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int *lhoTrumpRanks, int *rhoTrumpRanks,
	int commPartner, int commSuit, int countOwn,
	int countPart, int suit, int qtricks, int trump, int *res) {
	/* res=0		Continue with same suit.
	   res=1		Cutoff.
	   res=2		Continue with next suit. */


  *res=1;
  int qt=qtricks;
  posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->winner[suit].rank];
  qt++;
  if (qt>=cutoff)
    return qt;
  if ((trump==suit) && ((!commPartner) || (suit!=commSuit))) {
  /*if (trump==suit) {*/
    (*lhoTrumpRanks)=Max(0, (*lhoTrumpRanks)-1);
    (*rhoTrumpRanks)=Max(0, (*rhoTrumpRanks)-1);
  }

  if ((countLho<=1)&&(countRho<=1)&&(countPart<=1)) {
    qt+=countOwn-1;
    if (qt>=cutoff)
      return qt;
    *res=2;
    return qt;
  }

  if (posPoint->secondBest[suit].hand==hand) {
    posPoint->winRanks[depth][suit]|=
      bitMapRank[posPoint->secondBest[suit].rank];
    qt++;
    if (qt>=cutoff)
      return qt;
    if ((trump==suit) && ((!commPartner) || (suit!=commSuit))) {
      (*lhoTrumpRanks)=Max(0, (*lhoTrumpRanks)-1);
      (*rhoTrumpRanks)=Max(0, (*rhoTrumpRanks)-1);
    }
    if ((countLho<=2)&&(countRho<=2)&&(countPart<=2)) {
      qt+=countOwn-2;
      if (qt>=cutoff)
        return qt;
      *res=2;
      return qt;
    }
    /*else {
      aggr=0;
      for (k=0; k<=3; k++)
	aggr|=posPoint->rankInSuit[k][suit];
      if (rel[aggr].absRank[3][suit].hand==hand) {
	qt++;
	posPoint->winRanks[depth][suit]|=
           bitMapRank[rel[aggr].absRank[3][suit].rank];
	if (qt>=cutoff)
          return qt;
	if ((countLho<=3)&&(countRho<=3)&&(countPart<=3)) {
	  qt+=countOwn-3;
	  if (qt>=cutoff)
            return qt;
	}
	*res=2;
	return qt;
      }
    }*/
  }
  else if ((posPoint->secondBest[suit].hand==partner[hand])
      &&(countOwn>1)&&(countPart>1)) {
     /* Second best at partner and suit length of own
	    hand and partner > 1 */
    posPoint->winRanks[depth][suit]|=
        bitMapRank[posPoint->secondBest[suit].rank];
    qt++;
    if (qt>=cutoff)
      return qt;
	if ((trump==suit) && ((!commPartner) || (suit!=commSuit))) {
      (*lhoTrumpRanks)=Max(0, (*lhoTrumpRanks)-1);
      (*rhoTrumpRanks)=Max(0, (*rhoTrumpRanks)-1);
    }
    if ((countLho<=2)&&(countRho<=2)&&((countPart<=2)||(countOwn<=2))) {
      qt+=Max(countOwn-2,countPart-2);
      if (qt>=cutoff)
        return qt;
      *res=2;
      return qt;
    }
    /*else if (countPart>2) {
      aggr=0;
      for (k=0; k<=3; k++)
	aggr|=posPoint->rankInSuit[k][suit];
      if (rel[aggr].absRank[3][suit].hand==hand) {
	qt++;
	posPoint->winRanks[depth][suit]|=
           bitMapRank[rel[aggr].absRank[3][suit].rank];
	if (qt>=cutoff)
          return qt;
	*res=2;
	return qt;
      }
    }*/
  }

  *res=0;
  return qt;
}


int QuickTricksPartnerHandTrump(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, 
        struct localVarType * thrp) {
	/* res=0		Continue with same suit.
	   res=1		Cutoff.
	   res=2		Continue with next suit. */


  *res=1;
  int qt=qtricks;
  if (((countLho!=0) || (lhoTrumpRanks==0)) && ((countRho!=0) || (rhoTrumpRanks==0))) {
    posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->winner[suit].rank];
    posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
    qt++;   /* A trick can be taken */
    if (qt>=cutoff)
      return qt;
    if ((countLho<=1)&&(countRho<=1)&&(countOwn<=1)&&(lhoTrumpRanks==0)&&
       (rhoTrumpRanks==0)) {
      qt+=countPart-1;
      if (qt>=cutoff)
        return qt;
      *res=2;
      return qt;
    }
  }

  if (posPoint->secondBest[suit].hand==partner[hand]) {
    /* Second best found in partners hand */
    if ((lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
        /* Opponents have no trump */
      posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->secondBest[suit].rank];
      posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
      qt++;
      if (qt>=cutoff)
	return qt;
      if ((countLho<=2)&&(countRho<=2)&&(countOwn<=2)) {
        qt+=countPart-2;
        if (qt>=cutoff)
          return qt;
	*res=2;
	return qt;
      }
    }
  }
  else if ((posPoint->secondBest[suit].hand==hand)&&(countPart>1)&&(countOwn>1)) {
     /* Second best found in own hand and suit lengths of own hand and partner > 1*/
    if ((lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
      /* Opponents have no trump */
      posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->secondBest[suit].rank];
      posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
      qt++;
      if (qt>=cutoff)
        return qt;
      if ((countLho<=2)&&(countRho<=2)&&((countOwn<=2)||(countPart<=2))) {
        qt+=Max(countPart-2,countOwn-2);
	if (qt>=cutoff)
          return qt;
	*res=2;
	return qt;
      }
    }
  }
  else if ((suit==commSuit)&&(posPoint->secondBest[suit].hand==lho[hand])&&
	  ((countLho>=2)||(lhoTrumpRanks==0))&&((countRho>=2)||(rhoTrumpRanks==0))) {
    unsigned short ranks=0;
    for (int k=0; k<=3; k++)
      ranks|=posPoint->rankInSuit[k][suit];
    if (thrp->abs[ranks].absRank[3][suit].hand==partner[hand]) {
	  posPoint->winRanks[depth][suit]|=bitMapRank[thrp->abs[ranks].absRank[3][suit].rank];
      posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
      qt++;
      if (qt>=cutoff)
	return qt;
      if ((countOwn<=2)&&(countLho<=2)&&(countRho<=2)&&(lhoTrumpRanks==0)&&(rhoTrumpRanks==0)) {
        qt+=countPart-2;
	if (qt>=cutoff)
	  return qt;
      }
    }
  }
  *res=0;
  return qt;
}


int QuickTricksPartnerHandNT(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, 
        int *res, struct localVarType * thrp) {

  *res=1;
  int qt=qtricks;

  posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->winner[suit].rank];
  posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];
  qt++;
  if (qt>=cutoff)
    return qt;
  if ((countLho<=1)&&(countRho<=1)&&(countOwn<=1)) {
    qt+=countPart-1;
    if (qt>=cutoff)
      return qt;
   *res=2;
    return qt;
  }

  if ((posPoint->secondBest[suit].hand==partner[hand])&&(1/*countPart>0*/)) {
       /* Second best found in partners hand */
    posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->secondBest[suit].rank];
    qt++;
    if (qt>=cutoff)
      return qt;
    if ((countLho<=2)&&(countRho<=2)&&(countOwn<=2)) {
      qt+=countPart-2;
      if (qt>=cutoff)
        return qt;
      *res=2;
      return qt;
    }
  }
  else if ((posPoint->secondBest[suit].hand==hand)
		  &&(countPart>1)&&(countOwn>1)) {
        /* Second best found in own hand and own and
		   partner's suit length > 1 */
    posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->secondBest[suit].rank];
    qt++;
    if (qt>=cutoff)
      return qt;
    if ((countLho<=2)&&(countRho<=2)&&((countOwn<=2)||(countPart<=2))) {
      qt+=Max(countPart-2,countOwn-2);
      if (qt>=cutoff)
	return qt;
      *res=2;
      return qt;
    }
  }
  else if ((suit==commSuit)&&(posPoint->secondBest[suit].hand==lho[hand])) {
    unsigned short ranks=0;
    for (int k=0; k<=3; k++)
      ranks|=posPoint->rankInSuit[k][suit];
    if (thrp->abs[ranks].absRank[3][suit].hand==partner[hand]) {
      posPoint->winRanks[depth][suit]|=bitMapRank[thrp->abs[ranks].absRank[3][suit].rank];
      qt++;
      if (qt>=cutoff)
	return qt;
      if ((countOwn<=2)&&(countLho<=2)&&(countRho<=2)) {
        qtricks+=countPart-2;
	if (qt>=cutoff)
	  return qt;
      }
    }
  }
  *res=0;
  return qt;
}


int LaterTricksMIN(struct pos *posPoint, int hand, int depth, int target,
	int trump, struct localVarType * thrp) {
  int hh, ss, k, h, sum=0;
  /*unsigned short aggr;*/

  if ((trump==4)||(posPoint->winner[trump].rank==0)) {
    for (ss=0; ss<=3; ss++) {
      hh=posPoint->winner[ss].hand;
      if (hh!=-1) {
        if (thrp->nodeTypeStore[hh]==MAXNODE)
          sum+=Max(posPoint->length[hh][ss], posPoint->length[partner[hh]][ss]);
      }
    }
    if ((posPoint->tricksMAX+sum<target)&&
      (sum>0)&&(depth>0)&&(depth!=thrp->iniDepth)) {
      if ((posPoint->tricksMAX+(depth>>2)<target)) {
	for (ss=0; ss<=3; ss++) {
	  if (posPoint->winner[ss].hand==-1)
	    posPoint->winRanks[depth][ss]=0;
          else if (thrp->nodeTypeStore[posPoint->winner[ss].hand]==MINNODE) {
            if ((posPoint->rankInSuit[partner[posPoint->winner[ss].hand]][ss]==0)&&
		(posPoint->rankInSuit[lho[posPoint->winner[ss].hand]][ss]==0)&&
		(posPoint->rankInSuit[rho[posPoint->winner[ss].hand]][ss]==0))
	      posPoint->winRanks[depth][ss]=0;
	    else
              posPoint->winRanks[depth][ss]=bitMapRank[posPoint->winner[ss].rank];
	  }
          else
            posPoint->winRanks[depth][ss]=0;
	}
	return FALSE;
      }
    }
  }
  else if ((trump!=4) && (posPoint->winner[trump].rank!=0) &&
    (thrp->nodeTypeStore[posPoint->winner[trump].hand]==MINNODE)) {
    if ((posPoint->length[hand][trump]==0)&&
      (posPoint->length[partner[hand]][trump]==0)) {
      if (((posPoint->tricksMAX+(depth>>2)+1-
	  Max(posPoint->length[lho[hand]][trump],
	  posPoint->length[rho[hand]][trump]))<target)
          &&(depth>0)&&(depth!=thrp->iniDepth)) {
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=0;
	return FALSE;
      }
    }
    else if (((posPoint->tricksMAX+(depth>>2))<target)&&
      (depth>0)&&(depth!=thrp->iniDepth)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
      posPoint->winRanks[depth][trump]=
	  bitMapRank[posPoint->winner[trump].rank];
      return FALSE;
    }
    else {
      hh=posPoint->secondBest[trump].hand;
      if (hh!=-1) {
        if ((thrp->nodeTypeStore[hh]==MINNODE)&&(posPoint->secondBest[trump].rank!=0))  {
          if (((posPoint->length[hh][trump]>1) ||
            (posPoint->length[partner[hh]][trump]>1))&&
            ((posPoint->tricksMAX+(depth>>2)-1)<target)&&(depth>0)
	     &&(depth!=thrp->iniDepth)) {
            for (ss=0; ss<=3; ss++)
              posPoint->winRanks[depth][ss]=0;
	    posPoint->winRanks[depth][trump]=
              bitMapRank[posPoint->secondBest[trump].rank] ;
	         return FALSE;
	  }
        }
      }
    }
  }
  else if (trump!=4) {
    hh=posPoint->secondBest[trump].hand;
    if (hh!=-1) {
      if ((thrp->nodeTypeStore[hh]==MINNODE)&&
        (posPoint->length[hh][trump]>1)) {
	if (posPoint->winner[trump].hand==rho[hh]) {
          if (((posPoint->tricksMAX+(depth>>2))<target)&&
            (depth>0)&&(depth!=thrp->iniDepth)) {
            for (ss=0; ss<=3; ss++)
              posPoint->winRanks[depth][ss]=0;
	        posPoint->winRanks[depth][trump]=
              bitMapRank[posPoint->secondBest[trump].rank];
            return FALSE;
	  }
	}
	else {
	  unsigned short aggr=0;
	  for (k=0; k<=3; k++)
	    aggr|=posPoint->rankInSuit[k][trump];
	  h=thrp->abs[aggr].absRank[3][trump].hand;
	  if (h!=-1) {
	    if ((thrp->nodeTypeStore[h]==MINNODE)&&
	      ((posPoint->tricksMAX+(depth>>2))<target)&&
              (depth>0)&&(depth!=thrp->iniDepth)) {
              for (ss=0; ss<=3; ss++)
                posPoint->winRanks[depth][ss]=0;
	      posPoint->winRanks[depth][trump]=
		bitMapRank[thrp->abs[aggr].absRank[3][trump].rank];
              return FALSE;
	    }
	  }
	}
      }
    }
  }
  return TRUE;
}


int LaterTricksMAX(struct pos *posPoint, int hand, int depth, int target,
	int trump, struct localVarType * thrp) {
  int hh, ss, k, h, sum=0;
  /*unsigned short aggr;*/

  if ((trump==4)||(posPoint->winner[trump].rank==0)) {
    for (ss=0; ss<=3; ss++) {
      hh=posPoint->winner[ss].hand;
      if (hh!=-1) {
        if (thrp->nodeTypeStore[hh]==MINNODE)
          sum+=Max(posPoint->length[hh][ss], posPoint->length[partner[hh]][ss]);
      }
    }
    if ((posPoint->tricksMAX+(depth>>2)+1-sum>=target)&&
	(sum>0)&&(depth>0)&&(depth!=thrp->iniDepth)) {
      if ((posPoint->tricksMAX+1>=target)) {
	for (ss=0; ss<=3; ss++) {
	  if (posPoint->winner[ss].hand==-1)
	    posPoint->winRanks[depth][ss]=0;
          else if (thrp->nodeTypeStore[posPoint->winner[ss].hand]==MAXNODE) {
	    if ((posPoint->rankInSuit[partner[posPoint->winner[ss].hand]][ss]==0)&&
		(posPoint->rankInSuit[lho[posPoint->winner[ss].hand]][ss]==0)&&
		(posPoint->rankInSuit[rho[posPoint->winner[ss].hand]][ss]==0))
		posPoint->winRanks[depth][ss]=0;
	    else
              posPoint->winRanks[depth][ss]=bitMapRank[posPoint->winner[ss].rank];
	  }
          else
            posPoint->winRanks[depth][ss]=0;
	}
	return TRUE;
      }
    }
  }
  else if ((trump!=4) && (posPoint->winner[trump].rank!=0) &&
    (thrp->nodeTypeStore[posPoint->winner[trump].hand]==MAXNODE)) {
    if ((posPoint->length[hand][trump]==0)&&
      (posPoint->length[partner[hand]][trump]==0)) {
      if (((posPoint->tricksMAX+Max(posPoint->length[lho[hand]][trump],
        posPoint->length[rho[hand]][trump]))>=target)
        &&(depth>0)&&(depth!=thrp->iniDepth)) {
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=0;
	return TRUE;
      }
    }
    else if (((posPoint->tricksMAX+1)>=target)
      &&(depth>0)&&(depth!=thrp->iniDepth)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
      posPoint->winRanks[depth][trump]=
	  bitMapRank[posPoint->winner[trump].rank];
      return TRUE;
    }
    else {
      hh=posPoint->secondBest[trump].hand;
      if (hh!=-1) {
        if ((thrp->nodeTypeStore[hh]==MAXNODE)&&(posPoint->secondBest[trump].rank!=0))  {
          if (((posPoint->length[hh][trump]>1) ||
            (posPoint->length[partner[hh]][trump]>1))&&
            ((posPoint->tricksMAX+2)>=target)&&(depth>0)
	    &&(depth!=thrp->iniDepth)) {
            for (ss=0; ss<=3; ss++)
              posPoint->winRanks[depth][ss]=0;
	    posPoint->winRanks[depth][trump]=
            bitMapRank[posPoint->secondBest[trump].rank];
	    return TRUE;
	  }
 	}
      }
    }
  }

  else if (trump!=4) {
    hh=posPoint->secondBest[trump].hand;
    if (hh!=-1) {
      if ((thrp->nodeTypeStore[hh]==MAXNODE)&&
        (posPoint->length[hh][trump]>1)) {
	if (posPoint->winner[trump].hand==rho[hh]) {
          if (((posPoint->tricksMAX+1)>=target)&&(depth>0)
	     &&(depth!=thrp->iniDepth)) {
            for (ss=0; ss<=3; ss++)
              posPoint->winRanks[depth][ss]=0;
	    posPoint->winRanks[depth][trump]=
              bitMapRank[posPoint->secondBest[trump].rank] ;
            return TRUE;
	  }
	}
	else {
	  unsigned short aggr=0;
	  for (k=0; k<=3; k++)
	    aggr|=posPoint->rankInSuit[k][trump];
	  h=thrp->abs[aggr].absRank[3][trump].hand;
	  if (h!=-1) {
	    if ((thrp->nodeTypeStore[h]==MAXNODE)&&
		((posPoint->tricksMAX+1)>=target)&&(depth>0)
		&&(depth!=thrp->iniDepth)) {
              for (ss=0; ss<=3; ss++)
                posPoint->winRanks[depth][ss]=0;
	      posPoint->winRanks[depth][trump]=
		bitMapRank[thrp->abs[aggr].absRank[3][trump].rank];
              return TRUE;
	    }
	  }
	}
      }
    }
  }
  return FALSE;
}


int MoveGen(struct pos * posPoint, int depth, int trump, struct movePlyType *mply, 
  struct localVarType * thrp) {
  int k, state=MOVESVALID;

  int WeightAllocTrump(struct pos * posPoint, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, int trump, struct localVarType * thrp);
  int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, struct localVarType * thrp);

  for (k=0; k<4; k++)
    thrp->lowestWin[depth][k]=0;

  int m=0;
  int r=posPoint->handRelFirst;
  int first=posPoint->first[depth];
  int q=handId(first, r);

  if (r!=0) {
    int s=thrp->movePly[depth+r].current;             /* Current move of first hand */
    int t=thrp->movePly[depth+r].move[s].suit;        /* Suit played by first hand */
    unsigned short ris=posPoint->rankInSuit[q][t];

    if (ris!=0) {
    /* Not first hand and not void in suit */
      k=14;  
      while (k>=2) {
        if ((ris & bitMapRank[k])&&(state==MOVESVALID)) {
           /* Only first move in sequence is generated */
	  mply->move[m].suit=t;
	  mply->move[m].rank=k;
	  mply->move[m].sequence=0;
          m++;
          state=MOVESLOCKED;
        }
        else if (state==MOVESLOCKED) {
          if (ris & bitMapRank[k])
	     /* If the card is in own hand */
	    mply->move[m-1].sequence|=bitMapRank[k];
	  else if ((posPoint->removedRanks[t] & bitMapRank[k])==0)
	    /* If the card still exists and it is not in own hand */
            state=MOVESVALID;
	}
        k--;
      }
      if (m!=1) {
        if ((trump!=4)&&(posPoint->winner[trump].rank!=0)) {
          for (k=0; k<=m-1; k++)
	    mply->move[k].weight=WeightAllocTrump(posPoint,
              &(mply->move[k]), depth, ris, trump, thrp);
        }
        else {
	  for (k=0; k<=m-1; k++)
	    mply->move[k].weight=WeightAllocNT(posPoint,
              &(mply->move[k]), depth, ris, thrp);
        }
      }

      mply->last=m-1;
      if (m!=1)
        MergeSort(m, mply->move);
      if (depth!=thrp->iniDepth)
        return m;
      else {
        m=AdjustMoveList(thrp);
        return m;
      }
    }
  }

  /* First hand or void in suit */
  for (int suit=0; suit<=3; suit++)  {
    k=14;  state=MOVESVALID;
    while (k>=2) {
      if ((posPoint->rankInSuit[q][suit] & bitMapRank[k])&&
            (state==MOVESVALID)) {
           /* Only first move in sequence is generated */
	mply->move[m].suit=suit;
	mply->move[m].rank=k;
	mply->move[m].sequence=0;
        m++;
        state=MOVESLOCKED;
      }
      else if (state==MOVESLOCKED) {
        if (posPoint->rankInSuit[q][suit] & bitMapRank[k])
	     /* If the card is in own hand */
	    mply->move[m-1].sequence|=bitMapRank[k];
	  else if ((posPoint->removedRanks[suit] & bitMapRank[k])==0)
	    /* If the card still exists and it is not in own hand */
            state=MOVESVALID;
      }
      k--;
    }
  }

  if ((trump!=4)&&(posPoint->winner[trump].rank!=0)) {
    for (k=0; k<=m-1; k++)
      mply->move[k].weight=WeightAllocTrump(posPoint,
          &(mply->move[k]), depth, 0/*ris*/, trump, thrp);
  }
  else {
    for (k=0; k<=m-1; k++)
      mply->move[k].weight=WeightAllocNT(posPoint,
          &(mply->move[k]), depth, 0/*ris*/, thrp);
  }

  mply->last=m-1;
  if (m!=1)
    MergeSort(m, mply->move);

  if (depth!=thrp->iniDepth)
    return m;
  else {
    m=AdjustMoveList(thrp);
    return m;
  }
}


int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
  unsigned short notVoidInSuit, struct localVarType * thrp) {
  int weight=0, k, l, kk, ll, suitAdd=0, leadSuit;
  int suitWeightDelta;
  int thirdBestHand;
  int winMove=FALSE;  /* If winMove is TRUE, current move can win the current trick. */
  unsigned short suitCount, suitCountLH, suitCountRH;
  int countLH, countRH;

  int first=posPoint->first[depth];
  int q=handId(first, posPoint->handRelFirst);
  int suit=mp->suit;
  unsigned short aggr=0;
  for (int m=0; m<=3; m++)
    aggr|=posPoint->rankInSuit[m][suit];
  int rRank=relRanks->relRank[aggr][mp->rank];
  

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
        winMove=TRUE;	/* May also have 2nd best, but this card will not be searched. */		   
      else if (posPoint->rankInSuit[partner[first]][suit] >
	(posPoint->rankInSuit[lho[first]][suit] |
	   posPoint->rankInSuit[rho[first]][suit])) {
	winMove=TRUE;
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

	thirdBestHand=thrp->abs[aggr].absRank[3][suit].hand;

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
          winMove=TRUE;
	else if (posPoint->rankInSuit[rho[first]][suit]>
	   (bitMapRank[posPoint->move[depth+1].rank] |
	    posPoint->rankInSuit[partner[first]][suit])) 
          winMove=TRUE;
      }
      else {
	/* Side with highest rank in leadSuit wins */

	if (posPoint->rankInSuit[rho[first]][leadSuit] >
           (posPoint->rankInSuit[partner[first]][leadSuit] |
            bitMapRank[posPoint->move[depth+1].rank]))
          winMove=TRUE;			   			  
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
	  winMove=TRUE;
      }	
      else if (posPoint->high[depth+1]==first) {
	if (posPoint->rankInSuit[rho[first]][leadSuit]
	      < bitMapRank[posPoint->move[depth+2].rank])	
	  winMove=TRUE;
	
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
  int winMove=FALSE;	/* If winMove is true, current move can win the current trick. */
  unsigned short suitCount, suitCountLH, suitCountRH;
  int countLH, countRH;

  int first=posPoint->first[depth];
  int q=handId(first, posPoint->handRelFirst);
  int suit=mp->suit;
  unsigned short aggr=0;
  for (int m=0; m<=3; m++)
    aggr|=posPoint->rankInSuit[m][suit];
  int rRank=relRanks->relRank[aggr][mp->rank];

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
	      winMove=TRUE;
	  }
	  else if (((posPoint->length[lho[first]][suit]!=0)||
               (posPoint->rankInSuit[partner[first]][trump]>
                posPoint->rankInSuit[lho[first]][trump]))&&
		((posPoint->length[rho[first]][suit]!=0)||
		(posPoint->rankInSuit[partner[first]][trump]>
		 posPoint->rankInSuit[rho[first]][trump])))
	    winMove=TRUE;
	}
        else 
          winMove=TRUE;			   
      }
      else if (posPoint->rankInSuit[partner[first]][suit] >
	(posPoint->rankInSuit[lho[first]][suit] |
	posPoint->rankInSuit[rho[first]][suit])) {
	if (suit!=trump) {
	  if (((posPoint->length[lho[first]][suit]!=0)||
	      (posPoint->length[lho[first]][trump]==0))&&
	      ((posPoint->length[rho[first]][suit]!=0)||
	      (posPoint->length[rho[first]][trump]==0)))
	    winMove=TRUE;
	}
	else
	  winMove=TRUE;
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
	      winMove=TRUE;
	  }
	  else if ((posPoint->length[lho[first]][suit]==0)&&
              (posPoint->length[lho[first]][trump]!=0)) {
	    if (posPoint->rankInSuit[partner[first]][trump]
		    > posPoint->rankInSuit[lho[first]][trump])
	        winMove=TRUE;
	  }	
	  else if ((posPoint->length[rho[first]][suit]==0)&&
              (posPoint->length[rho[first]][trump]!=0)) {
	    if (posPoint->rankInSuit[partner[first]][trump]
		    > posPoint->rankInSuit[rho[first]][trump])
	      winMove=TRUE;
	  }	
          else
	    winMove=TRUE;
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

	thirdBestHand=thrp->abs[aggr].absRank[3][suit].hand;

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
	      winMove=TRUE;
	    else if ((posPoint->length[rho[first]][suit]==0)
                &&(posPoint->length[rho[first]][trump]!=0)
                &&(posPoint->rankInSuit[rho[first]][trump]>
                 posPoint->rankInSuit[partner[first]][trump]))
	      winMove=TRUE;
	  }
          else
            winMove=TRUE;
        }
	else if (posPoint->rankInSuit[rho[first]][suit]>
	    (bitMapRank[posPoint->move[depth+1].rank] |
	    posPoint->rankInSuit[partner[first]][suit])) {	 
	  if (suit!=trump) {
	    if ((posPoint->length[partner[first]][suit]!=0)||
		  (posPoint->length[partner[first]][trump]==0))
	      winMove=TRUE;
	  }
          else
            winMove=TRUE;
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
		winMove=TRUE;
	      else if (posPoint->rankInSuit[rho[first]][trump]
                  > posPoint->rankInSuit[partner[first]][trump])
		winMove=TRUE;
	    }	  
	  }
	}	
	else {   /* winnerHand is partner to first */
	  if (suit!=trump) {
	    if ((posPoint->length[rho[first]][suit]==0)&&
		  (posPoint->length[rho[first]][trump]!=0))
	       winMove=TRUE;
	  }  
	}
      }
      else {

	 /* Leading suit differs from suit played by LHO */

	if (suit==trump) {
	  if (posPoint->length[partner[first]][leadSuit]!=0)
	    winMove=TRUE;
	  else if (bitMapRank[mp->rank]>
		posPoint->rankInSuit[partner[first]][trump]) 
	    winMove=TRUE;
	  else if ((posPoint->length[rho[first]][leadSuit]==0)
              &&(posPoint->length[rho[first]][trump]!=0)&&
              (posPoint->rankInSuit[rho[first]][trump] >
              posPoint->rankInSuit[partner[first]][trump]))
            winMove=TRUE;
        }	
        else if (leadSuit!=trump) {

          /* Neither suit nor leadSuit is trump */

          if (posPoint->length[partner[first]][leadSuit]!=0) {
            if (posPoint->rankInSuit[rho[first]][leadSuit] >
              (posPoint->rankInSuit[partner[first]][leadSuit] |
              bitMapRank[posPoint->move[depth+1].rank]))
              winMove=TRUE;
	    else if ((posPoint->length[rho[first]][leadSuit]==0)
		  &&(posPoint->length[rho[first]][trump]!=0))
	      winMove=TRUE;
	  }

	  /* Partner to leading hand is void in leading suit */

	  else if ((posPoint->length[rho[first]][leadSuit]==0)
		&&(posPoint->rankInSuit[rho[first]][trump]>
	      posPoint->rankInSuit[partner[first]][trump]))
	    winMove=TRUE;
	  else if ((posPoint->length[partner[first]][trump]==0)
	      &&(posPoint->rankInSuit[rho[first]][leadSuit] >
		bitMapRank[posPoint->move[depth+1].rank]))
	    winMove=TRUE;
        }
        else {
	  /* Either no trumps or leadSuit is trump, side with
		highest rank in leadSuit wins */
	  if (posPoint->rankInSuit[rho[first]][leadSuit] >
            (posPoint->rankInSuit[partner[first]][leadSuit] |
             bitMapRank[posPoint->move[depth+1].rank]))
            winMove=TRUE;			   
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
	      winMove=TRUE;
	  }	
	  else if (bitMapRank[mp->rank] >
	    posPoint->rankInSuit[rho[first]][suit])
	    winMove=TRUE;
	}
	else {  /* Suit is trump */
	  if (posPoint->length[rho[first]][leadSuit]==0) {
	    if (bitMapRank[mp->rank] >
		  posPoint->rankInSuit[rho[first]][trump])
	      winMove=TRUE;
	  }
	  else
	    winMove=TRUE;
	}
      }	
      else if (posPoint->high[depth+1]==first) {
	if (posPoint->length[rho[first]][leadSuit]!=0) {
	  if (posPoint->rankInSuit[rho[first]][leadSuit]
		 < bitMapRank[posPoint->move[depth+2].rank])	
	    winMove=TRUE;
	}
	else if (leadSuit==trump)
          winMove=TRUE;
	else if ((leadSuit!=trump) &&
	    (posPoint->length[rho[first]][trump]==0))
	  winMove=TRUE;
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


/* Shell-1 */
/* K&R page 62: */
/*void shellSort(int n, int depth) {
  int gap, i, j;
  struct moveType temp;

  if (n==2) {
    if (movePly[depth].move[0].weight<movePly[depth].move[1].weight) {
      temp=movePly[depth].move[0];
      movePly[depth].move[0]=movePly[depth].move[1];
      movePly[depth].move[1]=temp;
      return;
    }
    else
      return;
  }
  for (gap=n>>1; gap>0; gap>>=1)
    for (i=gap; i<n; i++)
      for (j=i-gap; j>=0 && movePly[depth].move[j].weight<
         movePly[depth].move[j+gap].weight; j-=gap) {
        temp=movePly[depth].move[j];
        movePly[depth].move[j]=movePly[depth].move[j+gap];
        movePly[depth].move[j+gap]=temp;
      }
} */

/* Shell-2 */
/*void shellSort(int n, int depth)
{
  int i, j, increment;
  struct moveType temp;

  if (n==2) {
    if (movePly[depth].move[0].weight<movePly[depth].move[1].weight) {
      temp=movePly[depth].move[0];
      movePly[depth].move[0]=movePly[depth].move[1];
      movePly[depth].move[1]=temp;
      return;
    }
    else
      return;
  }
  increment = 3;
  while (increment > 0)
  {
    for (i=0; i < n; i++)
    {
      j = i;
      temp = movePly[depth].move[i];
      while ((j >= increment) && (movePly[depth].move[j-increment].weight < temp.weight))
      {
        movePly[depth].move[j] = movePly[depth].move[j - increment];
        j = j - increment;
      }
      movePly[depth].move[j] = temp;
    }
    if ((increment>>1) != 0)
      increment>>=1;
    else if (increment == 1)
      increment = 0;
    else
      increment = 1;
  }
} */


/* Insert-1 */
/*void InsertSort(int n, int depth, struct movePlyType *mply, int thrId) {
  int i, j;
  struct moveType a, temp;

  if (n==2) {
    if (mply->move[0].weight<mply->move[1].weight) {
      temp=mply->move[0];
      mply->move[0]=mply->move[1];
      mply->move[1]=temp;
      return;
    }
    else
      return;
  }

  a=mply->move[0];
  for (i=1; i<=n-1; i++)
    if (mply->move[i].weight>a.weight) {
      temp=a;
      a=mply->move[i];
      mply->move[i]=temp;
    }
  mply->move[0]=a;
  for (i=2; i<=n-1; i++) {
    j=i;
    a=mply->move[i];
    while (a.weight>mply->move[j-1].weight) {
      mply->move[j]=mply->move[j-1];
      j--;
    }
    mply->move[j]=a;
  }
} */


/* Insert-2 */
/*void InsertSort(int n, int depth) {
  int i, j;
  struct moveType a;

  if (n==2) {
    if (movePly[depth].move[0].weight<movePly[depth].move[1].weight) {
      a=movePly[depth].move[0];
      movePly[depth].move[0]=movePly[depth].move[1];
      movePly[depth].move[1]=a;
      return;
    }
    else
      return;
  }
  for (j=1; j<=n-1; j++) {
    a=movePly[depth].move[j];
    i=j-1;
    while ((i>=0)&&(movePly[depth].move[i].weight<a.weight)) {
      movePly[depth].move[i+1]=movePly[depth].move[i];
      i--;
    }
    movePly[depth].move[i+1]=a;
  }
}  */

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

  for (k=1; k<=13; k++) {
    suit=thrp->forbiddenMoves[k].suit;
    rank=thrp->forbiddenMoves[k].rank;
    for (r=0; r<=thrp->movePly[thrp->iniDepth].last; r++) {
      if ((suit==thrp->movePly[thrp->iniDepth].move[r].suit)&&
        (rank!=0)&&(rank==thrp->movePly[thrp->iniDepth].move[r].rank)) {
        /* For the forbidden move r: */
        for (n=r; n<=thrp->movePly[thrp->iniDepth].last; n++)
          thrp->movePly[thrp->iniDepth].move[n]=
		  thrp->movePly[thrp->iniDepth].move[n+1];
        thrp->movePly[thrp->iniDepth].last--;
      }
    }
  }
  return thrp->movePly[thrp->iniDepth].last+1;
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

int InvWinMask(int mask) {

  switch (mask) {
    case 0x01000000: return 1;
    case 0x00400000: return 2;
    case 0x00100000: return 3;
    case 0x00040000: return 4;
    case 0x00010000: return 5;
    case 0x00004000: return 6;
    case 0x00001000: return 7;
    case 0x00000400: return 8;
    case 0x00000100: return 9;
    case 0x00000040: return 10;
    case 0x00000010: return 11;
    case 0x00000004: return 12;
    case 0x00000001: return 13;
    default: return 0;
  }
}


inline int WinningMove(struct moveType * mvp1, struct moveType * mvp2, int trump) {
/* Return TRUE if move 1 wins over move 2, with the assumption that
move 2 is the presently winning card of the trick */

  if (mvp1->suit==mvp2->suit) {
    if ((mvp1->rank)>(mvp2->rank))
      return TRUE;
    else
      return FALSE;
  }
  else if ((mvp1->suit)==trump)
    return TRUE;
  else
    return FALSE;
}


inline int WinningMoveNT(struct moveType * mvp1, struct moveType * mvp2) {
/* Return TRUE if move 1 wins over move 2, with the assumption that
move 2 is the presently winning card of the trick */

  if (mvp1->suit==mvp2->suit) {
    if ((mvp1->rank)>(mvp2->rank))
      return TRUE;
    else
      return FALSE;
  }
  else
    return FALSE;
}


struct nodeCardsType * CheckSOP(struct pos * posPoint, struct nodeCardsType
  * nodep, int target, int tricks, int * result, int *value, 
    struct localVarType * thrp) {
    /* Check SOP if it matches the
    current position. If match, pointer to the SOP node is returned and
    result is set to TRUE, otherwise pointer to SOP node is returned
    and result set to FALSE. */

  /* 07-04-22 */
  if (thrp->nodeTypeStore[0]==MAXNODE) {
    if (nodep->lbound==-1) {  /* This bound values for
      this leading hand has not yet been determined */
      *result=FALSE;
      return nodep;
    }
    else if ((posPoint->tricksMAX + nodep->lbound)>=target) {
      *value=TRUE;
      *result=TRUE;
      return nodep;
    }
    else if ((posPoint->tricksMAX + nodep->ubound)<target) {
      *value=FALSE;
      *result=TRUE;
      return nodep;
    }
  }
  else {
    if (nodep->ubound==-1) {  /* This bound values for
      this leading hand has not yet been determined */
      *result=FALSE;
      return nodep;
    }
    else if ((posPoint->tricksMAX + (tricks + 1 - nodep->ubound))>=target) {
      *value=TRUE;
      *result=TRUE;
      return nodep;
    }
    else if ((posPoint->tricksMAX + (tricks + 1 - nodep->lbound))<target) {
      *value=FALSE;
      *result=TRUE;
      return nodep;
    }
  }

  *result=FALSE;
  return nodep;          /* No matching node was found */
}


struct nodeCardsType * UpdateSOP(struct pos * posPoint, struct nodeCardsType
  * nodep) {
    /* Update SOP node with new values for upper and lower
	  bounds. */

  if ((posPoint->lbound > nodep->lbound) ||
		(nodep->lbound==-1))
    nodep->lbound=posPoint->lbound;
  if ((posPoint->ubound < nodep->ubound) ||
		(nodep->ubound==-1))
    nodep->ubound=posPoint->ubound;

  if (posPoint->bestMoveRank!=0) {
    nodep->bestMoveSuit=posPoint->bestMoveSuit;
      nodep->bestMoveRank=posPoint->bestMoveRank;
  }

  return nodep;
}


struct nodeCardsType * FindSOP(
  struct pos 		* posPoint,
  struct winCardType 	* nodeP, 
  int 			firstHand,
  int 			target, 
  int 			tricks, 
  int 			* valp, 
  struct localVarType * thrp)
{
  /* credit Soren Hein */  

  struct nodeCardsType * sopP;
  struct winCardType * np;
  int res;

  np = nodeP;
  int s = 0;

  while (np)
  {
    if ((np->winMask & posPoint->orderSet[s]) == np->orderSet)
    {
      /* Winning rank set fits position */
      if (s != 3)
      {
        np = np->nextWin;
        s++;
	continue;
      }

      sopP = CheckSOP(posPoint, np->first, target, tricks, 
                      &res, valp, thrp);
      if (res)
        return sopP;
    }

    while (np->next == NULL)
    {
      np = np->prevWin;
      s--;
      if (np == NULL) /* Previous node is header node? */
        return NULL;
    }
    np = np->next;
  }
  return NULL;
}


struct nodeCardsType * BuildPath(struct pos * posPoint,
  struct posSearchType *nodep, int * result, struct localVarType * thrp) {
  /* If result is TRUE, a new SOP has been created and BuildPath returns a
  pointer to it. If result is FALSE, an existing SOP is used and BuildPath
  returns a pointer to the SOP */

  int found;
  struct winCardType * np, * p2, /* * sp2,*/ * nprev, * fnp, *pnp;
  struct winCardType temp;
  struct nodeCardsType * sopP=0, * p/*, * sp*/;

  np=nodep->posSearchPoint;
  nprev=NULL;
  int suit=0;

  /* If winning node has a card that equals the next winning card deduced
  from the position, then there already exists a (partial) path */

  if (np==NULL) {   /* There is no winning list created yet */
   /* Create winning nodes */
    p2=&(thrp->winCards[thrp->winSetSize]);
    AddWinSet(thrp);
    p2->next=NULL;
    p2->nextWin=NULL;
    p2->prevWin=NULL;
    nodep->posSearchPoint=p2;
    p2->winMask=posPoint->winMask[suit];
    p2->orderSet=posPoint->winOrderSet[suit];
    p2->first=NULL;
    np=p2;           /* Latest winning node */
    suit++;
    while (suit<4) {
      p2=&(thrp->winCards[thrp->winSetSize]);
      AddWinSet(thrp);
      np->nextWin=p2;
      p2->prevWin=np;
      p2->next=NULL;
      p2->nextWin=NULL;
      p2->winMask=posPoint->winMask[suit];
      p2->orderSet=posPoint->winOrderSet[suit];
      p2->first=NULL;
      np=p2;         /* Latest winning node */
      suit++;
    }
    p=&(thrp->nodeCards[thrp->nodeSetSize]);
    AddNodeSet(thrp);
    np->first=p;
    *result=TRUE;
    return p;
  }
  else {   /* Winning list exists */
    while (1) {   /* Find all winning nodes that correspond to current
		position */
      found=FALSE;
      while (1) {    /* Find node amongst alternatives */
	if ((np->winMask==posPoint->winMask[suit])&&
	   (np->orderSet==posPoint->winOrderSet[suit])) {
	   /* Part of path found */
	  found=TRUE;
	  nprev=np;
	  break;
	}
	if (np->next!=NULL)
	  np=np->next;
	else
	  break;
      }
      if (found) {
	suit++;
	if (suit>3) {
	  sopP=UpdateSOP(posPoint, np->first);

	  if (np->prevWin!=NULL) {
	    pnp=np->prevWin;
	    fnp=pnp->nextWin;
	  }
	  else
	    fnp=nodep->posSearchPoint;

	  temp.orderSet=np->orderSet;
	  temp.winMask=np->winMask;
	  temp.first=np->first;
	  temp.nextWin=np->nextWin;
	  np->orderSet=fnp->orderSet;
	  np->winMask=fnp->winMask;
	  np->first=fnp->first;
	  np->nextWin=fnp->nextWin;
	  fnp->orderSet=temp.orderSet;
	  fnp->winMask=temp.winMask;
	  fnp->first=temp.first;
	  fnp->nextWin=temp.nextWin;

	  *result=FALSE;
	  return sopP;
	}
	else {
	  np=np->nextWin;       /* Find next winning node  */
	  continue;
	}
      }
      else
	break;                    /* Node was not found */
    }               /* End outer while */

    /* Create additional node, coupled to existing node(s) */
    p2=&(thrp->winCards[thrp->winSetSize]);
    AddWinSet(thrp);
    p2->prevWin=nprev;
    if (nprev!=NULL) {
      p2->next=nprev->nextWin;
      nprev->nextWin=p2;
    }
    else {
      p2->next=nodep->posSearchPoint;
      nodep->posSearchPoint=p2;
    }
    p2->nextWin=NULL;
    p2->winMask=posPoint->winMask[suit];
    p2->orderSet=posPoint->winOrderSet[suit];
    p2->first=NULL;
    np=p2;          /* Latest winning node */
    suit++;

    /* Rest of path must be created */
    while (suit<4) {
      p2=&(thrp->winCards[thrp->winSetSize]);
      AddWinSet(thrp);
      np->nextWin=p2;
      p2->prevWin=np;
      p2->next=NULL;
      p2->winMask=posPoint->winMask[suit];
      p2->orderSet=posPoint->winOrderSet[suit];
      p2->first=NULL;
      p2->nextWin=NULL;
      np=p2;         /* Latest winning node */
      suit++;
    }

  /* All winning nodes in SOP have been traversed and new nodes created */
    p=&(thrp->nodeCards[thrp->nodeSetSize]);
    AddNodeSet(thrp);
    np->first=p;
    *result=TRUE;
    return p;
  }
}


struct posSearchType * SearchLenAndInsert(struct posSearchType
	* rootp, long long key, int insertNode, int *result, 
       struct localVarType * thrp) {
/* Search for node which matches with the suit length combination
   given by parameter key. If no such node is found, NULL is
  returned if parameter insertNode is FALSE, otherwise a new
  node is inserted with suitLengths set to key, the pointer to
  this node is returned.
  The algorithm used is defined in Knuth "The art of computer
  programming", vol.3 "Sorting and searching", 6.2.2 Algorithm T,
  page 424. */

  struct posSearchType *np, *p, *sp;

  if (insertNode)
    sp=&(thrp->posSearch[thrp->lenSetSize]);

  np=rootp;
  while (1) {
    if (key==np->suitLengths) {
      *result=TRUE;
      return np;
    }
    else if (key < np->suitLengths) {
      if (np->left!=NULL)
        np=np->left;
      else if (insertNode) {
	p=sp;
	AddLenSet(thrp);
	np->left=p;
	p->posSearchPoint=NULL;
	p->suitLengths=key;
	p->left=NULL; p->right=NULL;
	*result=TRUE;
	return p;
      }
      else {
	*result=FALSE;
        return NULL;
      }
    }
    else {      /* key > suitLengths */
      if (np->right!=NULL)
        np=np->right;
      else if (insertNode) {
	p=sp;
	AddLenSet(thrp);
	np->right=p;
	p->posSearchPoint=NULL;
	p->suitLengths=key;
	p->left=NULL; p->right=NULL;
	*result=TRUE;
	return p;
      }
      else {
	*result=FALSE;
        return NULL;
      }
    }
  }
}



void BuildSOP(struct pos * posPoint, long long suitLengths, int tricks, int firstHand, int target,
  int depth, int scoreFlag, int score, struct localVarType * thrp) {
  int hh, res, wm;
  unsigned short int w;
  unsigned short int temp[4][4];
  unsigned short int aggr[4];
  struct nodeCardsType * cardsP;
  struct posSearchType * np;


  for (int ss=0; ss<=3; ss++) {
    w=posPoint->winRanks[depth][ss];
    if (w==0) {
      posPoint->winMask[ss]=0;
      posPoint->winOrderSet[ss]=0;
      posPoint->leastWin[ss]=0;
      for (hh=0; hh<=3; hh++)
        temp[hh][ss]=0;
    }
    else {
      w=w & (-w);       /* Only lowest win */
      for (hh=0; hh<=3; hh++)
	temp[hh][ss]=posPoint->rankInSuit[hh][ss] & (-w);

      aggr[ss]=0;
      for (hh=0; hh<=3; hh++)
	aggr[ss]|=temp[hh][ss];
      posPoint->winMask[ss]=thrp->abs[aggr[ss]].winMask[ss];
      posPoint->winOrderSet[ss]=thrp->abs[aggr[ss]].aggrRanks[ss];
      wm=posPoint->winMask[ss];
      wm=wm & (-wm);
      posPoint->leastWin[ss]=InvWinMask(wm);
    }
  }

  /* 07-04-22 */
  if (scoreFlag) {
    if (thrp->nodeTypeStore[0]==MAXNODE) {
      posPoint->ubound=tricks+1;
      posPoint->lbound=target-posPoint->tricksMAX;
    }
    else {
      posPoint->ubound=tricks+1-target+posPoint->tricksMAX;
      posPoint->lbound=0;
    }
  }
  else {
    if (thrp->nodeTypeStore[0]==MAXNODE) {
      posPoint->ubound=target-posPoint->tricksMAX-1;
      posPoint->lbound=0;
    }
    else {
      posPoint->ubound=tricks+1;
      posPoint->lbound=tricks+1-target+posPoint->tricksMAX+1;
    }
  }

  /*long long suitLengths=0;
  for (int s=0; s<=2; s++)
    for (hh=0; hh<=3; hh++) {
      suitLengths<<=4;
      suitLengths|=posPoint->length[hh][s];
    }*/

  np=SearchLenAndInsert(thrp->rootnp[tricks][firstHand],
	 suitLengths, TRUE, &res, thrp);

  cardsP=BuildPath(posPoint, np, &res, thrp);
  if (res) {
    cardsP->ubound=posPoint->ubound;
    cardsP->lbound=posPoint->lbound;
    if (((thrp->nodeTypeStore[firstHand]==MAXNODE)&&(scoreFlag))||
	((thrp->nodeTypeStore[firstHand]==MINNODE)&&(!scoreFlag))) {
      cardsP->bestMoveSuit=thrp->bestMove[depth].suit;
      cardsP->bestMoveRank=thrp->bestMove[depth].rank;
    }
    else {
      cardsP->bestMoveSuit=0;
      cardsP->bestMoveRank=0;
    }
    posPoint->bestMoveSuit=thrp->bestMove[depth].suit;
    posPoint->bestMoveRank=thrp->bestMove[depth].rank;
    for (int k=0; k<=3; k++)
      cardsP->leastWin[k]=posPoint->leastWin[k];
  }

  #ifdef STAT
    c9[depth]++;
  #endif

}


int CheckDeal(struct moveType * cardp, int thrId) {
  int h, s, k, found;
  unsigned short int temp[4][4];

  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      temp[h][s]=localVar[thrId].game.suit[h][s];

  /* Check that all ranks appear only once within the same suit. */
  for (s=0; s<=3; s++)
    for (k=2; k<=14; k++) {
      found=FALSE;
      for (h=0; h<=3; h++) {
        if ((temp[h][s] & bitMapRank[k])!=0) {
          if (found) {
            cardp->suit=s;
            cardp->rank=k;
            return 1;
          }
          else
            found=TRUE;
        }
      }
    }

  return 0;
}


int NextMove(struct pos *posPoint, int depth, struct movePlyType *mply, 
  struct localVarType * thrp) {
  /* Returns TRUE if at least one move remains to be
  searched, otherwise FALSE is returned. */

  unsigned short int lw;
  int suit;
  struct moveType currMove=mply->move[mply->current];

  if (thrp->lowestWin[depth][currMove.suit]==0) {
    /* A small card has not yet been identified for this suit. */
    lw=posPoint->winRanks[depth][currMove.suit];
    if (lw!=0)
      lw=lw & (-lw);  /* LSB */
    else
      lw=bitMapRank[15];
    if (bitMapRank[currMove.rank]<lw) {
       /* The current move has a small card. */
      thrp->lowestWin[depth][currMove.suit]=lw;
      while (mply->current <= (mply->last-1)) {
	mply->current++;
	if (bitMapRank[mply->move[mply->current].rank] >=
	  thrp->lowestWin[depth][mply->move[mply->current].suit])
	  return TRUE;
      }
      return FALSE;
    }
    else {
      while (mply->current <= (mply->last-1)) {
	mply->current++;
	suit=mply->move[mply->current].suit;
	if ((currMove.suit==suit) || (bitMapRank[mply->move[mply->current].rank] >=
		thrp->lowestWin[depth][suit]))
	  return TRUE;
      }
      return FALSE;
    }
  }
  else {
    while (mply->current<=(mply->last-1)) {
      mply->current++;
      if (bitMapRank[mply->move[mply->current].rank] >=
	    thrp->lowestWin[depth][mply->move[mply->current].suit])
	return TRUE;
    }
    return FALSE;
  }
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
  int i, count, ec[4], trickCount=0, s, r;
  for (i=0; i<=3; i++) {
    count=counttable[ranks[3][i]];
    if (count>5)
      ec[i]=TRUE;
    else
      ec[i]=FALSE;
    trickCount=trickCount+count;
  }
  fprintf(fp, "\n");
  for (s=0; s<=3; s++) {
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
  for (s=0; s<=3; s++) {
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
  for (s=0; s<=3; s++) {
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



void Wipe(struct localVarType * thrp) {
  int k;

  for (k=1; k<=thrp->wcount; k++) {
    if (thrp->pw[k])
      free(thrp->pw[k]);
    thrp->pw[k]=NULL;
  }
  for (k=1; k<=thrp->ncount; k++) {
    if (thrp->pn[k])
      free(thrp->pn[k]);
    thrp->pn[k]=NULL;
  }
  for (k=1; k<=thrp->lcount; k++) {
    if (thrp->pl[k])
      free(thrp->pl[k]);
    thrp->pl[k]=NULL;
  }

  thrp->allocmem=thrp->summem;

  return;
}


void AddWinSet(struct localVarType * thrp) {
  if (thrp->clearTTflag) {
    thrp->windex++;
    thrp->winSetSize=thrp->windex;
    thrp->winCards=&(thrp->temp_win[thrp->windex]);
  }
  else if (thrp->winSetSize>=thrp->winSetSizeLimit) {
    /* The memory chunk for the winCards structure will be exceeded. */
    if ((thrp->allocmem + thrp->wmem)>thrp->maxmem) {
      /* Already allocated memory plus needed allocation overshot maxmem */
      thrp->windex++;
      thrp->winSetSize=thrp->windex;
      thrp->clearTTflag=TRUE;
      thrp->winCards=&(thrp->temp_win[thrp->windex]);
    }
    else {
      thrp->wcount++; thrp->winSetSizeLimit=WSIZE;
      thrp->pw[thrp->wcount] =
		  (struct winCardType *)calloc(thrp->winSetSizeLimit+1, 
         sizeof(struct winCardType));
      if (thrp->pw[thrp->wcount]==NULL) {
        thrp->clearTTflag=TRUE;
        thrp->windex++;
	thrp->winSetSize=thrp->windex;
	thrp->winCards=&(thrp->temp_win[thrp->windex]);
      }
      else {
	thrp->allocmem += (thrp->winSetSizeLimit+1)*sizeof(struct winCardType);
	thrp->winSetSize=0;
	thrp->winCards=thrp->pw[thrp->wcount];
      }
    }
  }
  else
    thrp->winSetSize++;
  return;
}

void AddNodeSet(struct localVarType * thrp) {
  if (thrp->nodeSetSize>=thrp->nodeSetSizeLimit) {
    /* The memory chunk for the nodeCards structure will be exceeded. */
    if ((thrp->allocmem + thrp->nmem)>thrp->maxmem) {
      /* Already allocated memory plus needed allocation overshot maxmem */
      thrp->clearTTflag=TRUE;
    }
    else {
      thrp->ncount++; thrp->nodeSetSizeLimit=NSIZE;
      thrp->pn[thrp->ncount] = 
	(struct nodeCardsType *)calloc(thrp->nodeSetSizeLimit+1, sizeof(struct nodeCardsType));
      if (thrp->pn[thrp->ncount]==NULL) {
        thrp->clearTTflag=TRUE;
      }
      else {
	thrp->allocmem+=(thrp->nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
	thrp->nodeSetSize=0;
	thrp->nodeCards=thrp->pn[thrp->ncount];
      }
    }
  }
  else
    thrp->nodeSetSize++;
  return;
}

void AddLenSet(struct localVarType * thrp) {
  if (thrp->lenSetSize>=thrp->lenSetSizeLimit) {
      /* The memory chunk for the posSearchType structure will be exceeded. */
    if ((thrp->allocmem + thrp->lmem)>thrp->maxmem) {
       /* Already allocated memory plus needed allocation overshot maxmem */
      thrp->clearTTflag=TRUE;
    }
    else {
      thrp->lcount++; thrp->lenSetSizeLimit=LSIZE;
      thrp->pl[thrp->lcount] = 
	(struct posSearchType *)calloc(thrp->lenSetSizeLimit+1, sizeof(struct posSearchType));
      if (thrp->pl[thrp->lcount]==NULL) {
        thrp->clearTTflag=TRUE;
      }
      else {
	thrp->allocmem += (thrp->lenSetSizeLimit+1)*sizeof(struct posSearchType);
	thrp->lenSetSize=0;
	thrp->posSearch=thrp->pl[thrp->lcount];
      }
    }
  }
  else
    thrp->lenSetSize++;
  return;
}


#if defined(_WIN32) && !defined(_OPENMP) && !defined(DDS_THREADS_SINGLE)
HANDLE solveAllEvents[MAXNOOFTHREADS];
struct paramType param;
LONG volatile threadIndex;
LONG volatile current;

long chunk;

DWORD CALLBACK SolveChunk (void *) {
  struct futureTricks fut[MAXNOOFBOARDS];
  int thid;
  long j;

  thid=InterlockedIncrement(&threadIndex);

  while ((j=(InterlockedIncrement(&current)-1))<param.noOfBoards) {

    int res=SolveBoard(param.bop->deals[j], param.bop->target[j],
    param.bop->solutions[j], param.bop->mode[j], &fut[j], thid);
    if (res==1) {
      param.solvedp->solvedBoard[j]=fut[j];
      /*param.error=0;*/
    }
    else {
      param.error=res;
    }
  }

  if (SetEvent(solveAllEvents[thid])==0) {
    /*int errCode=GetLastError();*/
    return 0;
  }

  return 1;

}


DWORD CALLBACK SolveChunkDDtable (void *) {
  struct futureTricks fut[MAXNOOFBOARDS];
  int thid;
  long j;

  thid=InterlockedIncrement(&threadIndex);

  while ((j=InterlockedExchangeAdd(&current, chunk))<param.noOfBoards) {

    for (int k=0; k<chunk && j+k<param.noOfBoards; k++) {
      int res=SolveBoard(param.bop->deals[j+k], param.bop->target[j+k],
	  param.bop->solutions[j+k], param.bop->mode[j+k], 
	  &fut[j+k], thid);
      if (res==1) {
	param.solvedp->solvedBoard[j+k]=fut[j+k];
	/*param.error=0;*/
      }
      else {
	param.error=res;
      }
    }
  }

  if (SetEvent(solveAllEvents[thid])==0) {
    /*int errCode=GetLastError();*/
    return 0;
  }

  return 1;

}


int SolveAllBoardsN(struct boards *bop, struct solvedBoards *solvedp, int chunkSize) {
  int k/*, errCode*/;
  DWORD res;
  DWORD solveAllWaitResult;

  current = 0;
  threadIndex = -1;
  chunk = chunkSize;
  param.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

    for (k = 0; k<noOfCores; k++) {
      solveAllEvents[k] = CreateEvent(NULL, FALSE, FALSE, 0);
      if (solveAllEvents[k] == 0) {
	/*errCode=GetLastError();*/
	return RETURN_THREAD_CREATE;
      }
    }

    param.bop = bop; param.solvedp = solvedp; param.noOfBoards = bop->noOfBoards;

    for (k = 0; k<MAXNOOFBOARDS; k++)
      solvedp->solvedBoard[k].cards = 0;

    if (chunkSize != 1) {
      for (k = 0; k<noOfCores; k++) {
        res = QueueUserWorkItem(SolveChunkDDtable, NULL, WT_EXECUTELONGFUNCTION);
        if (res != 1) {
	  /*errCode=GetLastError();*/
	  return res;
        }
      }
    }
    else {
      for (k=0; k<noOfCores; k++) {
        res=QueueUserWorkItem(SolveChunk, NULL, WT_EXECUTELONGFUNCTION);
        if (res!=1) {
          /*errCode=GetLastError();*/
          return res;
        }
      }
    }

    solveAllWaitResult = WaitForMultipleObjects(noOfCores, solveAllEvents, TRUE, INFINITE);
    if (solveAllWaitResult != WAIT_OBJECT_0) {
      /*errCode=GetLastError();*/
      return RETURN_THREAD_WAIT;
    }

    for (k = 0; k<noOfCores; k++) {
      CloseHandle(solveAllEvents[k]);
    }

    /* Calculate number of solved boards. */

    solvedp->noOfBoards = 0;
    for (k = 0; k<MAXNOOFBOARDS; k++) {
      if (solvedp->solvedBoard[k].cards != 0)
	solvedp->noOfBoards++;
    }

    if (param.error == 0)
      return 1;
    else
      return param.error;
}

#else

int SolveAllBoardsN(struct boards *bop, struct solvedBoards *solvedp, int chunkSize) {
  int k, i, res, chunk, fail;
  struct futureTricks fut[MAXNOOFBOARDS];

  chunk=chunkSize; fail=1;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  for (i=0; i<MAXNOOFBOARDS; i++)
      solvedp->solvedBoard[i].cards=0;

#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
  if (omp_get_dynamic())
    omp_set_dynamic(0);
  omp_set_num_threads(noOfCores);	/* Added after suggestion by Dirk Willecke. */
#elif defined (_OPENMP)
  omp_set_num_threads(1);
#endif

  #pragma omp parallel shared(bop, solvedp, chunk, fail) private(k)
  {

    #pragma omp for schedule(dynamic, chunk)

    for (k=0; k<bop->noOfBoards; k++) {
      res=SolveBoard(bop->deals[k], bop->target[k], bop->solutions[k],
        bop->mode[k], &fut[k],
 
#if defined (_OPENMP) && !defined(DDS_THREADS_SINGLE)
		omp_get_thread_num()
#else
		0
#endif
		);
      if (res==1) {
        solvedp->solvedBoard[k]=fut[k];
      }
      else
        fail=res;
    }
  }

  if (fail!=1)
    return fail;

  solvedp->noOfBoards=0;
  for (i=0; i<MAXNOOFBOARDS; i++) {
    if (solvedp->solvedBoard[i].cards!=0)
      solvedp->noOfBoards++;
  }

  return 1;
}

#endif


int STDCALL CalcDDtable(struct ddTableDeal tableDeal, struct ddTableResults * tablep) {

  int h, s, k, ind, tr, first, res;
  struct deal dl;
  struct boards bo;
  struct solvedBoards solved;

  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      dl.remainCards[h][s]=tableDeal.cards[h][s];

  for (k=0; k<=2; k++) {
    dl.currentTrickRank[k]=0;
    dl.currentTrickSuit[k]=0;
  }

  ind=0; bo.noOfBoards=20;

  for (tr=4; tr>=0; tr--)
    for (first=0; first<=3; first++) {
      dl.first=first;
      dl.trump=tr;
      bo.deals[ind]=dl;
      bo.target[ind]=-1;
      bo.solutions[ind]=1;
      bo.mode[ind]=1;
      ind++;
    }

  res=SolveAllBoardsN(&bo, &solved, 4);
  if (res==1) {
    for (ind=0; ind<20; ind++) {
      tablep->resTable[bo.deals[ind].trump][rho[bo.deals[ind].first]]=
	    13-solved.solvedBoard[ind].score[0];
    }
    return 1;
  }

  return res;
}


int STDCALL CalcAllTables(struct ddTableDeals *dealsp, int mode, int trumpFilter[5], 
  struct ddTablesRes *resp, struct allParResults *presp) {

  /* mode = 0:	par calculation, vulnerability None
     mode = 1:	par calculation, vulnerability All
     mode = 2:	par calculation, vulnerability NS
     mode = 3:	par calculation, vulnerability EW  
	 mode = -1:  no par calculation  */

  int h, s, k, m, ind, tr, first, res, rs, lastIndex=0, 
	  lastBoardIndex[MAXNOOFBOARDS>>2], okey=FALSE, count=0;
  struct boards bo;
  struct solvedBoards solved;

  /*int Par(struct ddTableResults * tablep, struct parResults *presp, int vulnerable);*/

  for (k=0; k<5; k++) { 
    if (!trumpFilter[k]) {
      okey=TRUE; 
      count++;
    }
  }

  if (!okey)
    return RETURN_NO_SUIT;

  switch (count) {
    case 1:  if (dealsp->noOfTables > 50) return RETURN_TOO_MANY_TABLES;break;
    case 2:  if (dealsp->noOfTables > 25) return RETURN_TOO_MANY_TABLES;break;
    case 3:  if (dealsp->noOfTables > 16) return RETURN_TOO_MANY_TABLES;break;
    case 4:  if (dealsp->noOfTables > 12) return RETURN_TOO_MANY_TABLES;break;
    case 5:  if (dealsp->noOfTables > 10) return RETURN_TOO_MANY_TABLES;break;
  }

  ind=0;
  resp->noOfBoards=0;

  for (m=0; m<dealsp->noOfTables; m++) {
    for (tr=4; tr>=0; tr--) {
      if (!trumpFilter[tr]) {
        for (first=0; first<=3; first++) {
	  for (h=0; h<=3; h++)
            for (s=0; s<=3; s++)
	      bo.deals[ind].remainCards[h][s]=dealsp->deals[m].cards[h][s];
	      bo.deals[ind].first=first;
	      bo.deals[ind].trump=tr;
	  for (k=0; k<=2; k++) {
            bo.deals[ind].currentTrickRank[k]=0;
            bo.deals[ind].currentTrickSuit[k]=0;
          }

          bo.target[ind]=-1;
          bo.solutions[ind]=1;
          bo.mode[ind]=1;
	  lastIndex=ind;
	  lastBoardIndex[m]=ind;
          ind++;
	}
      }
    }
  }

  bo.noOfBoards=lastIndex+1;

  res=SolveAllBoardsN(&bo, &solved, 4);
  if (res==1) {
    resp->noOfBoards+=solved.noOfBoards;
    for (ind=0; ind<=lastIndex; ind++) {
      for (k=0; k<=lastIndex; k++) {
	if (ind<=lastBoardIndex[k]) {
	  resp->results[k].resTable[bo.deals[ind].trump][rho[bo.deals[ind].first]]=
	     13-solved.solvedBoard[ind].score[0];
	  break;
	}
      }
    }

    if ((mode > -1) && (mode < 4)) {
      /* Calculate par */
      for (k=0; k<dealsp->noOfTables; k++) {
	rs=Par(&(resp->results[k]), &(presp->presults[k]), mode);
	/* vulnerable 0: None  1: Both  2: NS  3: EW */
	if (rs!=1)
	  return rs;
      }
    }
    return 1;
  }
  return res;
}
 

int STDCALL CalcAllTablesPBN(struct ddTableDealsPBN *dealsp, int mode, int trumpFilter[5], 
    struct ddTablesRes *resp, struct allParResults *presp) {
  int res, k;
  struct ddTableDeals dls;

  int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);

  for (k=0; k<dealsp->noOfTables; k++)
    if (ConvertFromPBN(dealsp->deals[k].cards, dls.deals[k].cards)!=1)
      return RETURN_PBN_FAULT;

  dls.noOfTables=dealsp->noOfTables;

  res=CalcAllTables(&dls, mode, trumpFilter, resp, presp);

  return res;
}


int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]) {
  int bp=0, first, card, hand, handRelFirst, suitInHand, h, s;
  int IsCard(char cardChar);

  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      remainCards[h][s]=0;

  while (((dealBuff[bp]!='W')&&(dealBuff[bp]!='N')&&
	(dealBuff[bp]!='E')&&(dealBuff[bp]!='S')&&
        (dealBuff[bp]!='w')&&(dealBuff[bp]!='n')&&
	(dealBuff[bp]!='e')&&(dealBuff[bp]!='s'))&&(bp<3))
    bp++;

  if (bp>=3)
    return 0;

  if ((dealBuff[bp]=='N')||(dealBuff[bp]=='n'))
    first=0;
  else if ((dealBuff[bp]=='E')||(dealBuff[bp]=='e'))
    first=1;
  else if ((dealBuff[bp]=='S')||(dealBuff[bp]=='s'))
    first=2;
  else
    first=3;

  bp++;
  bp++;

  handRelFirst=0;  suitInHand=0;

  while ((bp<80)&&(dealBuff[bp]!='\0')) {
    card=IsCard(dealBuff[bp]);
    if (card) {
      switch (first) {
	case 0:
	  hand=handRelFirst;
	  break;
	case 1:
	  if (handRelFirst==0)
	    hand=1;
	  else if (handRelFirst==3)
	    hand=0;
	  else
	    hand=handRelFirst+1;
	    break;
	case 2:
	  if (handRelFirst==0)
	    hand=2;
	  else if (handRelFirst==1)
	    hand=3;
	  else
	    hand=handRelFirst-2;
	  break;
	default:
          if (handRelFirst==0)
	    hand=3;
	  else
	    hand=handRelFirst-1;
      }

      remainCards[hand][suitInHand]|=(bitMapRank[card]<<2);

    }
    else if (dealBuff[bp]=='.')
      suitInHand++;
    else if (dealBuff[bp]==' ') {
      handRelFirst++;
      suitInHand=0;
    }
    bp++;
  }
  return 1;
}


int IsCard(char cardChar)   {
  switch (cardChar)  {
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case 'T':
      return 10;
    case 'J':
      return 11;
    case 'Q':
      return 12;
    case 'K':
      return 13;
    case 'A':
      return 14;
    case 't':
      return 10;
    case 'j':
      return 11;
    case 'q':
      return 12;
    case 'k':
      return 13;
    case 'a':
      return 14;
    default:
      return 0;
   }
 }

 
int STDCALL SolveBoardPBN(struct dealPBN dlpbn, int target,
    int solutions, int mode, struct futureTricks *futp, int threadIndex) {

  int res, k;
  struct deal dl;
  int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);

  if (ConvertFromPBN(dlpbn.remainCards, dl.remainCards)!=RETURN_NO_FAULT)
    return RETURN_PBN_FAULT;

  for (k=0; k<=2; k++) {
    dl.currentTrickRank[k]=dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k]=dlpbn.currentTrickSuit[k];
  }
  dl.first=dlpbn.first;
  dl.trump=dlpbn.trump;

  res=SolveBoard(dl, target, solutions, mode, futp, threadIndex);

  return res;
}

int STDCALL CalcDDtablePBN(struct ddTableDealPBN tableDealPBN, struct ddTableResults * tablep) {
  struct ddTableDeal tableDeal;
  int res;

  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards)!=1)
    return RETURN_PBN_FAULT;

  res=CalcDDtable(tableDeal, tablep);

  return res;
}


int STDCALL SolveAllBoards(struct boardsPBN *bop, struct solvedBoards *solvedp) {
  struct boards bo;
  int k, i, res;

  bo.noOfBoards=bop->noOfBoards;
  for (k=0; k<bop->noOfBoards; k++) {
    bo.mode[k]=bop->mode[k];
    bo.solutions[k]=bop->solutions[k];
    bo.target[k]=bop->target[k];
    bo.deals[k].first=bop->deals[k].first;
    bo.deals[k].trump=bop->deals[k].trump;
    for (i=0; i<=2; i++) {
      bo.deals[k].currentTrickSuit[i]=bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i]=bop->deals[k].currentTrickRank[i];
    }
    if (ConvertFromPBN(bop->deals[k].remainCards, bo.deals[k].remainCards)!=1)
      return RETURN_PBN_FAULT;
  }

  res=SolveAllBoardsN(&bo, solvedp, 1);

  return res;
}

int STDCALL SolveAllChunksPBN(struct boardsPBN *bop, struct solvedBoards *solvedp, int chunkSize) {
  struct boards bo;
  int k, i, res;

  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  bo.noOfBoards = bop->noOfBoards;
  for (k = 0; k<bop->noOfBoards; k++) {
    bo.mode[k] = bop->mode[k];
    bo.solutions[k] = bop->solutions[k];
    bo.target[k] = bop->target[k];
    bo.deals[k].first = bop->deals[k].first;
    bo.deals[k].trump = bop->deals[k].trump;
    for (i = 0; i <= 2; i++) {
      bo.deals[k].currentTrickSuit[i] = bop->deals[k].currentTrickSuit[i];
      bo.deals[k].currentTrickRank[i] = bop->deals[k].currentTrickRank[i];
    }
    if (ConvertFromPBN(bop->deals[k].remainCards, bo.deals[k].remainCards) != 1)
      return RETURN_PBN_FAULT;
  }

  res = SolveAllBoardsN(&bo, solvedp, chunkSize);
  return res;
}


int STDCALL SolveAllChunks(struct boardsPBN *bop, struct solvedBoards *solvedp, int chunkSize) {

  int res = SolveAllChunksPBN(bop, solvedp, chunkSize);

  return res;
} 


int STDCALL SolveAllChunksBin(struct boards *bop, struct solvedBoards *solvedp, int chunkSize) {
  int res;

  if (chunkSize < 1)
    return RETURN_CHUNK_SIZE;

  res = SolveAllBoardsN(bop, solvedp, chunkSize);
  return res;
}










