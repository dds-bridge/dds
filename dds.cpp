
/* DDS 2.3.0   A bridge double dummy solver.				      */
/* Copyright (C) 2006-2012 by Bo Haglund                                      */
/* Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli.        */
/* The code for calculation of par score / contracts is based upon the	      */
/* perl code written by Matthew Kidd for ACBLmerge. He has kindly given me    */
/* permission to include a C++ adaptation in DDS. 	      		      */ 
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


/*#include "stdafx.h"*/ 	/* Needed by Visual C++ */

#include "dll.h"

struct localVarType localVar[MAXNOOFTHREADS];

int * counttable;
int * highestRank;
int lho[4];
int rho[4];
int partner[4];
unsigned short int bitMapRank[16];
unsigned char cardRank[15];
unsigned char cardSuit[5];
unsigned char cardHand[4];
int stat_contr[5]={0,0,0,0,0};
int max_low[3][8];  /* index 1: 0=NT, 1=Major, 2=Minor  index 2: contract level 1-7 */

struct ttStoreType * ttStore;
int lastTTstore;
int ttCollect;
int suppressTTlog;

int noOfThreads=MAXNOOFTHREADS;  /* The number of entries to the transposition tables. There is
				    one entry per thread. */
int noOfCores;			/* The number of processor cores, however cannot be higher than noOfThreads. */

#ifdef _MANAGED
#pragma managed(push, off)
#endif


#if defined(_WIN32)
extern "C" BOOL APIENTRY DllMain(HMODULE hModule,
				DWORD ul_reason_for_call,
				LPVOID lpReserved) {
  int k;

  if (ul_reason_for_call==DLL_PROCESS_ATTACH) {
    InitStart(0, 0);
  }
  else if (ul_reason_for_call==DLL_PROCESS_DETACH) {
    for (k=0; k<noOfThreads; k++) {
      Wipe(k);
      if (localVar[k].pw[0])
        free(localVar[k].pw[0]);
      localVar[k].pw[0]=NULL;
      if (localVar[k].pn[0])
        free(localVar[k].pn[0]);
      localVar[k].pn[0]=NULL;
      if (localVar[k].pl[0])
        free(localVar[k].pl[0]);
      localVar[k].pl[0]=NULL;
      if (localVar[k].pw)
	free(localVar[k].pw);
      localVar[k].pw=NULL;
      if (localVar[k].pn)
	free(localVar[k].pn);
      localVar[k].pn=NULL;
      if (localVar[k].pl)
	free(localVar[k].pl);
      localVar[k].pl=NULL;
      if (ttStore)
        free(ttStore);
      ttStore=NULL;
      if (localVar[k].rel)
        free(localVar[k].rel);
      localVar[k].rel=NULL;
      if (localVar[k].adaptWins)
	free(localVar[k].adaptWins);
      localVar[k].adaptWins=NULL;
    }
    if (highestRank)
      free(highestRank);
    highestRank=NULL;
    if (counttable)
      free(counttable);
    counttable=NULL;
	/*_CrtDumpMemoryLeaks();*/	/* MEMORY LEAK? */
  }
  return 1;
}
#endif

#ifdef _MANAGED
#pragma managed(pop)
#endif

  int STDCALL SolveBoard(struct deal dl, int target,
    int solutions, int mode, struct futureTricks *futp, int thrId) {

  int k, n, cardCount, found, totalTricks, tricks, last, checkRes;
  int g, upperbound, lowerbound, first, i, j, h, forb, ind, flag, noMoves;
  int mcurr;
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


  /*InitStart(0,0);*/   /* Include InitStart() if inside SolveBoard,
			   but preferable InitStart should be called outside
					 SolveBoard like in DllMain for Windows. */

  if ((thrId<0)||(thrId>=noOfThreads)) {	/* Fault corrected after suggestion by Dirk Willecke. */
    DumpInput(-15, dl, target, solutions, mode);
	return -15;
  }

  for (k=0; k<=13; k++) {
    localVar[thrId].forbiddenMoves[k].rank=0;
    localVar[thrId].forbiddenMoves[k].suit=0;
  }

  if (target<-1) {
    DumpInput(-5, dl, target, solutions, mode);
    return -5;
  }
  if (target>13) {
    DumpInput(-7, dl, target, solutions, mode);
    return -7;
  }
  if (solutions<1) {
    DumpInput(-8, dl, target, solutions, mode);
    return -8;
  }
  if (solutions>3) {
    DumpInput(-9, dl, target, solutions, mode);
    return -9;
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
	DumpInput(-13, dl, target, solutions, mode);
	return -13;
      }
    }
  }

  if (target==-1)
    localVar[thrId].tricksTarget=99;
  else
    localVar[thrId].tricksTarget=target;

  localVar[thrId].newDeal=FALSE; localVar[thrId].newTrump=FALSE;
  localVar[thrId].diffDeal=0; localVar[thrId].aggDeal=0;
  cardCount=0;
  for (i=0; i<=3; i++) {
    for (j=0; j<=3; j++) {
      cardCount+=counttable[dl.remainCards[i][j]>>2];
      localVar[thrId].diffDeal+=((dl.remainCards[i][j]>>2)^
	      (localVar[thrId].game.suit[i][j]));
      localVar[thrId].aggDeal+=(dl.remainCards[i][j]>>2);
      if (localVar[thrId].game.suit[i][j]!=dl.remainCards[i][j]>>2) {
        localVar[thrId].game.suit[i][j]=dl.remainCards[i][j]>>2;
	localVar[thrId].newDeal=TRUE;
      }
    }
  }

  if (localVar[thrId].newDeal) {
    if (localVar[thrId].diffDeal==0)
      localVar[thrId].similarDeal=TRUE;
    else if ((localVar[thrId].aggDeal/localVar[thrId].diffDeal)
       > SIMILARDEALLIMIT)
      localVar[thrId].similarDeal=TRUE;
    else
      localVar[thrId].similarDeal=FALSE;
  }
  else
    localVar[thrId].similarDeal=FALSE;

  if (dl.trump!=localVar[thrId].trump)
    localVar[thrId].newTrump=TRUE;

  for (i=0; i<=3; i++)
    for (j=0; j<=3; j++)
      noOfCardsPerHand[i]+=counttable[localVar[thrId].game.suit[i][j]];

  for (i=1; i<=3; i++) {
    if (noOfCardsPerHand[i]!=noOfCardsPerHand[0]) {
      DumpInput(-14, dl, target, solutions, mode);
      return -14;
    }
  }

  if (dl.currentTrickRank[2]) {
    if ((dl.currentTrickRank[2]<2)||(dl.currentTrickRank[2]>14)
      ||(dl.currentTrickSuit[2]<0)||(dl.currentTrickSuit[2]>3)) {
      DumpInput(-12, dl, target, solutions, mode);
      return -12;
    }
    localVar[thrId].handToPlay=handId(dl.first, 3);
    handRelFirst=3;
    noStartMoves=3;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[localVar[thrId].handToPlay][k]!=0) {
          latestTrickSuit[localVar[thrId].handToPlay]=k;
          latestTrickRank[localVar[thrId].handToPlay]=
            InvBitMapRank(localVar[thrId].game.suit[localVar[thrId].handToPlay][k]);
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
      DumpInput(-12, dl, target, solutions, mode);
      return -12;
    }
    localVar[thrId].handToPlay=handId(dl.first, 2);
    handRelFirst=2;
    noStartMoves=2;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[localVar[thrId].handToPlay][k]!=0) {
          latestTrickSuit[localVar[thrId].handToPlay]=k;
          latestTrickRank[localVar[thrId].handToPlay]=
            InvBitMapRank(localVar[thrId].game.suit[localVar[thrId].handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
	if (localVar[thrId].game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(localVar[thrId].game.suit[handId(dl.first, 3)][k]);
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
      DumpInput(-12, dl, target, solutions, mode);
      return -12;
    }
    localVar[thrId].handToPlay=handId(dl.first,1);
    handRelFirst=1;
    noStartMoves=1;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[localVar[thrId].handToPlay][k]!=0) {
          latestTrickSuit[localVar[thrId].handToPlay]=k;
          latestTrickRank[localVar[thrId].handToPlay]=
            InvBitMapRank(localVar[thrId].game.suit[localVar[thrId].handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
	if (localVar[thrId].game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(localVar[thrId].game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[handId(dl.first, 2)][k]!=0) {
          latestTrickSuit[handId(dl.first, 2)]=k;
          latestTrickRank[handId(dl.first, 2)]=
            InvBitMapRank(localVar[thrId].game.suit[handId(dl.first, 2)][k]);
          break;
        }
      }
      latestTrickSuit[dl.first]=dl.currentTrickSuit[0];
      latestTrickRank[dl.first]=dl.currentTrickRank[0];
    }
  }
  else {
    localVar[thrId].handToPlay=dl.first;
    handRelFirst=0;
    noStartMoves=0;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[localVar[thrId].handToPlay][k]!=0) {
          latestTrickSuit[localVar[thrId].handToPlay]=k;
          latestTrickRank[localVar[thrId].handToPlay]=
            InvBitMapRank(localVar[thrId].game.suit[localVar[thrId].handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(localVar[thrId].game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[handId(dl.first, 2)][k]!=0) {
          latestTrickSuit[handId(dl.first, 2)]=k;
          latestTrickRank[handId(dl.first, 2)]=
            InvBitMapRank(localVar[thrId].game.suit[handId(dl.first, 2)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (localVar[thrId].game.suit[handId(dl.first, 1)][k]!=0) {
          latestTrickSuit[handId(dl.first, 1)]=k;
          latestTrickRank[handId(dl.first, 1)]=
            InvBitMapRank(localVar[thrId].game.suit[handId(dl.first, 1)][k]);
          break;
        }
      }
    }
  }

  localVar[thrId].trump=dl.trump;
  localVar[thrId].game.first=dl.first;
  first=dl.first;
  localVar[thrId].game.noOfCards=cardCount;
  if (dl.currentTrickRank[0]!=0) {
    localVar[thrId].game.leadHand=dl.first;
    localVar[thrId].game.leadSuit=dl.currentTrickSuit[0];
    localVar[thrId].game.leadRank=dl.currentTrickRank[0];
  }
  else {
    localVar[thrId].game.leadHand=0;
    localVar[thrId].game.leadSuit=0;
    localVar[thrId].game.leadRank=0;
  }

  for (k=0; k<=2; k++) {
    localVar[thrId].initialMoves[k].suit=255;
    localVar[thrId].initialMoves[k].rank=255;
  }

  for (k=0; k<noStartMoves; k++) {
    localVar[thrId].initialMoves[noStartMoves-1-k].suit=dl.currentTrickSuit[k];
    localVar[thrId].initialMoves[noStartMoves-1-k].rank=dl.currentTrickRank[k];
  }

  if (cardCount % 4)
    totalTricks=((cardCount-4)>>2)+2;
  else
    totalTricks=((cardCount-4)>>2)+1;
  checkRes=CheckDeal(&localVar[thrId].cd, thrId);
  if (localVar[thrId].game.noOfCards<=0) {
    DumpInput(-2, dl, target, solutions, mode);
    return -2;
  }
  if (localVar[thrId].game.noOfCards>52) {
    DumpInput(-10, dl, target, solutions, mode);
    return -10;
  }
  if (totalTricks<target) {
    DumpInput(-3, dl, target, solutions, mode);
    return -3;
  }
  if (checkRes) {
    DumpInput(-4, dl, target, solutions, mode);
    return -4;
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
    #ifdef BENCH
    futp->totalNodes=0;
    #endif
    futp->cards=1;
    futp->suit[0]=latestTrickSuit[localVar[thrId].handToPlay];
    futp->rank[0]=latestTrickRank[localVar[thrId].handToPlay];
    futp->equals[0]=0;
    if ((target==0)&&(solutions<3))
      futp->score[0]=0;
    else if ((localVar[thrId].handToPlay==maxHand)||
	(partner[localVar[thrId].handToPlay]==maxHand))
      futp->score[0]=1;
    else
      futp->score[0]=0;

	/*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
    return 1;
  }

  if ((mode!=2)&&
    (((localVar[thrId].newDeal)&&(!localVar[thrId].similarDeal))
      || localVar[thrId].newTrump  ||
	  (localVar[thrId].winSetSize > SIMILARMAXWINNODES))) {

    Wipe(thrId);
	localVar[thrId].winSetSizeLimit=WINIT;
    localVar[thrId].nodeSetSizeLimit=NINIT;
    localVar[thrId].lenSetSizeLimit=LINIT;
    localVar[thrId].allocmem=(WINIT+1)*sizeof(struct winCardType);
    localVar[thrId].allocmem+=(NINIT+1)*sizeof(struct nodeCardsType);
    localVar[thrId].allocmem+=(LINIT+1)*sizeof(struct posSearchType);
    localVar[thrId].winCards=localVar[thrId].pw[0];
    localVar[thrId].nodeCards=localVar[thrId].pn[0];
    localVar[thrId].posSearch=localVar[thrId].pl[0];
    localVar[thrId].wcount=0; localVar[thrId].ncount=0; localVar[thrId].lcount=0;
    InitGame(0, FALSE, first, handRelFirst, thrId);
  }
  else {
    InitGame(0, TRUE, first, handRelFirst, thrId);
	/*localVar[thrId].fp2=fopen("dyn.txt", "a");
	fprintf(localVar[thrId].fp2, "wcount=%d, ncount=%d, lcount=%d\n",
	  wcount, ncount, lcount);
    fprintf(localVar[thrId].fp2, "winSetSize=%d, nodeSetSize=%d, lenSetSize=%d\n",
	  winSetSize, nodeSetSize, lenSetSize);
    fclose(localVar[thrId].fp2);*/
  }

  localVar[thrId].nodes=0; localVar[thrId].trickNodes=0;
  localVar[thrId].iniDepth=cardCount-4;
  hiwinSetSize=0;
  hinodeSetSize=0;

  if (mode==0) {
    MoveGen(&localVar[thrId].lookAheadPos, localVar[thrId].iniDepth, localVar[thrId].trump,
		&localVar[thrId].movePly[localVar[thrId].iniDepth], thrId);
    if (localVar[thrId].movePly[localVar[thrId].iniDepth].last==0) {
	futp->nodes=0;
    #ifdef BENCH
        futp->totalNodes=0;
    #endif
	futp->cards=1;
	futp->suit[0]=localVar[thrId].movePly[localVar[thrId].iniDepth].move[0].suit;
	futp->rank[0]=localVar[thrId].movePly[localVar[thrId].iniDepth].move[0].rank;
	futp->equals[0]=
	  localVar[thrId].movePly[localVar[thrId].iniDepth].move[0].sequence<<2;
	futp->score[0]=-2;

	/*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
	return 1;
    }
  }
  if ((target==0)&&(solutions<3)) {
    MoveGen(&localVar[thrId].lookAheadPos, localVar[thrId].iniDepth, localVar[thrId].trump,
		&localVar[thrId].movePly[localVar[thrId].iniDepth], thrId);
    futp->nodes=0;
    #ifdef BENCH
    futp->totalNodes=0;
    #endif
    for (k=0; k<=localVar[thrId].movePly[localVar[thrId].iniDepth].last; k++) {
	futp->suit[k]=localVar[thrId].movePly[localVar[thrId].iniDepth].move[k].suit;
	futp->rank[k]=localVar[thrId].movePly[localVar[thrId].iniDepth].move[k].rank;
	futp->equals[k]=
	  localVar[thrId].movePly[localVar[thrId].iniDepth].move[k].sequence<<2;
	futp->score[k]=0;
    }
    if (solutions==1)
	futp->cards=1;
    else
	futp->cards=localVar[thrId].movePly[localVar[thrId].iniDepth].last+1;

	/*_CrtDumpMemoryLeaks(); */ /* MEMORY LEAK? */
    return 1;
  }

  if ((target!=-1)&&(solutions!=3)) {
    localVar[thrId].val=ABsearch(&localVar[thrId].lookAheadPos,
		localVar[thrId].tricksTarget, localVar[thrId].iniDepth, thrId);

    temp=localVar[thrId].movePly[localVar[thrId].iniDepth];
    last=localVar[thrId].movePly[localVar[thrId].iniDepth].last;
    noMoves=last+1;
    hiwinSetSize=localVar[thrId].winSetSize;
    hinodeSetSize=localVar[thrId].nodeSetSize;
    hilenSetSize=localVar[thrId].lenSetSize;
    if (localVar[thrId].nodeSetSize>MaxnodeSetSize)
      MaxnodeSetSize=localVar[thrId].nodeSetSize;
    if (localVar[thrId].winSetSize>MaxwinSetSize)
      MaxwinSetSize=localVar[thrId].winSetSize;
    if (localVar[thrId].lenSetSize>MaxlenSetSize)
      MaxlenSetSize=localVar[thrId].lenSetSize;
    if (localVar[thrId].val==1)
      localVar[thrId].payOff=localVar[thrId].tricksTarget;
    else
      localVar[thrId].payOff=0;
    futp->cards=1;
    ind=2;

    if (localVar[thrId].payOff<=0) {
      futp->suit[0]=localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[0].suit;
      futp->rank[0]=localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[0].rank;
	futp->equals[0]=(localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[0].sequence)<<2;
      if (localVar[thrId].tricksTarget>1)
        futp->score[0]=-1;
      else
	futp->score[0]=0;
    }
    else {
      futp->suit[0]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].suit;
      futp->rank[0]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].rank;
	futp->equals[0]=(localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].sequence)<<2;
      futp->score[0]=localVar[thrId].payOff;
    }
  }
  else {
    g=localVar[thrId].estTricks[localVar[thrId].handToPlay];
    upperbound=13;
    lowerbound=0;
    do {
      if (g==lowerbound)
        tricks=g+1;
      else
        tricks=g;
	  assert((localVar[thrId].lookAheadPos.handRelFirst>=0)&&
		(localVar[thrId].lookAheadPos.handRelFirst<=3));
      localVar[thrId].val=ABsearch(&localVar[thrId].lookAheadPos, tricks,
		  localVar[thrId].iniDepth, thrId);

      if (localVar[thrId].val==TRUE)
        mv=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4];
      hiwinSetSize=Max(hiwinSetSize, localVar[thrId].winSetSize);
      hinodeSetSize=Max(hinodeSetSize, localVar[thrId].nodeSetSize);
	hilenSetSize=Max(hilenSetSize, localVar[thrId].lenSetSize);
      if (localVar[thrId].nodeSetSize>MaxnodeSetSize)
        MaxnodeSetSize=localVar[thrId].nodeSetSize;
      if (localVar[thrId].winSetSize>MaxwinSetSize)
        MaxwinSetSize=localVar[thrId].winSetSize;
	if (localVar[thrId].lenSetSize>MaxlenSetSize)
        MaxlenSetSize=localVar[thrId].lenSetSize;
      if (localVar[thrId].val==FALSE) {
	upperbound=tricks-1;
	g=upperbound;
      }
      else {
        lowerbound=tricks;
        g=lowerbound;
      }
      InitSearch(&localVar[thrId].iniPosition, localVar[thrId].game.noOfCards-4,
        localVar[thrId].initialMoves, first, TRUE, thrId);
    }
    while (lowerbound<upperbound);
    localVar[thrId].payOff=g;
    temp=localVar[thrId].movePly[localVar[thrId].iniDepth];
    last=localVar[thrId].movePly[localVar[thrId].iniDepth].last;
    noMoves=last+1;
    ind=2;
    localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4]=mv;
    futp->cards=1;
    if (localVar[thrId].payOff<=0) {
      futp->score[0]=0;
      futp->suit[0]=localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[0].suit;
      futp->rank[0]=localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[0].rank;
      futp->equals[0]=(localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[0].sequence)<<2;
    }
    else {
      futp->score[0]=localVar[thrId].payOff;
      futp->suit[0]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].suit;
      futp->rank[0]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].rank;
      futp->equals[0]=(localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].sequence)<<2;
    }
    localVar[thrId].tricksTarget=localVar[thrId].payOff;
  }

  if ((solutions==2)&&(localVar[thrId].payOff>0)) {
    forb=1;
    ind=forb;
    while ((localVar[thrId].payOff==localVar[thrId].tricksTarget)&&(ind<(temp.last+1))) {
      localVar[thrId].forbiddenMoves[forb].suit=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].suit;
      localVar[thrId].forbiddenMoves[forb].rank=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].rank;
      forb++; ind++;
      /* All moves before bestMove in the move list shall be
      moved to the forbidden moves list, since none of them reached
      the target */
      mcurr=localVar[thrId].movePly[localVar[thrId].iniDepth].current;
      for (k=0; k<=localVar[thrId].movePly[localVar[thrId].iniDepth].last; k++)
        if ((localVar[thrId].bestMove[localVar[thrId].iniDepth].suit==
			localVar[thrId].movePly[localVar[thrId].iniDepth].move[k].suit)
          &&(localVar[thrId].bestMove[localVar[thrId].iniDepth].rank==
		    localVar[thrId].movePly[localVar[thrId].iniDepth].move[k].rank))
          break;
      for (i=0; i<k; i++) {  /* All moves until best move */
        flag=FALSE;
        for (j=0; j<forb; j++) {
          if ((localVar[thrId].movePly[localVar[thrId].iniDepth].move[i].suit==localVar[thrId].forbiddenMoves[j].suit)
            &&(localVar[thrId].movePly[localVar[thrId].iniDepth].move[i].rank==localVar[thrId].forbiddenMoves[j].rank)) {
            /* If the move is already in the forbidden list */
            flag=TRUE;
            break;
          }
        }
        if (!flag) {
          localVar[thrId].forbiddenMoves[forb]=localVar[thrId].movePly[localVar[thrId].iniDepth].move[i];
          forb++;
        }
      }
      InitSearch(&localVar[thrId].iniPosition, localVar[thrId].game.noOfCards-4,
          localVar[thrId].initialMoves, first, TRUE, thrId);
      localVar[thrId].val=ABsearch(&localVar[thrId].lookAheadPos, localVar[thrId].tricksTarget,
		  localVar[thrId].iniDepth, thrId);

      hiwinSetSize=localVar[thrId].winSetSize;
      hinodeSetSize=localVar[thrId].nodeSetSize;
      hilenSetSize=localVar[thrId].lenSetSize;
      if (localVar[thrId].nodeSetSize>MaxnodeSetSize)
        MaxnodeSetSize=localVar[thrId].nodeSetSize;
      if (localVar[thrId].winSetSize>MaxwinSetSize)
        MaxwinSetSize=localVar[thrId].winSetSize;
      if (localVar[thrId].lenSetSize>MaxlenSetSize)
        MaxlenSetSize=localVar[thrId].lenSetSize;
      if (localVar[thrId].val==TRUE) {
        localVar[thrId].payOff=localVar[thrId].tricksTarget;
        futp->cards=ind;
        futp->suit[ind-1]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].suit;
        futp->rank[ind-1]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].rank;
	futp->equals[ind-1]=(localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].sequence)<<2;
        futp->score[ind-1]=localVar[thrId].payOff;
      }
      else
        localVar[thrId].payOff=0;
    }
  }
  else if ((solutions==2)&&(localVar[thrId].payOff==0)&&
	((target==-1)||(localVar[thrId].tricksTarget==1))) {
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

  if ((solutions==3)&&(localVar[thrId].payOff>0)) {
    forb=1;
    ind=forb;
    for (i=0; i<last; i++) {
      localVar[thrId].forbiddenMoves[forb].suit=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].suit;
      localVar[thrId].forbiddenMoves[forb].rank=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].rank;
      forb++; ind++;

      g=localVar[thrId].payOff;
      upperbound=localVar[thrId].payOff;
      lowerbound=0;

      InitSearch(&localVar[thrId].iniPosition, localVar[thrId].game.noOfCards-4,
          localVar[thrId].initialMoves, first, TRUE, thrId);
      do {
        if (g==lowerbound)
          tricks=g+1;
        else
          tricks=g;
	assert((localVar[thrId].lookAheadPos.handRelFirst>=0)&&
		  (localVar[thrId].lookAheadPos.handRelFirst<=3));
        localVar[thrId].val=ABsearch(&localVar[thrId].lookAheadPos, tricks,
			localVar[thrId].iniDepth, thrId);

        if (localVar[thrId].val==TRUE)
          mv=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4];
        hiwinSetSize=Max(hiwinSetSize, localVar[thrId].winSetSize);
        hinodeSetSize=Max(hinodeSetSize, localVar[thrId].nodeSetSize);
	hilenSetSize=Max(hilenSetSize, localVar[thrId].lenSetSize);
        if (localVar[thrId].nodeSetSize>MaxnodeSetSize)
          MaxnodeSetSize=localVar[thrId].nodeSetSize;
        if (localVar[thrId].winSetSize>MaxwinSetSize)
          MaxwinSetSize=localVar[thrId].winSetSize;
	if (localVar[thrId].lenSetSize>MaxlenSetSize)
          MaxlenSetSize=localVar[thrId].lenSetSize;
        if (localVar[thrId].val==FALSE) {
	  upperbound=tricks-1;
	  g=upperbound;
	}
	else {
	  lowerbound=tricks;
	  g=lowerbound;
	}

        InitSearch(&localVar[thrId].iniPosition, localVar[thrId].game.noOfCards-4,
          localVar[thrId].initialMoves, first, TRUE, thrId);
      }
      while (lowerbound<upperbound);
      localVar[thrId].payOff=g;
      if (localVar[thrId].payOff==0) {
        last=localVar[thrId].movePly[localVar[thrId].iniDepth].last;
        futp->cards=temp.last+1;
        for (j=0; j<=last; j++) {
          futp->suit[ind-1+j]=localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[j].suit;
          futp->rank[ind-1+j]=localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[j].rank;
	    futp->equals[ind-1+j]=(localVar[thrId].movePly[localVar[thrId].game.noOfCards-4].move[j].sequence)<<2;
          futp->score[ind-1+j]=localVar[thrId].payOff;
        }
        break;
      }
      else {
        localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4]=mv;

        futp->cards=ind;
        futp->suit[ind-1]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].suit;
        futp->rank[ind-1]=localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].rank;
	  futp->equals[ind-1]=(localVar[thrId].bestMove[localVar[thrId].game.noOfCards-4].sequence)<<2;
        futp->score[ind-1]=localVar[thrId].payOff;
      }
    }
  }
  else if ((solutions==3)&&(localVar[thrId].payOff==0)) {
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
    localVar[thrId].forbiddenMoves[k].suit=0;
    localVar[thrId].forbiddenMoves[k].rank=0;
  }

  futp->nodes=localVar[thrId].trickNodes;
  #ifdef BENCH
  futp->totalNodes=localVar[thrId].nodes;
  #endif
  /*if ((wcount>0)||(ncount>0)||(lcount>0)) {
    localVar[thrId].fp2=fopen("dyn.txt", "a");
	fprintf(localVar[thrId].fp2, "wcount=%d, ncount=%d, lcount=%d\n",
	  wcount, ncount, lcount);
    fprintf(localVar[thrId].fp2, "winSetSize=%d, nodeSetSize=%d, lenSetSize=%d\n",
	  winSetSize, nodeSetSize, lenSetSize);
	fprintf(localVar[thrId].fp2, "\n");
    fclose(localVar[thrId].fp2);
  }*/

  /*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
  return 1;
}


int _initialized=0;

void InitStart(int gb_ram, int ncores) {
  int k, r, i, j, m;
  unsigned short int res;
  unsigned long long pcmem;	/* kbytes */

  if (_initialized)
    return;
  _initialized = 1;

  ttCollect=FALSE;
  suppressTTlog=FALSE;
  lastTTstore=0;
  ttStore = (struct ttStoreType *)calloc(SEARCHSIZE, sizeof(struct ttStoreType));
  if (ttStore==NULL)
    exit(1);

  if (gb_ram==0) {		/* Autoconfig */

  #ifdef _WIN32

    SYSTEM_INFO temp;

    MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);	/* Using GlobalMemoryStatusEx instead of GlobalMemoryStatus
					was suggested by Lorne Anderson. */

    pcmem=(unsigned long long)statex.ullTotalPhys/1024;

    if (pcmem < 1500000)
      noOfThreads=Min(MAXNOOFTHREADS, 2);
    else if (pcmem < 2500000)
      noOfThreads=Min(MAXNOOFTHREADS, 4);
    else
      noOfThreads=MAXNOOFTHREADS;

    GetSystemInfo(&temp);
    noOfCores=Min(noOfThreads, (int)temp.dwNumberOfProcessors);

  #endif
  #ifdef __linux__   /* The code for linux was suggested by Antony Lee. */
    FILE* fifo = popen("free -k | tail -n+3 | head -n1 | awk '{print $NF}'", "r");
    fscanf(fifo, "%ld", &pcmem);
    fclose(fifo);
  #endif

  }
  else {
    if (gb_ram < 2)
      noOfThreads=Min(MAXNOOFTHREADS, 2);
    else if (gb_ram < 3)
      noOfThreads=Min(MAXNOOFTHREADS, 4);
    else
      noOfThreads=Min(MAXNOOFTHREADS, 8);

    noOfCores=Min(noOfThreads, ncores);

    pcmem=(unsigned long long)(1000000 * gb_ram);
  }

  /*printf("noOfThreads: %d   noOfCores: %d\n", noOfThreads, noOfCores);*/

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

  max_low[0][0]=0; max_low[1][0]=0; max_low[2][0]=0;
  max_low[0][1]=0; max_low[1][1]=0; max_low[2][1]=0;
  max_low[0][2]=1; max_low[1][2]=1; max_low[2][2]=1;
  max_low[0][3]=0; max_low[1][3]=2; max_low[2][3]=2;
  max_low[0][4]=1; max_low[1][4]=0; max_low[2][4]=3;
  max_low[0][5]=2; max_low[1][5]=1; max_low[2][5]=0;
  max_low[0][6]=0; max_low[1][6]=0; max_low[2][6]=0;
  max_low[0][7]=0; max_low[1][7]=0; max_low[2][7]=0;

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

    localVar[k].rel = (struct relRanksType *)calloc(8192, sizeof(struct relRanksType));
    if (localVar[k].rel==NULL)
      exit(1);

    localVar[k].adaptWins = (struct adaptWinRanksType *)calloc(8192,
	  sizeof(struct adaptWinRanksType));
    if (localVar[k].adaptWins==NULL)
      exit(1);
  }

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

  /*localVar[thrId].fp2=fopen("dyn.txt", "w");
  fclose(localVar[thrId].fp2);*/
  /*localVar[thrId].fp2=fopen("dyn.txt", "a");
  fprintf(localVar[thrId].fp2, "maxIndex=%ld\n", maxIndex);
  fclose(localVar[thrId].fp2);*/

  return;
}


void InitGame(int gameNo, int moveTreeFlag, int first, int handRelFirst, int thrId) {

  int k, s, h, m, ord, r;
  unsigned int topBitRank=1;
  unsigned short int ind;

  #ifdef STAT
    localVar[thrId].fp2=fopen("stat.txt","w");
  #endif

  #ifdef TTDEBUG
  if (!suppressTTlog) {
    localVar[thrId].fp7=fopen("storett.txt","w");
    localVar[thrId].fp11=fopen("rectt.txt", "w");
    fclose(localVar[thrId].fp11);
    ttCollect=TRUE;
  }
  #endif


  if (localVar[thrId].newDeal) {

    /* Initialization of the rel structure is implemented
       according to a solution given by Thomas Andrews */

    for (k=0; k<=3; k++)
      for (m=0; m<=3; m++)
        localVar[thrId].iniPosition.rankInSuit[k][m]=localVar[thrId].game.suit[k][m];

    for (s=0; s<4; s++) {
      localVar[thrId].rel[0].aggrRanks[s]=0;
      localVar[thrId].rel[0].winMask[s]=0;
      for (ord=1; ord<=13; ord++) {
	localVar[thrId].rel[0].absRank[ord][s].hand=-1;
	localVar[thrId].rel[0].absRank[ord][s].rank=0;
      }
      for (r=2; r<=14; r++)
        localVar[thrId].rel[0].relRank[r][s]=0;
    }

    for (ind=1; ind<8192; ind++) {
      if (ind>=(topBitRank+topBitRank)) {
       /* Next top bit */
        topBitRank <<=1;
      }

      localVar[thrId].rel[ind]=localVar[thrId].rel[ind ^ topBitRank];

      for (s=0; s<4; s++) {
	ord=0;
	for (r=14; r>=2; r--) {
	  if ((ind & bitMapRank[r])!=0) {
	    ord++;
	    localVar[thrId].rel[ind].relRank[r][s]=ord;
	    for (h=0; h<4; h++) {
	      if ((localVar[thrId].game.suit[h][s] & bitMapRank[r])!=0) {
		localVar[thrId].rel[ind].absRank[ord][s].hand=h;
		localVar[thrId].rel[ind].absRank[ord][s].rank=r;
		break;
	      }
	    }
	  }
	}
	for (k=ord+1; k<=13; k++) {
	  localVar[thrId].rel[ind].absRank[k][s].hand=-1;
	  localVar[thrId].rel[ind].absRank[k][s].rank=0;
	}
	for (h=0; h<4; h++) {
	  if (localVar[thrId].game.suit[h][s] & topBitRank) {
	    localVar[thrId].rel[ind].aggrRanks[s]=
	          (localVar[thrId].rel[ind].aggrRanks[s]>>2)|(h<<24);
	    localVar[thrId].rel[ind].winMask[s]=
	          (localVar[thrId].rel[ind].winMask[s]>>2)|(3<<24);
	    break;
	  }
	}
      }
    }
  }

  localVar[thrId].iniPosition.first[localVar[thrId].game.noOfCards-4]=first;
  localVar[thrId].iniPosition.handRelFirst=handRelFirst;
  localVar[thrId].lookAheadPos=localVar[thrId].iniPosition;

  localVar[thrId].estTricks[1]=6;
  localVar[thrId].estTricks[3]=6;
  localVar[thrId].estTricks[0]=7;
  localVar[thrId].estTricks[2]=7;

  #ifdef STAT
  fprintf(localVar[thrId].fp2, "Estimated tricks for hand to play:\n");
  fprintf(localVar[thrId].fp2, "hand=%d  est tricks=%d\n",
	  localVar[thrId].handToPlay, localVar[thrId].estTricks[localVar[thrId].handToPlay]);
  #endif

  InitSearch(&localVar[thrId].lookAheadPos, localVar[thrId].game.noOfCards-4,
	localVar[thrId].initialMoves, first, moveTreeFlag, thrId);
  return;
}


void InitSearch(struct pos * posPoint, int depth, struct moveType startMoves[],
	int first, int mtd, int thrId)  {

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
    /*bestMove[d].suit=0;*/
    localVar[thrId].bestMove[d].rank=0;
    localVar[thrId].bestMoveTT[d].rank=0;
    /*bestMove[d].weight=0;
    bestMove[d].sequence=0; 0315 */
  }

  if (((handId(first, handRelFirst))==0)||
    ((handId(first, handRelFirst))==2)) {
    localVar[thrId].nodeTypeStore[0]=MAXNODE;
    localVar[thrId].nodeTypeStore[1]=MINNODE;
    localVar[thrId].nodeTypeStore[2]=MAXNODE;
    localVar[thrId].nodeTypeStore[3]=MINNODE;
  }
  else {
    localVar[thrId].nodeTypeStore[0]=MINNODE;
    localVar[thrId].nodeTypeStore[1]=MAXNODE;
    localVar[thrId].nodeTypeStore[2]=MINNODE;
    localVar[thrId].nodeTypeStore[3]=MAXNODE;
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
    localVar[thrId].movePly[depth+k].current=0;
    localVar[thrId].movePly[depth+k].last=0;
    localVar[thrId].movePly[depth+k].move[0].suit=startMoves[k-1].suit;
    localVar[thrId].movePly[depth+k].move[0].rank=startMoves[k-1].rank;
    if (k<noOfStartMoves) {     /* If there is more than one start move */
      if (WinningMove(&startMoves[k-1], &move, localVar[thrId].trump, thrId)) {
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
    localVar[thrId].iniRemovedRanks[s]=posPoint->removedRanks[s];

  /*for (d=0; d<=49; d++) {
    for (s=0; s<=3; s++)
      posPoint->winRanks[d][s]=0;
  }*/

  /* Initialize winning and second best ranks */
  for (s=0; s<=3; s++) {
    maxAgg=0;
    for (h=0; h<=3; h++) {
      aggHand[h][s]=startMovesBitMap[h][s] | localVar[thrId].game.suit[h][s];
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
    localVar[thrId].no[d]=0;
  }
  #endif

  if (!mtd) {
    localVar[thrId].lenSetSize=0;
    for (k=0; k<=13; k++) {
      for (h=0; h<=3; h++) {
	localVar[thrId].rootnp[k][h]=&localVar[thrId].posSearch[localVar[thrId].lenSetSize];
	localVar[thrId].posSearch[localVar[thrId].lenSetSize].suitLengths=0;
	localVar[thrId].posSearch[localVar[thrId].lenSetSize].posSearchPoint=NULL;
	localVar[thrId].posSearch[localVar[thrId].lenSetSize].left=NULL;
	localVar[thrId].posSearch[localVar[thrId].lenSetSize].right=NULL;
	localVar[thrId].lenSetSize++;
      }
    }
    localVar[thrId].nodeSetSize=0;
    localVar[thrId].winSetSize=0;
  }

  #ifdef TTDEBUG
  if (!suppressTTlog)
    lastTTstore=0;
  #endif

  return;
}

#ifdef STAT
int score1Counts[50], score0Counts[50];
int sumScore1Counts, sumScore0Counts, dd, suit, rank, order;
int c1[50], c2[50], c3[50], c4[50], c5[50], c6[50], c7[50], c8[50], c9[50];
int sumc1, sumc2, sumc3, sumc4, sumc5, sumc6, sumc7, sumc8, sumc9;
#endif

int ABsearch(struct pos * posPoint, int target, int depth, int thrId) {
    /* posPoint points to the current look-ahead position,
       target is number of tricks to take for the player,
       depth is the remaining search length, must be positive,
       the value of the subtree is returned.  */

  int moveExists, mexists, value, hand, scoreFlag, found, trump;
  int ready, hfirst, hh, ss, rr, mcurrent, qtricks, tricks, res, k;
  unsigned short int makeWinRank[4];
  struct nodeCardsType * cardsP;
  struct evalType evalData;
  struct winCardType * np;
  struct posSearchType * pp;
  struct nodeCardsType  * tempP;
  struct movePlyType *mply;
  unsigned short int aggr[4];
  unsigned short int ranks;
  long long suitLengths;

  struct evalType Evaluate(struct pos * posPoint, int trump, int thrId);
  void Make(struct pos * posPoint, unsigned short int trickCards[4],
    int depth, int trump, struct movePlyType *mply, int thrId);
  void Undo(struct pos * posPoint, int depth, struct movePlyType *mply, int thrId);

  /*cardsP=NULL;*/
  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
  trump=localVar[thrId].trump;
  hand=handId(posPoint->first[depth], posPoint->handRelFirst);
  localVar[thrId].nodes++;
  if (posPoint->handRelFirst==0) {
    localVar[thrId].trickNodes++;
    if (posPoint->tricksMAX>=target) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

        #ifdef STAT
        c1[depth]++;

        score1Counts[depth]++;
        if (depth==localVar[thrId].iniDepth) {
          fprintf(localVar[thrId].fp2, "score statistics:\n");
          for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
            fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
            score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
              c3[dd], c4[dd]);
            fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
              c6[dd], c7[dd], c8[dd]);
          }
        }
        #endif

      return TRUE;
    }
    if (((posPoint->tricksMAX+(depth>>2)+1)<target)/*&&(depth>0)*/) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

 #ifdef STAT
        c2[depth]++;
        score0Counts[depth]++;
        if (depth==localVar[thrId].iniDepth) {
          fprintf(localVar[thrId].fp2, "score statistics:\n");
          for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
            fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
            score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
              c3[dd], c4[dd]);
            fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
              c6[dd], c7[dd], c8[dd]);
          }
        }
 #endif

      return FALSE;
    }

    if (localVar[thrId].nodeTypeStore[hand]==MAXNODE) {
      qtricks=QuickTricks(posPoint, hand, depth, target, trump, &res, thrId);
      if (res) {
	if (qtricks==0)
	  return FALSE;
	else
          return TRUE;
 #ifdef STAT
          c3[depth]++;
          score1Counts[depth]++;
          if (depth==localVar[thrId].iniDepth) {
            fprintf(localVar[thrId].fp2, "score statistics:\n");
            for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
              fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                c3[dd], c4[dd]);
              fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
      }
      if (!LaterTricksMIN(posPoint,hand,depth,target,trump,thrId))
	return FALSE;
    }
    else {
      qtricks=QuickTricks(posPoint, hand, depth, target, trump, &res, thrId);
      if (res) {
        if (qtricks==0)
	  return TRUE;
	else
          return FALSE;
 #ifdef STAT
          c4[depth]++;
          score0Counts[depth]++;
          if (depth==localVar[thrId].iniDepth) {
            fprintf(localVar[thrId].fp2, "score statistics:\n");
            for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
              fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                c3[dd], c4[dd]);
              fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
      }
      if (LaterTricksMAX(posPoint,hand,depth,target,trump,thrId))
	return TRUE;
    }
  }

  else if (posPoint->handRelFirst==1) {
    ss=posPoint->move[depth+1].suit;
    ranks=posPoint->rankInSuit[hand][ss] |
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

    if ((found)&&(depth!=localVar[thrId].iniDepth)) {
      for (k=0; k<=3; k++)
	posPoint->winRanks[depth][k]=0;
	if (rr!=0)
	  posPoint->winRanks[depth][ss]=bitMapRank[rr];

      if (localVar[thrId].nodeTypeStore[hand]==MAXNODE) {
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
	    if (posPoint->tricksMAX+qtricks>=target) {
	      return TRUE;
	    }
	  }

	  for (k=0; k<=3; k++) {
	    if ((k!=ss)&&(posPoint->length[hh][k]!=0))  {	/* Not lead suit, not void in suit */
	      if ((posPoint->length[lho[hh]][k]==0)&&(posPoint->length[rho[hh]][k]==0)
		&&(posPoint->length[partner[hh]][k]==0)) {
		qtricks+=counttable[posPoint->rankInSuit[hh][k]];
		if (posPoint->tricksMAX+qtricks>=target) {
		  return TRUE;
		}
	      }
	      else if ((posPoint->winner[k].rank!=0)&&(posPoint->winner[k].hand==hh)) {
		qtricks++;
		posPoint->winRanks[depth][k]|=bitMapRank[posPoint->winner[k].rank];
		if (posPoint->tricksMAX+qtricks>=target) {
		  return TRUE;
		}
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
	    if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) {
	      return FALSE;
	    }
	  }

	  for (k=0; k<=3; k++) {
	    if ((k!=ss)&&(posPoint->length[hh][k]!=0))  {	/* Not lead suit, not void in suit */
	      if ((posPoint->length[lho[hh]][k]==0)&&(posPoint->length[rho[hh]][k]==0)
		  &&(posPoint->length[partner[hh]][k]==0)) {
		qtricks+=counttable[posPoint->rankInSuit[hh][k]];
		if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) {
		  return FALSE;
		}
	      }
	      else if ((posPoint->winner[k].rank!=0)&&(posPoint->winner[k].hand==hh)) {
		qtricks++;
		posPoint->winRanks[depth][k]|=bitMapRank[posPoint->winner[k].rank];
		if ((posPoint->tricksMAX+((depth)>>2)+3-qtricks)<=target) {
		  return FALSE;
		}
	      }
	    }
	  }
	}
      }
    }
  }

  if ((posPoint->handRelFirst==0)&&
    (depth!=localVar[thrId].iniDepth)) {
    for (ss=0; ss<=3; ss++) {
      aggr[ss]=0;
      for (hh=0; hh<=3; hh++)
	aggr[ss]|=posPoint->rankInSuit[hh][ss];
      posPoint->orderSet[ss]=localVar[thrId].rel[aggr[ss]].aggrRanks[ss];
    }
    tricks=depth>>2;
    suitLengths=0;
    for (ss=0; ss<=2; ss++)
      for (hh=0; hh<=3; hh++) {
	suitLengths<<=4;
	suitLengths|=posPoint->length[hh][ss];
      }

    pp=SearchLenAndInsert(localVar[thrId].rootnp[tricks][hand],
	 suitLengths, FALSE, &res, thrId);
	/* Find node that fits the suit lengths */
    if (pp!=NULL) {
      np=pp->posSearchPoint;
      if (np==NULL)
        cardsP=NULL;
      else
        cardsP=FindSOP(posPoint, np, hand, target, tricks, &scoreFlag, thrId);

      if (cardsP!=NULL) {
        if (scoreFlag==1) {
	  for (ss=0; ss<=3; ss++)
	    posPoint->winRanks[depth][ss]=
	      localVar[thrId].adaptWins[aggr[ss]].winRanks[(int)cardsP->leastWin[ss]];

          if (cardsP->bestMoveRank!=0) {
            localVar[thrId].bestMoveTT[depth].suit=cardsP->bestMoveSuit;
            localVar[thrId].bestMoveTT[depth].rank=cardsP->bestMoveRank;
          }
 #ifdef STAT
          c5[depth]++;
          if (scoreFlag==1)
            score1Counts[depth]++;
          else
            score0Counts[depth]++;
          if (depth==localVar[thrId].iniDepth) {
            fprintf(localVar[thrId].fp2, "score statistics:\n");
            for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
              fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
                score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],c3[dd], c4[dd]);
              fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif
 #ifdef TTDEBUG
          if (!suppressTTlog) {
            if (lastTTstore<SEARCHSIZE)
              ReceiveTTstore(posPoint, cardsP, target, depth, thrId);
            else
              ttCollect=FALSE;
	  }
 #endif
          return TRUE;
	}
        else {
	  for (ss=0; ss<=3; ss++)
	    posPoint->winRanks[depth][ss]=
	      localVar[thrId].adaptWins[aggr[ss]].winRanks[(int)cardsP->leastWin[ss]];

          if (cardsP->bestMoveRank!=0) {
            localVar[thrId].bestMoveTT[depth].suit=cardsP->bestMoveSuit;
            localVar[thrId].bestMoveTT[depth].rank=cardsP->bestMoveRank;
          }
 #ifdef STAT
          c6[depth]++;
          if (scoreFlag==1)
            score1Counts[depth]++;
          else
            score0Counts[depth]++;
          if (depth==localVar[thrId].iniDepth) {
            fprintf(localVar[thrId].fp2, "score statistics:\n");
            for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
              fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
                score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd],
                  c4[dd]);
              fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                  c6[dd], c7[dd], c8[dd]);
            }
          }
 #endif

 #ifdef TTDEBUG
          if (!suppressTTlog) {
            if (lastTTstore<SEARCHSIZE)
              ReceiveTTstore(posPoint, cardsP, target, depth, thrId);
            else
              ttCollect=FALSE;
          }
 #endif
          return FALSE;
	}
      }
    }
  }

  if (depth==0) {                    /* Maximum depth? */
    evalData=Evaluate(posPoint, trump, thrId);        /* Leaf node */
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
        if (depth==localVar[thrId].iniDepth) {
          fprintf(localVar[thrId].fp2, "score statistics:\n");
          for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
            fprintf(localVar[thrId].fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd],
              c4[dd]);
            fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
              c6[dd], c7[dd], c8[dd]);
          }
        }
 #endif
    }
    return value;
  }
  else {
    mply=&localVar[thrId].movePly[depth];
    moveExists=MoveGen(posPoint, depth, trump, mply, thrId);

/*#if 0*/
    if ((posPoint->handRelFirst==3)&&(depth>=/*29*/33/*37*/)
	&&(depth!=localVar[thrId].iniDepth)) {
    /*localVar[thrId].movePly[depth].current=0;*/
      mply->current=0;
      mexists=TRUE;
      ready=FALSE;
      while (mexists) {
	Make(posPoint, makeWinRank, depth, trump, mply, thrId);
	depth--;

	for (ss=0; ss<=3; ss++) {
	  aggr[ss]=0;
	  for (hh=0; hh<=3; hh++)
	    aggr[ss]|=posPoint->rankInSuit[hh][ss];
	  posPoint->orderSet[ss]=localVar[thrId].rel[aggr[ss]].aggrRanks[ss];
	}
	tricks=depth>>2;
	hfirst=posPoint->first[depth];
	suitLengths=0;
	for (ss=0; ss<=2; ss++)
          for (hh=0; hh<=3; hh++) {
	    suitLengths<<=4;
	    suitLengths|=posPoint->length[hh][ss];
	  }

	pp=SearchLenAndInsert(localVar[thrId].rootnp[tricks][hfirst],
	  suitLengths, FALSE, &res, thrId);
	/* Find node that fits the suit lengths */
	if (pp!=NULL) {
	  np=pp->posSearchPoint;
	  if (np==NULL)
	    tempP=NULL;
	  else
	    tempP=FindSOP(posPoint, np, hfirst, target, tricks, &scoreFlag, thrId);

	  if (tempP!=NULL) {
	    if ((localVar[thrId].nodeTypeStore[hand]==MAXNODE)&&(scoreFlag==1)) {
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]=
		  localVar[thrId].adaptWins[aggr[ss]].winRanks[(int)tempP->leastWin[ss]];
	      if (tempP->bestMoveRank!=0) {
		localVar[thrId].bestMoveTT[depth+1].suit=tempP->bestMoveSuit;
		localVar[thrId].bestMoveTT[depth+1].rank=tempP->bestMoveRank;
	      }
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]|=makeWinRank[ss];
	      Undo(posPoint, depth+1, &localVar[thrId].movePly[depth+1],thrId);
	      return TRUE;
	    }
	    else if ((localVar[thrId].nodeTypeStore[hand]==MINNODE)&&(scoreFlag==0)) {
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]=
		  localVar[thrId].adaptWins[aggr[ss]].winRanks[(int)tempP->leastWin[ss]];
	      if (tempP->bestMoveRank!=0) {
		localVar[thrId].bestMoveTT[depth+1].suit=tempP->bestMoveSuit;
		localVar[thrId].bestMoveTT[depth+1].rank=tempP->bestMoveRank;
	      }
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]|=makeWinRank[ss];
	      Undo(posPoint, depth+1, &localVar[thrId].movePly[depth+1], thrId);
		return FALSE;
	    }
	    else {
	      localVar[thrId].movePly[depth+1].move[localVar[thrId].movePly[depth+1].current].weight+=100;
	      ready=TRUE;
	    }
	  }
	}
	depth++;
	Undo(posPoint, depth, mply, thrId);
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
    if (localVar[thrId].nodeTypeStore[hand]==MAXNODE) {
      value=FALSE;
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

      while (moveExists)  {
        Make(posPoint, makeWinRank, depth, trump, mply, thrId);        /* Make current move */

        assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
	value=ABsearch(posPoint, target, depth-1, thrId);

        Undo(posPoint, depth, mply, thrId);      /* Retract current move */
        if (value==TRUE) {
        /* A cut-off? */
	  for (ss=0; ss<=3; ss++)
            posPoint->winRanks[depth][ss]=posPoint->winRanks[depth-1][ss] |
              makeWinRank[ss];
	  mcurrent=mply->current;
	  localVar[thrId].bestMove[depth]=mply->move[mcurrent];
          goto ABexit;
        }
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth][ss] |
            posPoint->winRanks[depth-1][ss] | makeWinRank[ss];

        moveExists=NextMove(posPoint, depth, mply, thrId);
      }
    }
    else {                          /* A minnode */
      value=TRUE;
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

      while (moveExists)  {
        Make(posPoint, makeWinRank, depth, trump, mply, thrId);        /* Make current move */

        value=ABsearch(posPoint, target, depth-1, thrId);

        Undo(posPoint, depth, mply, thrId);       /* Retract current move */
        if (value==FALSE) {
        /* A cut-off? */
	  for (ss=0; ss<=3; ss++)
            posPoint->winRanks[depth][ss]=posPoint->winRanks[depth-1][ss] |
              makeWinRank[ss];
	  mcurrent=mply->current;
          localVar[thrId].bestMove[depth]=mply->move[mcurrent];
          goto ABexit;
        }
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth][ss] |
            posPoint->winRanks[depth-1][ss] | makeWinRank[ss];

        moveExists=NextMove(posPoint, depth, mply, thrId);
      }
    }
  }
  ABexit:
  if (depth>=4) {
    if(posPoint->handRelFirst==0) {
      tricks=depth>>2;
      if (value)
	k=target;
      else
	k=target-1;
      if (depth!=localVar[thrId].iniDepth)
        BuildSOP(posPoint, tricks, hand, target, depth,
          value, k, thrId);
      if (localVar[thrId].clearTTflag) {
         /* Wipe out the TT dynamically allocated structures
	    except for the initially allocated structures.
	    Set the TT limits to the initial values.
	    Reset TT array indices to zero.
	    Reset memory chunk indices to zero.
	    Set allocated memory to the initial value. */
        /*localVar[thrId].fp2=fopen("dyn.txt", "a");
	fprintf(localVar[thrId].fp2, "Clear TT:\n");
	fprintf(localVar[thrId].fp2, "wcount=%d, ncount=%d, lcount=%d\n",
	       wcount, ncount, lcount);
        fprintf(localVar[thrId].fp2, "winSetSize=%d, nodeSetSize=%d, lenSetSize=%d\n",
	       winSetSize, nodeSetSize, lenSetSize);
	fprintf(localVar[thrId].fp2, "\n");
        fclose(localVar[thrId].fp2);*/

        Wipe(thrId);
	localVar[thrId].winSetSizeLimit=WINIT;
	localVar[thrId].nodeSetSizeLimit=NINIT;
	localVar[thrId].lenSetSizeLimit=LINIT;
	localVar[thrId].lcount=0;
	localVar[thrId].allocmem=(localVar[thrId].lenSetSizeLimit+1)*sizeof(struct posSearchType);
	localVar[thrId].lenSetSize=0;
	localVar[thrId].posSearch=localVar[thrId].pl[localVar[thrId].lcount];
	for (k=0; k<=13; k++) {
	  for (hh=0; hh<=3; hh++) {
	    localVar[thrId].rootnp[k][hh]=&localVar[thrId].posSearch[localVar[thrId].lenSetSize];
	    localVar[thrId].posSearch[localVar[thrId].lenSetSize].suitLengths=0;
	    localVar[thrId].posSearch[localVar[thrId].lenSetSize].posSearchPoint=NULL;
	    localVar[thrId].posSearch[localVar[thrId].lenSetSize].left=NULL;
	    localVar[thrId].posSearch[localVar[thrId].lenSetSize].right=NULL;
	    localVar[thrId].lenSetSize++;
	  }
	}
        localVar[thrId].nodeSetSize=0;
        localVar[thrId].winSetSize=0;
	localVar[thrId].wcount=0; localVar[thrId].ncount=0;
	localVar[thrId].allocmem+=(localVar[thrId].winSetSizeLimit+1)*sizeof(struct winCardType);
        localVar[thrId].winCards=localVar[thrId].pw[localVar[thrId].wcount];
	localVar[thrId].allocmem+=(localVar[thrId].nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
	localVar[thrId].nodeCards=localVar[thrId].pn[localVar[thrId].ncount];
	localVar[thrId].clearTTflag=FALSE;
	localVar[thrId].windex=-1;
      }
    }
  }

 #ifdef STAT
  c8[depth]++;
  if (value==1)
    score1Counts[depth]++;
  else
    score0Counts[depth]++;
  if (depth==localVar[thrId].iniDepth) {
    if (localVar[thrId].fp2==NULL)
      exit(0);
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "top level cards:\n");
    for (hh=0; hh<=3; hh++) {
      fprintf(localVar[thrId].fp2, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
        fprintf(localVar[thrId].fp2, "suit=%c", cardSuit[ss]);
        for (rr=14; rr>=2; rr--)
          if (posPoint->rankInSuit[hh][ss] & bitMapRank[rr])
            fprintf(localVar[thrId].fp2, " %c", cardRank[rr]);
        fprintf(localVar[thrId].fp2, "\n");
      }
      fprintf(localVar[thrId].fp2, "\n");
    }
    fprintf(localVar[thrId].fp2, "top level winning cards:\n");
    for (ss=0; ss<=3; ss++) {
      fprintf(localVar[thrId].fp2, "suit=%c", cardSuit[ss]);
      for (rr=14; rr>=2; rr--)
        if (posPoint->winRanks[depth][ss] & bitMapRank[rr])
          fprintf(localVar[thrId].fp2, " %c", cardRank[rr]);
      fprintf(localVar[thrId].fp2, "\n");
    }
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "\n");

    fprintf(localVar[thrId].fp2, "score statistics:\n");
    sumScore0Counts=0;
    sumScore1Counts=0;
    sumc1=0; sumc2=0; sumc3=0; sumc4=0;
    sumc5=0; sumc6=0; sumc7=0; sumc8=0; sumc9=0;
    for (dd=localVar[thrId].iniDepth; dd>=0; dd--) {
      fprintf(localVar[thrId].fp2, "depth=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
          score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd], c4[dd]);
      fprintf(localVar[thrId].fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd], c6[dd],
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
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "score sum statistics:\n");
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "sumScore0Counts=%d sumScore1Counts=%d\n",
        sumScore0Counts, sumScore1Counts);
    fprintf(localVar[thrId].fp2, "nodeSetSize=%d  winSetSize=%d\n", localVar[thrId].nodeSetSize,
        localVar[thrId].winSetSize);
    fprintf(localVar[thrId].fp2, "sumc1=%d sumc2=%d sumc3=%d sumc4=%d\n",
        sumc1, sumc2, sumc3, sumc4);
    fprintf(localVar[thrId].fp2, "sumc5=%d sumc6=%d sumc7=%d sumc8=%d sumc9=%d\n",
        sumc5, sumc6, sumc7, sumc8, sumc9);
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "No of searched nodes per depth:\n");
    for (dd=localVar[thrId].iniDepth; dd>=0; dd--)
      fprintf(localVar[thrId].fp2, "depth=%d  nodes=%d\n", dd, localVar[thrId].no[dd]);
    fprintf(localVar[thrId].fp2, "\n");
    fprintf(localVar[thrId].fp2, "Total nodes=%d\n", localVar[thrId].nodes);
  }
 #endif

  return value;
}


void Make(struct pos * posPoint, unsigned short int trickCards[4],
  int depth, int trump, struct movePlyType *mply, int thrId)  {
  int r, s, t, u, w, firstHand;
  int suit, count, mcurr, h, q, done;
  struct moveType mo1, mo2;
  unsigned short aggr;

  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));
  for (suit=0; suit<=3; suit++)
    trickCards[suit]=0;

  firstHand=posPoint->first[depth];
  r=mply->current;

  if (posPoint->handRelFirst==3)  {         /* This hand is last hand */
    mo1=mply->move[r];
    mo2=posPoint->move[depth+1];
    if (mo1.suit==mo2.suit) {
      if (mo1.rank>mo2.rank) {
	posPoint->move[depth]=mo1;
        posPoint->high[depth]=handId(firstHand, 3);
      }
      else {
        posPoint->move[depth]=posPoint->move[depth+1];
        posPoint->high[depth]=posPoint->high[depth+1];
      }
    }
    else if (mo1.suit==trump) {
      posPoint->move[depth]=mo1;
      posPoint->high[depth]=handId(firstHand, 3);
    }
    else {
      posPoint->move[depth]=posPoint->move[depth+1];
      posPoint->high[depth]=posPoint->high[depth+1];
    }

    /* Is the trick won by rank? */
    suit=posPoint->move[depth].suit;
    count=0;
    mcurr=mply->current;
    if (mply->move[mcurr].suit==suit)
      count++;
    for (h=1; h<=3; h++) {
      mcurr=localVar[thrId].movePly[depth+h].current;
      if (localVar[thrId].movePly[depth+h].move[mcurr].suit==suit)
        count++;
    }

    if (localVar[thrId].nodeTypeStore[posPoint->high[depth]]==MAXNODE)
      posPoint->tricksMAX++;
    posPoint->first[depth-1]=posPoint->high[depth];   /* Defines who is first
        in the next move */

    t=handId(firstHand, 3);
    posPoint->handRelFirst=0;      /* Hand pointed to by posPoint->first
                                    will lead the next trick */

    done=FALSE;
    for (s=3; s>=0; s--) {
      q=handId(firstHand, 3-s);
    /* Add the moves to removed ranks */
      r=localVar[thrId].movePly[depth+s].current;
      w=localVar[thrId].movePly[depth+s].move[r].rank;
      u=localVar[thrId].movePly[depth+s].move[r].suit;
      posPoint->removedRanks[u]|=bitMapRank[w];

      if (s==0)
        posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);

      if ((w==posPoint->winner[u].rank)||(w==posPoint->secondBest[u].rank)) {
	aggr=0;
        for (h=0; h<=3; h++)
	  aggr|=posPoint->rankInSuit[h][u];
	posPoint->winner[u].rank=localVar[thrId].rel[aggr].absRank[1][u].rank;
	posPoint->winner[u].hand=localVar[thrId].rel[aggr].absRank[1][u].hand;
	posPoint->secondBest[u].rank=localVar[thrId].rel[aggr].absRank[2][u].rank;
	posPoint->secondBest[u].hand=localVar[thrId].rel[aggr].absRank[2][u].hand;
      }

    /* Determine win-ranked cards */
      if ((q==posPoint->high[depth])&&(!done)) {
        done=TRUE;
        if (count>=2) {
          trickCards[u]=bitMapRank[w];
          /* Mark ranks as winning if they are part of a sequence */
          trickCards[u]|=localVar[thrId].movePly[depth+s].move[r].sequence;
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
    mo1=mply->move[r];
    mo2=posPoint->move[depth+1];
    r=mply->current;
    u=mply->move[r].suit;
    w=mply->move[r].rank;
    if (mo1.suit==mo2.suit) {
      if (mo1.rank>mo2.rank) {
	posPoint->move[depth]=mo1;
        posPoint->high[depth]=handId(firstHand, posPoint->handRelFirst);
      }
      else {
	posPoint->move[depth]=posPoint->move[depth+1];
        posPoint->high[depth]=posPoint->high[depth+1];
      }
    }
    else if (mo1.suit==trump) {
      posPoint->move[depth]=mo1;
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
  localVar[thrId].no[depth]++;
#endif

  return;
}


void Undo(struct pos * posPoint, int depth, struct movePlyType *mply, int thrId)  {
  int r, s, t, u, w, firstHand;

  assert((posPoint->handRelFirst>=0)&&(posPoint->handRelFirst<=3));

  firstHand=posPoint->first[depth];

  switch (posPoint->handRelFirst) {
    case 3: case 2: case 1:
     posPoint->handRelFirst--;
     break;
    case 0:
     posPoint->handRelFirst=3;
  }

  if (posPoint->handRelFirst==0) {          /* 1st hand which won the previous
                                            trick */
    t=firstHand;
    r=mply->current;
    u=mply->move[r].suit;
    w=mply->move[r].rank;
  }
  else if (posPoint->handRelFirst==3)  {    /* Last hand */
    for (s=3; s>=0; s--) {
    /* Delete the moves from removed ranks */
      r=localVar[thrId].movePly[depth+s].current;
      w=localVar[thrId].movePly[depth+s].move[r].rank;
      u=localVar[thrId].movePly[depth+s].move[r].suit;

      posPoint->removedRanks[u]&= (~bitMapRank[w]);

      if (w>posPoint->winner[u].rank) {
        posPoint->secondBest[u].rank=posPoint->winner[u].rank;
        posPoint->secondBest[u].hand=posPoint->winner[u].hand;
        posPoint->winner[u].rank=w;
        posPoint->winner[u].hand=handId(firstHand, 3-s);

      }
      else if (w>posPoint->secondBest[u].rank) {
        posPoint->secondBest[u].rank=w;
        posPoint->secondBest[u].hand=handId(firstHand, 3-s);
      }
    }
    t=handId(firstHand, 3);


    if (localVar[thrId].nodeTypeStore[posPoint->first[depth-1]]==MAXNODE)
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



struct evalType Evaluate(struct pos * posPoint, int trump, int thrId)  {
  int s, smax=0, max, k, firstHand, count;
  struct evalType eval;

  firstHand=posPoint->first[0];
  assert((firstHand >= 0)&&(firstHand <= 3));

  for (s=0; s<=3; s++)
    eval.winRanks[s]=0;

  /* Who wins the last trick? */
  if (trump!=4)  {            /* Highest trump card wins */
    max=0;
    count=0;
    for (s=0; s<=3; s++) {
      if (posPoint->rankInSuit[s][trump]!=0)
        count++;
      if (posPoint->rankInSuit[s][trump]>max) {
        smax=s;
        max=posPoint->rankInSuit[s][trump];
      }
    }

    if (max>0) {        /* Trumpcard wins */
      if (count>=2)
        eval.winRanks[trump]=max;

      if (localVar[thrId].nodeTypeStore[smax]==MAXNODE)
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

  count=0;
  max=0;
  for (s=0; s<=3; s++)  {
    if (posPoint->rankInSuit[s][k]!=0)
      count++;
    if (posPoint->rankInSuit[s][k]>max)  {
      smax=s;
      max=posPoint->rankInSuit[s][k];
    }
  }

  if (count>=2)
    eval.winRanks[k]=max;

  if (localVar[thrId].nodeTypeStore[smax]==MAXNODE)
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
	int depth, int target, int trump, int *result, int thrId) {
  int suit, sum, qtricks, commPartner, commRank=0, commSuit=-1, s, found=FALSE;
  int opps, res;
  int countLho, countRho, countPart, countOwn, lhoTrumpRanks, rhoTrumpRanks;
  int cutoff, ss, rr, lowestQtricks=0, count=0/*, ruff=FALSE*/;

  int QtricksLeadHandNT(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int *lhoTrumpRanks, int *rhoTrumpRanks, int commPartner,
	int commSuit, int countOwn, int countPart, int suit, int qtricks, int trump, int *res);

  int QtricksLeadHandTrump(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int trump, int *res);

  int QuickTricksPartnerHandTrump(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, int thrId);

  int QuickTricksPartnerHandNT(int hand, struct pos *posPoint, int cutoff, int depth,
	int countLho, int countRho, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, int thrId);

  *result=TRUE;
  qtricks=0;
  for (s=0; s<=3; s++)
    posPoint->winRanks[depth][s]=0;

  if ((depth<=0)||(depth==localVar[thrId].iniDepth)) {
    *result=FALSE;
    return qtricks;
  }

  if (localVar[thrId].nodeTypeStore[hand]==MAXNODE)
    cutoff=target-posPoint->tricksMAX;
  else
    cutoff=posPoint->tricksMAX-target+(depth>>2)+2;

  commPartner=FALSE;
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
	      countPart, suit, qtricks, commSuit, commRank, trump, &res, thrId);
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
	      countPart, suit, qtricks, commSuit, commRank, trump, &res, thrId);
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
      if (localVar[thrId].nodeTypeStore[hand]!=MAXNODE)
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

  int qt;

  *res=1;
  qt=qtricks;
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

  int qt;

  *res=1;
  qt=qtricks;
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
	/*if (trump==suit) {*/
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
    /*if (trump==suit) {*/
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
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, int thrId) {
	/* res=0		Continue with same suit.
	   res=1		Cutoff.
	   res=2		Continue with next suit. */

  int qt, k;
  unsigned short ranks;

  *res=1;
  qt=qtricks;
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
    ranks=0;
    for (k=0; k<=3; k++)
      ranks|=posPoint->rankInSuit[k][suit];
    if (localVar[thrId].rel[ranks].absRank[3][suit].hand==partner[hand]) {
	  posPoint->winRanks[depth][suit]|=bitMapRank[localVar[thrId].rel[ranks].absRank[3][suit].rank];
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
	int countPart, int suit, int qtricks, int commSuit, int commRank, int trump, int *res, int thrId) {

  int qt, k/*, found, rr*/;
  unsigned short ranks;

  *res=1;
  qt=qtricks;

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
    ranks=0;
    for (k=0; k<=3; k++)
      ranks=ranks | posPoint->rankInSuit[k][suit];
    if (localVar[thrId].rel[ranks].absRank[3][suit].hand==partner[hand]) {
      posPoint->winRanks[depth][suit]|=bitMapRank[localVar[thrId].rel[ranks].absRank[3][suit].rank];
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
	int trump, int thrId) {
  int hh, ss, k, h, sum=0;
  unsigned short aggr;

  if ((trump==4)||(posPoint->winner[trump].rank==0)) {
    for (ss=0; ss<=3; ss++) {
      hh=posPoint->winner[ss].hand;
      if (hh!=-1) {
        if (localVar[thrId].nodeTypeStore[hh]==MAXNODE)
          sum+=Max(posPoint->length[hh][ss], posPoint->length[partner[hh]][ss]);
      }
    }
    if ((posPoint->tricksMAX+sum<target)&&
      (sum>0)&&(depth>0)&&(depth!=localVar[thrId].iniDepth)) {
      if ((posPoint->tricksMAX+(depth>>2)<target)) {
	for (ss=0; ss<=3; ss++) {
	  if (posPoint->winner[ss].hand==-1)
	    posPoint->winRanks[depth][ss]=0;
          else if (localVar[thrId].nodeTypeStore[posPoint->winner[ss].hand]==MINNODE) {
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
    (localVar[thrId].nodeTypeStore[posPoint->winner[trump].hand]==MINNODE)) {
    if ((posPoint->length[hand][trump]==0)&&
      (posPoint->length[partner[hand]][trump]==0)) {
      if (((posPoint->tricksMAX+(depth>>2)+1-
	  Max(posPoint->length[lho[hand]][trump],
	  posPoint->length[rho[hand]][trump]))<target)
          &&(depth>0)&&(depth!=localVar[thrId].iniDepth)) {
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=0;
	return FALSE;
      }
    }
    else if (((posPoint->tricksMAX+(depth>>2))<target)&&
      (depth>0)&&(depth!=localVar[thrId].iniDepth)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
      posPoint->winRanks[depth][trump]=
	  bitMapRank[posPoint->winner[trump].rank];
      return FALSE;
    }
    else {
      hh=posPoint->secondBest[trump].hand;
      if (hh!=-1) {
        if ((localVar[thrId].nodeTypeStore[hh]==MINNODE)&&(posPoint->secondBest[trump].rank!=0))  {
          if (((posPoint->length[hh][trump]>1) ||
            (posPoint->length[partner[hh]][trump]>1))&&
            ((posPoint->tricksMAX+(depth>>2)-1)<target)&&(depth>0)
	     &&(depth!=localVar[thrId].iniDepth)) {
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
      if ((localVar[thrId].nodeTypeStore[hh]==MINNODE)&&
        (posPoint->length[hh][trump]>1)) {
	if (posPoint->winner[trump].hand==rho[hh]) {
          if (((posPoint->tricksMAX+(depth>>2))<target)&&
            (depth>0)&&(depth!=localVar[thrId].iniDepth)) {
            for (ss=0; ss<=3; ss++)
              posPoint->winRanks[depth][ss]=0;
	        posPoint->winRanks[depth][trump]=
              bitMapRank[posPoint->secondBest[trump].rank];
            return FALSE;
	  }
	}
	else {
	  aggr=0;
	  for (k=0; k<=3; k++)
	    aggr|=posPoint->rankInSuit[k][trump];
	  h=localVar[thrId].rel[aggr].absRank[3][trump].hand;
	  if (h!=-1) {
	    if ((localVar[thrId].nodeTypeStore[h]==MINNODE)&&
	      ((posPoint->tricksMAX+(depth>>2))<target)&&
              (depth>0)&&(depth!=localVar[thrId].iniDepth)) {
              for (ss=0; ss<=3; ss++)
                posPoint->winRanks[depth][ss]=0;
	      posPoint->winRanks[depth][trump]=
		bitMapRank[localVar[thrId].rel[aggr].absRank[3][trump].rank];
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
	int trump, int thrId) {
  int hh, ss, k, h, sum=0;
  unsigned short aggr;

  if ((trump==4)||(posPoint->winner[trump].rank==0)) {
    for (ss=0; ss<=3; ss++) {
      hh=posPoint->winner[ss].hand;
      if (hh!=-1) {
        if (localVar[thrId].nodeTypeStore[hh]==MINNODE)
          sum+=Max(posPoint->length[hh][ss], posPoint->length[partner[hh]][ss]);
      }
    }
    if ((posPoint->tricksMAX+(depth>>2)+1-sum>=target)&&
	(sum>0)&&(depth>0)&&(depth!=localVar[thrId].iniDepth)) {
      if ((posPoint->tricksMAX+1>=target)) {
	for (ss=0; ss<=3; ss++) {
	  if (posPoint->winner[ss].hand==-1)
	    posPoint->winRanks[depth][ss]=0;
          else if (localVar[thrId].nodeTypeStore[posPoint->winner[ss].hand]==MAXNODE) {
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
    (localVar[thrId].nodeTypeStore[posPoint->winner[trump].hand]==MAXNODE)) {
    if ((posPoint->length[hand][trump]==0)&&
      (posPoint->length[partner[hand]][trump]==0)) {
      if (((posPoint->tricksMAX+Max(posPoint->length[lho[hand]][trump],
        posPoint->length[rho[hand]][trump]))>=target)
        &&(depth>0)&&(depth!=localVar[thrId].iniDepth)) {
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=0;
	return TRUE;
      }
    }
    else if (((posPoint->tricksMAX+1)>=target)
      &&(depth>0)&&(depth!=localVar[thrId].iniDepth)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
      posPoint->winRanks[depth][trump]=
	  bitMapRank[posPoint->winner[trump].rank];
      return TRUE;
    }
    else {
      hh=posPoint->secondBest[trump].hand;
      if (hh!=-1) {
        if ((localVar[thrId].nodeTypeStore[hh]==MAXNODE)&&(posPoint->secondBest[trump].rank!=0))  {
          if (((posPoint->length[hh][trump]>1) ||
            (posPoint->length[partner[hh]][trump]>1))&&
            ((posPoint->tricksMAX+2)>=target)&&(depth>0)
	    &&(depth!=localVar[thrId].iniDepth)) {
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
      if ((localVar[thrId].nodeTypeStore[hh]==MAXNODE)&&
        (posPoint->length[hh][trump]>1)) {
	if (posPoint->winner[trump].hand==rho[hh]) {
          if (((posPoint->tricksMAX+1)>=target)&&(depth>0)
	     &&(depth!=localVar[thrId].iniDepth)) {
            for (ss=0; ss<=3; ss++)
              posPoint->winRanks[depth][ss]=0;
	    posPoint->winRanks[depth][trump]=
              bitMapRank[posPoint->secondBest[trump].rank] ;
            return TRUE;
	  }
	}
	else {
	  aggr=0;
	  for (k=0; k<=3; k++)
	    aggr|=posPoint->rankInSuit[k][trump];
	  h=localVar[thrId].rel[aggr].absRank[3][trump].hand;
	  if (h!=-1) {
	    if ((localVar[thrId].nodeTypeStore[h]==MAXNODE)&&
		((posPoint->tricksMAX+1)>=target)&&(depth>0)
		&&(depth!=localVar[thrId].iniDepth)) {
              for (ss=0; ss<=3; ss++)
                posPoint->winRanks[depth][ss]=0;
	      posPoint->winRanks[depth][trump]=
		bitMapRank[localVar[thrId].rel[aggr].absRank[3][trump].rank];
              return TRUE;
	    }
	  }
	}
      }
    }
  }
  return FALSE;
}


int MoveGen(struct pos * posPoint, int depth, int trump, struct movePlyType *mply, int thrId) {
  int suit, k, m, r, s, t, q, first, state;
  unsigned short ris;

  int WeightAllocTrump(struct pos * posPoint, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, int trump, int thrId);
  int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, int thrId);

  for (k=0; k<4; k++)
    localVar[thrId].lowestWin[depth][k]=0;

  m=0;
  r=posPoint->handRelFirst;
  assert((r>=0)&&(r<=3));
  first=posPoint->first[depth];
  q=handId(first, r);

  if (r!=0) {
    s=localVar[thrId].movePly[depth+r].current;             /* Current move of first hand */
    t=localVar[thrId].movePly[depth+r].move[s].suit;        /* Suit played by first hand */
    ris=posPoint->rankInSuit[q][t];

    if (ris!=0) {
    /* Not first hand and not void in suit */
      k=14;   state=MOVESVALID;
      while (k>=2) {
        if ((ris & bitMapRank[k])&&(state==MOVESVALID)) {
           /* Only first move in sequence is generated */
	  mply->move[m].suit=t;
	  mply->move[m].rank=k;
	  mply->move[m].sequence=0;
          m++;
          state=MOVESLOCKED;
        }
        else if (state==MOVESLOCKED)
          if ((posPoint->removedRanks[t] & bitMapRank[k])==0) {
            if ((ris & bitMapRank[k])==0)
            /* If the card still exists and it is not in own hand */
              state=MOVESVALID;
            else
            /* If the card still exists and it is in own hand */
	      mply->move[m-1].sequence|=bitMapRank[k];
          }
        k--;
      }
      if (m!=1) {
        if ((trump!=4)&&(posPoint->winner[trump].rank!=0)) {
          for (k=0; k<=m-1; k++)
	    mply->move[k].weight=WeightAllocTrump(posPoint,
              &(mply->move[k]), depth, ris, trump, thrId);
        }
        else {
	  for (k=0; k<=m-1; k++)
	    mply->move[k].weight=WeightAllocNT(posPoint,
              &(mply->move[k]), depth, ris, thrId);
        }
      }

      mply->last=m-1;
      if (m!=1)
        MergeSort(m, mply->move);
      if (depth!=localVar[thrId].iniDepth)
        return m;
      else {
        m=AdjustMoveList(thrId);
        return m;
      }
    }
  }

  /* First hand or void in suit */
  for (suit=0; suit<=3; suit++)  {
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
      else if (state==MOVESLOCKED)
        if ((posPoint->removedRanks[suit] & bitMapRank[k])==0) {
          if ((posPoint->rankInSuit[q][suit] & bitMapRank[k])==0)
            /* If the card still exists and it is not in own hand */
            state=MOVESVALID;
          else
            /* If the card still exists and it is in own hand */
	    mply->move[m-1].sequence|=bitMapRank[k];
      }
      k--;
    }
  }

  if ((trump!=4)&&(posPoint->winner[trump].rank!=0)) {
    for (k=0; k<=m-1; k++)
      mply->move[k].weight=WeightAllocTrump(posPoint,
          &(mply->move[k]), depth, 0/*ris*/, trump, thrId);
  }
  else {
    for (k=0; k<=m-1; k++)
      mply->move[k].weight=WeightAllocNT(posPoint,
          &(mply->move[k]), depth, 0/*ris*/, thrId);
  }

  mply->last=m-1;
  if (m!=1)
    MergeSort(m, mply->move);

  if (depth!=localVar[thrId].iniDepth)
    return m;
  else {
    m=AdjustMoveList(thrId);
    return m;
  }
}


int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
  unsigned short notVoidInSuit, int thrId) {
  int weight=0, k, l, kk, ll, suit, suitAdd=0, leadSuit;
  int suitWeightDelta, first, q;
  int rRank, thirdBestHand;
  int winMove=FALSE;	/* If winMove is true, current move can win the current trick. */
  unsigned short suitCount, suitCountLH, suitCountRH, aggr;
  int countLH, countRH;

  first=posPoint->first[depth];
  q=handId(first, posPoint->handRelFirst);
  suit=mp->suit;
  aggr=0;
  for (k=0; k<=3; k++)
    aggr|=posPoint->rankInSuit[k][suit];
  rRank=localVar[thrId].rel[aggr].relRank[mp->rank][suit];

  switch (posPoint->handRelFirst) {
    case 0:
      thirdBestHand=localVar[thrId].rel[aggr].absRank[3][suit].hand;
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
	  
      if (posPoint->winner[suit].rank==mp->rank) 
        winMove=TRUE;			   
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
	    suitWeightDelta-=7;
        }
        /* Encourage playing suit if LHO has second highest rank. */
        else if (posPoint->secondBest[suit].hand==lho[q]) 
          suitWeightDelta+=14;
	  
        /* Higher weight if also second best rank is present on current side to play, or
        if second best is a singleton at LHO or RHO. */

        if (((posPoint->secondBest[suit].hand!=lho[first])
           ||(suitCountLH==1))&&
           ((posPoint->secondBest[suit].hand!=rho[first])
           ||(suitCountRH==1)))
          weight=suitWeightDelta+41/*48*/+rRank;

        /* Encourage playing second highest rank if hand also has
        third highest rank. */

        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank))			
          weight=suitWeightDelta+39;
        else
          weight=suitWeightDelta+20+rRank;

        /* Encourage playing cards that previously caused search cutoff
        or was stored as the best move in a transposition table entry match. */

        if ((localVar[thrId].bestMove[depth].suit==suit)&&
          (localVar[thrId].bestMove[depth].rank==mp->rank)) 
          weight+=123/*122*//*112*//*73*/;
        else if ((localVar[thrId].bestMoveTT[depth].suit==suit)&&
          (localVar[thrId].bestMoveTT[depth].rank==mp->rank)) 
          weight+=24/*20*//*17*//*14*/;
      }
      else {
	/* Discourage suit if RHO has winning or second best card.
	   Exception: RHO has singleton. */

        if ((posPoint->winner[suit].hand==rho[q])||
          (posPoint->secondBest[suit].hand==rho[q])) {
	  if (suitCountRH!=1)
	    suitWeightDelta-=7;	
        }


	/* Try suit if LHO has winning card and partner second best. 
	     Exception: partner has singleton. */ 

        else if ((posPoint->winner[suit].hand==lho[q])&&
	  (posPoint->secondBest[suit].hand==partner[q])) {

	/* This case was suggested by Joël Bradmetz. */

	  if (posPoint->length[partner[q]][suit]!=1)
	    suitWeightDelta+=31/*34*//*37*//*32*//*20*/;
        }
     
	/* Encourage playing the suit if the hand together with partner have both the 2nd highest
	and the 3rd highest cards such that the side of the hand has the highest card in the
	next round playing this suit. */

	if ((posPoint->secondBest[suit].hand==partner[first])&&(partner[first]==thirdBestHand))
	  suitWeightDelta+=22/*10*/;
	else if(((posPoint->secondBest[suit].hand==first)&&(partner[first]==thirdBestHand)&&
	  (posPoint->length[partner[first]][suit]>1))||((posPoint->secondBest[suit].hand==partner[first])&&
	  (first==thirdBestHand)&&(posPoint->length[partner[first]][suit]>1)))
	  suitWeightDelta+=24/*10*/;	

	/* Higher weight if LHO or RHO has the highest (winning) card as a singleton. */

	if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+25/*23*//*22*/+rRank;
        else if (posPoint->winner[suit].hand==first) {
          weight=suitWeightDelta-20/*24*//*27*//*12*//*10*/+rRank;
        }

	/* Encourage playing second highest rank if hand also has
	third highest rank. */

        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank)) 
	  weight=suitWeightDelta+44/*42*/;
	else if (mp->sequence)
          weight=suitWeightDelta+31/*32*/-rRank;
        else 
          weight=suitWeightDelta+12+rRank; 
	
	/* Encourage playing cards that previously caused search cutoff
	or was stored as the best move in a transposition table entry match. */

	if ((localVar[thrId].bestMove[depth].suit==suit)&&
            (localVar[thrId].bestMove[depth].rank==mp->rank)) 
          weight+=47/*45*//*39*//*38*/;
	else if ((localVar[thrId].bestMoveTT[depth].suit==suit)&&
            (localVar[thrId].bestMoveTT[depth].rank==mp->rank)) 
          weight+=13/*15*//*16*//*19*//*14*/;
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
	      suitAdd-=3;
	  }
	  /* Discourage suit discard of highest card. */

	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd-=3;

	  /*Encourage discard of low cards in long suits. */
	    weight=/*60*/-(mp->rank)+suitAdd;		
        }
	else {	
	  weight=80+rRank;
        } 
      }
      else {
        if (!notVoidInSuit) {
	  suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/33; 
 
	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=7;	
		  
          /* Discourage suit discard of highest card. */
	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd-=10;

	  /*Encourage discard of low cards in long suits. */
          weight=-(mp->rank)+suitAdd; 
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
	    weight=-15+rRank;		
	  }	
        }
      }

      break;

    case 2:
            
      leadSuit=posPoint->move[depth+2].suit;
      if (WinningMoveNT(mp, &(posPoint->move[depth+1]),thrId)) {
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
          else if (WinningMoveNT(mp, &(posPoint->move[depth+1]), thrId)) {
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
      else if (WinningMoveNT(mp, &(posPoint->move[depth+1]),thrId))
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
  unsigned short notVoidInSuit, int trump, int thrId) {
  int weight=0, k, l, kk, ll, suit, suitAdd=0, leadSuit;
  int suitWeightDelta, first, q, rRank, thirdBestHand;
  int suitBonus=0;
  int winMove=FALSE;	/* If winMove is true, current move can win the current trick. */
  unsigned short suitCount, suitCountLH, suitCountRH, aggr;
  int countLH, countRH;

  first=posPoint->first[depth];
  q=handId(first, posPoint->handRelFirst);
  suit=mp->suit;
  aggr=0;
  for (k=0; k<=3; k++)
    aggr|=posPoint->rankInSuit[k][suit];
  rRank=localVar[thrId].rel[aggr].relRank[mp->rank][suit];

  switch (posPoint->handRelFirst) {
    case 0:
      thirdBestHand=localVar[thrId].rel[aggr].absRank[3][suit].hand;
      suitCount=posPoint->length[q][suit];
      suitCountLH=posPoint->length[lho[q]][suit];
      suitCountRH=posPoint->length[rho[q]][suit];

      /* Discourage suit if LHO or RHO can ruff. */

      if ((suit!=trump) &&
        (((posPoint->rankInSuit[lho[q]][suit]==0) &&
          (posPoint->rankInSuit[lho[q]][trump]!=0)) ||
          ((posPoint->rankInSuit[rho[q]][suit]==0) &&
          (posPoint->rankInSuit[rho[q]][trump]!=0))))
        suitBonus=-17/*20*//*-10*/;
	    
      /* Encourage suit if partner can ruff. */

      if ((suit!=trump)&&(posPoint->length[partner[q]][suit]==0)&&
	     (posPoint->length[partner[q]][trump]>0)&&(suitCountRH>0))
	suitBonus+=26/*28*/;

      /* Discourage suit if RHO has high card. */

      if ((posPoint->winner[suit].hand==rho[q])||
          (posPoint->secondBest[suit].hand==rho[q])) {
	if (suitCountRH!=1)
	  suitBonus-=11/*12*//*13*//*18*/;
      }

      /* Try suit if LHO has winning card and partner second best. 
      Exception: partner has singleton. */ 

      else if ((posPoint->winner[suit].hand==lho[q])&&
	(posPoint->secondBest[suit].hand==partner[q])) {

	/* This case was suggested by Joël Bradmetz. */

	if (posPoint->length[partner[q]][suit]!=1) 
	  suitBonus+=30/*28*//*22*/;
      }
 
      /* Encourage play of suit where partner wins and
      returns the suit for a ruff. */
      if ((suit!=trump)&&(suitCount==1)&&
	(posPoint->length[q][trump]>0)&&
	(posPoint->length[partner[q]][suit]>1)&&
	(posPoint->winner[suit].hand==partner[q]))
	suitBonus+=23/*24*//*19*//*16*/;

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
	((countLH+countRH)<<5)/(12/*15*/);

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
          weight=suitWeightDelta+39+rRank;

	/* Lead hand has the highest card. */

        else if (posPoint->winner[suit].hand==first) {

	/* Also, partner has second highest card. */

          if (posPoint->secondBest[suit].hand==partner[first])
            weight=suitWeightDelta+46+rRank;
	  else if (posPoint->winner[suit].rank==mp->rank)

	    /* If the current card to play is the highest card. */

            weight=suitWeightDelta+31;
          else
            weight=suitWeightDelta-2+rRank;
        }
        else if (posPoint->winner[suit].hand==partner[first]) {
          /* If partner has highest card */
          if (posPoint->secondBest[suit].hand==first)
            weight=suitWeightDelta+35/*35*//*46*//*50*/+rRank;
          else 
            weight=suitWeightDelta+24/*35*/+rRank;  
        } 
	/* Encourage playing second highest rank if hand also has
	third highest rank. */

        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank))			
          weight=suitWeightDelta+41;
	else if (mp->sequence)
	  weight=suitWeightDelta+17+rRank;
        else
          weight=suitWeightDelta+11+rRank;

	/* Encourage playing cards that previously caused search cutoff
	or was stored as the best move in a transposition table entry match. */

	if ((localVar[thrId].bestMove[depth].suit==suit)&&
            (localVar[thrId].bestMove[depth].rank==mp->rank)) 
          weight+=53/*50*//*52*/;
	else if ((localVar[thrId].bestMoveTT[depth].suit==suit)&&
            (localVar[thrId].bestMoveTT[depth].rank==mp->rank)) 
          weight+=14/*15*//*12*//*11*/;
      }
      else {

	/* Encourage playing the suit if the hand together with partner have both the 2nd highest
	and the 3rd highest cards such that the side of the hand has the highest card in the
	next round playing this suit. */

	if ((posPoint->secondBest[suit].hand==partner[first])&&(partner[first]==thirdBestHand))
	   suitWeightDelta+=20/*22*/;
	else if(((posPoint->secondBest[suit].hand==first)&&(partner[first]==thirdBestHand)&&
	  (posPoint->length[partner[first]][suit]>1))||
	  ((posPoint->secondBest[suit].hand==partner[first])&&
	  (first==thirdBestHand)&&(posPoint->length[partner[first]][suit]>1)))
	   suitWeightDelta+=20/*24*/;
	
	/* Higher weight if LHO or RHO has the highest (winning) card as a singleton. */

        if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+rRank-2;
        else if (posPoint->winner[suit].hand==first) {
          if (posPoint->secondBest[suit].hand==partner[first])

	  /* Opponents win by ruffing */

            weight=suitWeightDelta+33+rRank;
          else if (posPoint->winner[suit].rank==mp->rank) 

	  /* Opponents win by ruffing */

            weight=suitWeightDelta+36;
          else
            weight=suitWeightDelta-17+rRank;
        }
        else if (posPoint->winner[suit].hand==partner[first]) {

          /* Opponents win by ruffing */

          weight=suitWeightDelta+33+rRank;
        } 
	/* Encourage playing second highest rank if hand also has
	third highest rank. */

        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank)) 
          weight=suitWeightDelta+31;
        else 
	  weight=suitWeightDelta+13-(mp->rank);
	
	/* Encourage playing cards that previously caused search cutoff
	or was stored as the best move in a transposition table entry match. */

	if ((localVar[thrId].bestMove[depth].suit==suit)&&
            (localVar[thrId].bestMove[depth].rank==mp->rank)) 
          weight+=17;
	else if ((localVar[thrId].bestMoveTT[depth].suit==suit)&&
            (localVar[thrId].bestMoveTT[depth].rank==mp->rank)) 
          weight+=(3/*4*//*10*//*9*/);
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
          suitAdd=(suitCount<<6)/(43/*36*/);

	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 

          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=2;

          if (suit==trump)  
	        weight=25/*23*/-(mp->rank)+suitAdd;
          else
            weight=60-(mp->rank)+suitAdd;  /* Better discard than ruff since rho
								wins anyway */		
        } 
        else if (k > bitMapRank[mp->rank])
	  weight=40/*41*/+rRank;

          /* If lowest card for partner to leading hand 
	    is higher than lho played card, playing as low as 
	    possible will give the cheapest win */

        else if ((ll > bitMapRank[posPoint->move[depth+1].rank])&&
          (posPoint->rankInSuit[first][leadSuit] > ll))
	      weight=37/*40*/+rRank;

	  /* If rho has a card in the leading suit that
             is higher than the trick leading card but lower
             than the highest rank of the leading hand, then
             lho playing the lowest card will be the cheapest
             win */

	else if (mp->rank > posPoint->move[depth+1].rank) {
          if (bitMapRank[mp->rank] < ll) 
            weight=75-(mp->rank);  /* If played card is lower than any of the cards of
					rho, it will be the cheapest win */		
          else if (bitMapRank[mp->rank] > kk)
            weight=70-(mp->rank);  /* If played card is higher than any cards at partner
				    of the leading hand, rho can play low, under the
                                    condition that he has a lower card than lho played */    
          else {
            if (mp->sequence)
              weight=62/*63*//*60*/-(mp->rank);   
            else
              weight=48/*45*/-(mp->rank);  
          }
        } 
        else if (posPoint->length[rho[first]][leadSuit]>0) {
          if (mp->sequence)
            weight=47/*50*/-(mp->rank);  /* Playing a card in a sequence may promote a winner */
												/* Insensistive */
          else
            weight=45-(mp->rank);	
        }
        else
          weight=43/*45*/-(mp->rank);
      }
      else {
        if (!notVoidInSuit) { 
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(33/*36*/);
	
	  /* Discourage suit discard if 2nd highest card becomes singleton. */
 
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(4/*2*/);
  
	  if (suit==trump) {
            weight=16/*15*/-(mp->rank)+suitAdd;  /* Ruffing is preferred, makes the trick
						costly for the opponents */
	  }
          else
            weight=-6-(mp->rank)+suitAdd;
        }
        else if ((k > bitMapRank[mp->rank])||
          (l > bitMapRank[mp->rank]))
	  weight=-7/*-9*/+rRank;

          /* If lowest rank for either partner to leading hand 
	  or rho is higher than played card for lho,
	  lho should play as low card as possible */
			
        else if (mp->rank > posPoint->move[depth+1].rank) {		  
          if (mp->sequence) 
            weight=19/*19*/-(mp->rank);	
          else 
            weight=10-(mp->rank);
        }          
        else
	  weight=-17+rRank;
      }
      break;

    case 2:
            
      leadSuit=posPoint->move[depth+2].suit;
      if (WinningMove(mp, &(posPoint->move[depth+1]),trump,thrId)) {
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
          suitAdd=(suitCount<<6)/(48/*36*/);

	  /* Discourage suit discard if 2nd highest card becomes singleton. */ 

          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(3/*2*/);
        
          if (posPoint->high[depth+1]==first) {
            if (suit==trump) 
              weight=30-(mp->rank)+suitAdd;  /* Ruffs partner's winner */  
	    else 
              weight=60-(mp->rank)+suitAdd;  
          } 
          else if (WinningMove(mp, &(posPoint->move[depth+1]),trump,thrId))

             /* Own hand on top by ruffing */

            weight=70-(mp->rank)+suitAdd;  
        }
        else 
          weight=60-(mp->rank);	
      }
      else {
        if (!notVoidInSuit) {
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/36;

	  /* Discourage suit discard if 2nd highest card becomes singleton. */
 
	  if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(4/*2*/);	
          
          if (WinningMove(mp, &(posPoint->move[depth+1]),trump,thrId))

             /* Own hand on top by ruffing */

            weight=40-(mp->rank)+suitAdd;  
          else if (suit==trump)

            /* Discard a trump but still losing */

	    weight=-/*33*/36+rRank+suitAdd;
          else
            weight=-(mp->rank)+suitAdd;
        }
        else {
	  k=posPoint->rankInSuit[rho[first]][suit];
	  if ((k & (-k)) > bitMapRank[mp->rank])

	    /* If least bit map rank of RHO to lead hand is higher than bit map rank
		of current card move. */

	    weight=-(mp->rank);

          else if (WinningMove(mp, &(posPoint->move[depth+1]),trump,thrId)) {

	    /* If current card move is highest so far. */

            if (mp->rank==posPoint->secondBest[leadSuit].rank)
              weight=25;		
            else if (mp->sequence)
              weight=21/*20*/-(mp->rank);
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
        suitAdd=(suitCount<<6)/(24/*36*/);
        if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
          suitAdd-=(2/*0*//*2*/);

        if ((posPoint->high[depth+1])==lho[first]) {

          /* If the current winning move is given by the partner */

          if (suit==trump)

            /* Ruffing partners winner? */

            weight=2/*17*/-(mp->rank)+suitAdd;
          else 
            weight=25-(mp->rank)+suitAdd;
        }
        else if (WinningMove(mp, &(posPoint->move[depth+1]),trump,thrId)) 

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
      else if (WinningMove(mp, &(posPoint->move[depth+1]),trump,thrId))

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
    for (int i = 1; i < n; i++)
    {
        struct moveType tmp = a[i];
        int j = i;
        for (; j && tmp.weight > a[j - 1].weight ; --j)
            a[j] = a[j - 1];
        a[j] = tmp;
    }
  }

  return;
}



int AdjustMoveList(int thrId) {
  int k, r, n, rank, suit;

  for (k=1; k<=13; k++) {
    suit=localVar[thrId].forbiddenMoves[k].suit;
    rank=localVar[thrId].forbiddenMoves[k].rank;
    for (r=0; r<=localVar[thrId].movePly[localVar[thrId].iniDepth].last; r++) {
      if ((suit==localVar[thrId].movePly[localVar[thrId].iniDepth].move[r].suit)&&
        (rank!=0)&&(rank==localVar[thrId].movePly[localVar[thrId].iniDepth].move[r].rank)) {
        /* For the forbidden move r: */
        for (n=r; n<=localVar[thrId].movePly[localVar[thrId].iniDepth].last; n++)
          localVar[thrId].movePly[localVar[thrId].iniDepth].move[n]=
		  localVar[thrId].movePly[localVar[thrId].iniDepth].move[n+1];
        localVar[thrId].movePly[localVar[thrId].iniDepth].last--;
      }
    }
  }
  return localVar[thrId].movePly[localVar[thrId].iniDepth].last+1;
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


inline int WinningMove(struct moveType * mvp1, struct moveType * mvp2, int trump, int thrId) {
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


inline int WinningMoveNT(struct moveType * mvp1, struct moveType * mvp2, int thrId) {
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
  * nodep, int target, int tricks, int * result, int *value, int thrId) {
    /* Check SOP if it matches the
    current position. If match, pointer to the SOP node is returned and
    result is set to TRUE, otherwise pointer to SOP node is returned
    and result set to FALSE. */

  /* 07-04-22 */
  if (localVar[thrId].nodeTypeStore[0]==MAXNODE) {
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

  nodep->bestMoveSuit=posPoint->bestMoveSuit;
    nodep->bestMoveRank=posPoint->bestMoveRank;

  return nodep;
}


struct nodeCardsType * FindSOP(struct pos * posPoint,
  struct winCardType * nodeP, int firstHand,
	int target, int tricks, int * valp, int thrId) {
  struct nodeCardsType * sopP;
  struct winCardType * np;
  int s, res;

  np=nodeP; s=0;
  while ((np!=NULL)&&(s<4)) {
    if ((np->winMask & posPoint->orderSet[s])==
       np->orderSet)  {
      /* Winning rank set fits position */
      if (s==3) {
	sopP=CheckSOP(posPoint, np->first, target, tricks, &res, valp, thrId);
	if (res) {
	  return sopP;
	}
	else {
	  if (np->next!=NULL) {
	    np=np->next;
	  }
	  else {
	    np=np->prevWin;
	    s--;
	    if (np==NULL)
	      return NULL;
	    while (np->next==NULL) {
	      np=np->prevWin;
	      s--;
	      if (np==NULL)  /* Previous node is header node? */
				return NULL;
	    }
	    np=np->next;
	  }
	}
      }
      else if (s<4) {
	np=np->nextWin;
	s++;
      }
    }
    else {
      if (np->next!=NULL) {
	np=np->next;
      }
      else {
	np=np->prevWin;
	s--;
	if (np==NULL)
	  return NULL;
	while (np->next==NULL) {
	  np=np->prevWin;
	  s--;
	  if (np==NULL)  /* Previous node is header node? */
	    return NULL;
	}
	np=np->next;
      }
    }
  }
  return NULL;
}


struct nodeCardsType * BuildPath(struct pos * posPoint,
  struct posSearchType *nodep, int * result, int thrId) {
  /* If result is TRUE, a new SOP has been created and BuildPath returns a
  pointer to it. If result is FALSE, an existing SOP is used and BuildPath
  returns a pointer to the SOP */

  int found, suit;
  struct winCardType * np, * p2, /* * sp2,*/ * nprev, * fnp, *pnp;
  struct winCardType temp;
  struct nodeCardsType * sopP=0, * p/*, * sp*/;

  np=nodep->posSearchPoint;
  nprev=NULL;
  suit=0;

  /* If winning node has a card that equals the next winning card deduced
  from the position, then there already exists a (partial) path */

  if (np==NULL) {   /* There is no winning list created yet */
   /* Create winning nodes */
    p2=&localVar[thrId].winCards[localVar[thrId].winSetSize];
    AddWinSet(thrId);
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
      p2=&localVar[thrId].winCards[localVar[thrId].winSetSize];
      AddWinSet(thrId);
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
    p=&localVar[thrId].nodeCards[localVar[thrId].nodeSetSize];
    AddNodeSet(thrId);
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
    p2=&localVar[thrId].winCards[localVar[thrId].winSetSize];
    AddWinSet(thrId);
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
      p2=&localVar[thrId].winCards[localVar[thrId].winSetSize];
      AddWinSet(thrId);
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
    p=&localVar[thrId].nodeCards[localVar[thrId].nodeSetSize];
    AddNodeSet(thrId);
    np->first=p;
    *result=TRUE;
    return p;
  }
}


struct posSearchType * SearchLenAndInsert(struct posSearchType
	* rootp, long long key, int insertNode, int *result, int thrId) {
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
    sp=&localVar[thrId].posSearch[localVar[thrId].lenSetSize];

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
	AddLenSet(thrId);
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
	AddLenSet(thrId);
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



void BuildSOP(struct pos * posPoint, int tricks, int firstHand, int target,
  int depth, int scoreFlag, int score, int thrId) {
  int ss, hh, res, wm;
  unsigned short int w;
  unsigned short int temp[4][4];
  unsigned short int aggr[4];
  struct nodeCardsType * cardsP;
  struct posSearchType * np;
  long long suitLengths;

#ifdef TTDEBUG
  int k, mcurrent, rr;
  mcurrent=localVar[thrId].movePly[depth].current;
#endif

  for (ss=0; ss<=3; ss++) {
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
	aggr[ss]=aggr[ss] | temp[hh][ss];
      posPoint->winMask[ss]=localVar[thrId].rel[aggr[ss]].winMask[ss];
      posPoint->winOrderSet[ss]=localVar[thrId].rel[aggr[ss]].aggrRanks[ss];
      wm=posPoint->winMask[ss];
      wm=wm & (-wm);
      posPoint->leastWin[ss]=InvWinMask(wm);
    }
  }

  /* 07-04-22 */
  if (scoreFlag) {
    if (localVar[thrId].nodeTypeStore[0]==MAXNODE) {
      posPoint->ubound=tricks+1;
      posPoint->lbound=target-posPoint->tricksMAX;
    }
    else {
      posPoint->ubound=tricks+1-target+posPoint->tricksMAX;
      posPoint->lbound=0;
    }
  }
  else {
    if (localVar[thrId].nodeTypeStore[0]==MAXNODE) {
      posPoint->ubound=target-posPoint->tricksMAX-1;
      posPoint->lbound=0;
    }
    else {
      posPoint->ubound=tricks+1;
      posPoint->lbound=tricks+1-target+posPoint->tricksMAX+1;
    }
  }

  suitLengths=0;
  for (ss=0; ss<=2; ss++)
    for (hh=0; hh<=3; hh++) {
      suitLengths<<=4;
      suitLengths|=posPoint->length[hh][ss];
    }

  np=SearchLenAndInsert(localVar[thrId].rootnp[tricks][firstHand],
	 suitLengths, TRUE, &res, thrId);

  cardsP=BuildPath(posPoint, np, &res, thrId);
  if (res) {
    cardsP->ubound=posPoint->ubound;
    cardsP->lbound=posPoint->lbound;
    if (((localVar[thrId].nodeTypeStore[firstHand]==MAXNODE)&&(scoreFlag))||
	((localVar[thrId].nodeTypeStore[firstHand]==MINNODE)&&(!scoreFlag))) {
      cardsP->bestMoveSuit=localVar[thrId].bestMove[depth].suit;
      cardsP->bestMoveRank=localVar[thrId].bestMove[depth].rank;
    }
    else {
      cardsP->bestMoveSuit=0;
      cardsP->bestMoveRank=0;
    }
    posPoint->bestMoveSuit=localVar[thrId].bestMove[depth].suit;
    posPoint->bestMoveRank=localVar[thrId].bestMove[depth].rank;
    for (ss=0; ss<=3; ss++)
      cardsP->leastWin[ss]=posPoint->leastWin[ss];
  }

  #ifdef STAT
    c9[depth]++;
  #endif

  #ifdef TTDEBUG
  if ((res) && (ttCollect) && (!suppressTTlog)) {
    fprintf(localVar[thrId].fp7, "cardsP=%d\n", (int)cardsP);
    fprintf(localVar[thrId].fp7, "nodeSetSize=%d\n", localVar[thrId].nodeSetSize);
    fprintf(localVar[thrId].fp7, "ubound=%d\n", cardsP->ubound);
    fprintf(localVar[thrId].fp7, "lbound=%d\n", cardsP->lbound);
    fprintf(localVar[thrId].fp7, "target=%d\n", target);
    fprintf(localVar[thrId].fp7, "first=%c nextFirst=%c\n",
      cardHand[posPoint->first[depth]], cardHand[posPoint->first[depth-1]]);
    fprintf(localVar[thrId].fp7, "bestMove:  suit=%c rank=%c\n", cardSuit[localVar[thrId].bestMove[depth].suit],
      cardRank[localVar[thrId].bestMove[depth].rank]);
    fprintf(localVar[thrId].fp7, "\n");
    fprintf(localVar[thrId].fp7, "Last trick:\n");
    fprintf(localVar[thrId].fp7, "1st hand=%c\n", cardHand[posPoint->first[depth+3]]);
    for (k=3; k>=0; k--) {
      mcurrent=localVar[thrId].movePly[depth+k+1].current;
      fprintf(localVar[thrId].fp7, "suit=%c  rank=%c\n",
        cardSuit[localVar[thrId].movePly[depth+k+1].move[mcurrent].suit],
        cardRank[localVar[thrId].movePly[depth+k+1].move[mcurrent].rank]);
    }
    fprintf(localVar[thrId].fp7, "\n");
    for (hh=0; hh<=3; hh++) {
      fprintf(localVar[thrId].fp7, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
	fprintf(localVar[thrId].fp7, "suit=%c", cardSuit[ss]);
	for (rr=14; rr>=2; rr--)
	  if (posPoint->rankInSuit[hh][ss] & bitMapRank[rr])
	    fprintf(localVar[thrId].fp7, " %c", cardRank[rr]);
	fprintf(localVar[thrId].fp7, "\n");
      }
      fprintf(localVar[thrId].fp7, "\n");
    }
    fprintf(localVar[thrId].fp7, "\n");
  }
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


int NextMove(struct pos *posPoint, int depth, struct movePlyType *mply, int thrId) {
  /* Returns TRUE if at least one move remains to be
  searched, otherwise FALSE is returned. */
  int mcurrent;
  unsigned short int lw;
  unsigned char suit;
  struct moveType currMove;

  mcurrent=mply->current;
  currMove=mply->move[mcurrent];

  if (localVar[thrId].lowestWin[depth][currMove.suit]==0) {
    /* A small card has not yet been identified for this suit. */
    lw=posPoint->winRanks[depth][currMove.suit];
    if (lw!=0)
      lw=lw & (-lw);  /* LSB */
    else
      lw=bitMapRank[15];
    if (bitMapRank[currMove.rank]<lw) {
       /* The current move has a small card. */
      localVar[thrId].lowestWin[depth][currMove.suit]=lw;
      while (mply->current <= (mply->last-1)) {
	mply->current++;
	mcurrent=mply->current;
	if (bitMapRank[mply->move[mcurrent].rank] >=
	  localVar[thrId].lowestWin[depth][mply->move[mcurrent].suit])
	  return TRUE;
      }
      return FALSE;
    }
    else {
      while (mply->current <= (mply->last-1)) {
	mply->current++;
	mcurrent=mply->current;
	suit=mply->move[mcurrent].suit;
	if ((currMove.suit==suit) || (bitMapRank[mply->move[mcurrent].rank] >=
		localVar[thrId].lowestWin[depth][suit]))
	  return TRUE;
      }
      return FALSE;
    }
  }
  else {
    while (mply->current<=(mply->last-1)) {
      mply->current++;
      mcurrent=mply->current;
      if (bitMapRank[mply->move[mcurrent].rank] >=
	    localVar[thrId].lowestWin[depth][mply->move[mcurrent].suit])
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
    return -1;
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
      fprintf(fp, "\t\%c ", cardSuit[s]);
    else
      fprintf(fp, "\t\t\%c ", cardSuit[s]);
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



void Wipe(int thrId) {
  int k;

  for (k=1; k<=localVar[thrId].wcount; k++) {
    if (localVar[thrId].pw[k])
      free(localVar[thrId].pw[k]);
    localVar[thrId].pw[k]=NULL;
  }
  for (k=1; k<=localVar[thrId].ncount; k++) {
    if (localVar[thrId].pn[k])
      free(localVar[thrId].pn[k]);
    localVar[thrId].pn[k]=NULL;
  }
  for (k=1; k<=localVar[thrId].lcount; k++) {
    if (localVar[thrId].pl[k])
      free(localVar[thrId].pl[k]);
    localVar[thrId].pl[k]=NULL;
  }

  localVar[thrId].allocmem=localVar[thrId].summem;

  return;
}


void AddWinSet(int thrId) {
  if (localVar[thrId].clearTTflag) {
    localVar[thrId].windex++;
    localVar[thrId].winSetSize=localVar[thrId].windex;
    /*localVar[thrId].fp2=fopen("dyn.txt", "a");
    fprintf(localVar[thrId].fp2, "windex=%d\n", windex);
    fclose(localVar[thrId].fp2);*/
    localVar[thrId].winCards=&localVar[thrId].temp_win[localVar[thrId].windex];
  }
  else if (localVar[thrId].winSetSize>=localVar[thrId].winSetSizeLimit) {
    /* The memory chunk for the winCards structure will be exceeded. */
    if ((localVar[thrId].allocmem+localVar[thrId].wmem)>localVar[thrId].maxmem) {
      /* Already allocated memory plus needed allocation overshot maxmem */
      localVar[thrId].windex++;
      localVar[thrId].winSetSize=localVar[thrId].windex;
      /*localVar[thrId].fp2=fopen("dyn.txt", "a");
      fprintf(localVar[thrId].fp2, "windex=%d\n", windex);
      fclose(localVar[thrId].fp2);*/
      localVar[thrId].clearTTflag=TRUE;
      localVar[thrId].winCards=&localVar[thrId].temp_win[localVar[thrId].windex];
    }
    else {
      localVar[thrId].wcount++; localVar[thrId].winSetSizeLimit=WSIZE;
      localVar[thrId].pw[localVar[thrId].wcount] =
		  (struct winCardType *)calloc(localVar[thrId].winSetSizeLimit+1, sizeof(struct winCardType));
      if (localVar[thrId].pw[localVar[thrId].wcount]==NULL) {
        localVar[thrId].clearTTflag=TRUE;
        localVar[thrId].windex++;
	localVar[thrId].winSetSize=localVar[thrId].windex;
	localVar[thrId].winCards=&localVar[thrId].temp_win[localVar[thrId].windex];
      }
      else {
	localVar[thrId].allocmem+=(localVar[thrId].winSetSizeLimit+1)*sizeof(struct winCardType);
	localVar[thrId].winSetSize=0;
	localVar[thrId].winCards=localVar[thrId].pw[localVar[thrId].wcount];
      }
    }
  }
  else
    localVar[thrId].winSetSize++;
  return;
}

void AddNodeSet(int thrId) {
  if (localVar[thrId].nodeSetSize>=localVar[thrId].nodeSetSizeLimit) {
    /* The memory chunk for the nodeCards structure will be exceeded. */
    if ((localVar[thrId].allocmem+localVar[thrId].nmem)>localVar[thrId].maxmem) {
      /* Already allocated memory plus needed allocation overshot maxmem */
      localVar[thrId].clearTTflag=TRUE;
    }
    else {
      localVar[thrId].ncount++; localVar[thrId].nodeSetSizeLimit=NSIZE;
      localVar[thrId].pn[localVar[thrId].ncount] = (struct nodeCardsType *)calloc(localVar[thrId].nodeSetSizeLimit+1, sizeof(struct nodeCardsType));
      if (localVar[thrId].pn[localVar[thrId].ncount]==NULL) {
        localVar[thrId].clearTTflag=TRUE;
      }
      else {
	localVar[thrId].allocmem+=(localVar[thrId].nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
	localVar[thrId].nodeSetSize=0;
	localVar[thrId].nodeCards=localVar[thrId].pn[localVar[thrId].ncount];
      }
    }
  }
  else
    localVar[thrId].nodeSetSize++;
  return;
}

void AddLenSet(int thrId) {
  if (localVar[thrId].lenSetSize>=localVar[thrId].lenSetSizeLimit) {
      /* The memory chunk for the posSearchType structure will be exceeded. */
    if ((localVar[thrId].allocmem+localVar[thrId].lmem)>localVar[thrId].maxmem) {
       /* Already allocated memory plus needed allocation overshot maxmem */
      localVar[thrId].clearTTflag=TRUE;
    }
    else {
      localVar[thrId].lcount++; localVar[thrId].lenSetSizeLimit=LSIZE;
      localVar[thrId].pl[localVar[thrId].lcount] = (struct posSearchType *)calloc(localVar[thrId].lenSetSizeLimit+1, sizeof(struct posSearchType));
      if (localVar[thrId].pl[localVar[thrId].lcount]==NULL) {
        localVar[thrId].clearTTflag=TRUE;
      }
      else {
	localVar[thrId].allocmem+=(localVar[thrId].lenSetSizeLimit+1)*sizeof(struct posSearchType);
	localVar[thrId].lenSetSize=0;
	localVar[thrId].posSearch=localVar[thrId].pl[localVar[thrId].lcount];
      }
    }
  }
  else
    localVar[thrId].lenSetSize++;
  return;
}



#ifdef TTDEBUG

void ReceiveTTstore(struct pos *posPoint, struct nodeCardsType * cardsP,
  int target, int depth, int thrId) {
  int tricksLeft, hh, ss, rr;
/* Stores current position information and TT position value in table
  ttStore with current entry lastTTStore. Also stores corresponding
  information in log rectt.txt. */
  tricksLeft=0;
  for (hh=0; hh<=3; hh++)
    for (ss=0; ss<=3; ss++)
      tricksLeft+=posPoint->length[hh][ss];
  tricksLeft=tricksLeft/4;
  ttStore[lastTTstore].tricksLeft=tricksLeft;
  ttStore[lastTTstore].cardsP=cardsP;
  ttStore[lastTTstore].first=posPoint->first[depth];
  if ((localVar[thrId].handToPlay==posPoint->first[depth])||
    (localVar[thrId].handToPlay==partner[posPoint->first[depth]])) {
    ttStore[lastTTstore].target=target-posPoint->tricksMAX;
    ttStore[lastTTstore].ubound=cardsP->ubound;
    ttStore[lastTTstore].lbound=cardsP->lbound;
  }
  else {
    ttStore[lastTTstore].target=tricksLeft-
      target+posPoint->tricksMAX+1;
  }
  for (hh=0; hh<=3; hh++)
    for (ss=0; ss<=3; ss++)
      ttStore[lastTTstore].suit[hh][ss]=
        posPoint->rankInSuit[hh][ss];
  localVar[thrId].fp11=fopen("rectt.txt", "a");
  if (lastTTstore<SEARCHSIZE) {
    fprintf(localVar[thrId].fp11, "lastTTstore=%d\n", lastTTstore);
    fprintf(localVar[thrId].fp11, "tricksMAX=%d\n", posPoint->tricksMAX);
    fprintf(localVar[thrId].fp11, "leftTricks=%d\n",
      ttStore[lastTTstore].tricksLeft);
    fprintf(localVar[thrId].fp11, "cardsP=%d\n",
      ttStore[lastTTstore].cardsP);
    fprintf(localVar[thrId].fp11, "ubound=%d\n",
      ttStore[lastTTstore].ubound);
    fprintf(localVar[thrId].fp11, "lbound=%d\n",
      ttStore[lastTTstore].lbound);
    fprintf(localVar[thrId].fp11, "first=%c\n",
      cardHand[ttStore[lastTTstore].first]);
    fprintf(localVar[thrId].fp11, "target=%d\n",
      ttStore[lastTTstore].target);
    fprintf(localVar[thrId].fp11, "\n");
    for (hh=0; hh<=3; hh++) {
      fprintf(localVar[thrId].fp11, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
        fprintf(localVar[thrId].fp11, "suit=%c", cardSuit[ss]);
        for (rr=14; rr>=2; rr--)
          if (ttStore[lastTTstore].suit[hh][ss]
            & bitMapRank[rr])
            fprintf(localVar[thrId].fp11, " %c", cardRank[rr]);
         fprintf(localVar[thrId].fp11, "\n");
      }
      fprintf(localVar[thrId].fp11, "\n");
    }
  }
  fclose(localVar[thrId].fp11);
  lastTTstore++;
}
#endif

#if defined(_WIN32)
HANDLE solveAllEvents[MAXNOOFTHREADS];
struct paramType param;
LONG volatile threadIndex;
LONG volatile current;

const long chunk = 4;

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
	param.error=0;
      }
      else {
	param.error=res;
      }
    }
  }

  if (SetEvent(solveAllEvents[thid])==0) {
    int errCode=GetLastError();
    return 0;
  }

  return 1;

}

int SolveAllBoards4(struct boards *bop, struct solvedBoards *solvedp) {
  int k, errCode;
  DWORD res;
  DWORD solveAllWaitResult;

  current=0;

  threadIndex=-1;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return -101;

  for (k=0; k<noOfCores; k++) {
    solveAllEvents[k]=CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k]==0) {
      errCode=GetLastError();
      return -102;
    }
  }

  param.bop=bop; param.solvedp=solvedp; param.noOfBoards=bop->noOfBoards;

  for (k=0; k<MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards=0;

  for (k=0; k<noOfCores; k++) {
    res=QueueUserWorkItem(SolveChunkDDtable, NULL, WT_EXECUTELONGFUNCTION);
    if (res!=1) {
      errCode=GetLastError();
      return res;
    }
  }

  solveAllWaitResult = WaitForMultipleObjects(noOfCores,
	  solveAllEvents, TRUE, INFINITE);
  if (solveAllWaitResult!=WAIT_OBJECT_0) {
    errCode=GetLastError();
    return -103;
  }

  for (k=0; k<noOfCores; k++) {
    CloseHandle(solveAllEvents[k]);
  }

  /* Calculate number of solved boards. */

  solvedp->noOfBoards=0;
  for (k=0; k<MAXNOOFBOARDS; k++) {
    if (solvedp->solvedBoard[k].cards!=0)
      solvedp->noOfBoards++;
  }

  if (param.error==0)
    return 1;
  else
    return param.error;
}

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
      param.error=0;
    }
    else {
      param.error=res;
    }
  }

  if (SetEvent(solveAllEvents[thid])==0) {
    int errCode=GetLastError();
    return 0;
  }

  return 1;

}

int SolveAllBoards1(struct boards *bop, struct solvedBoards *solvedp) {
  int k, errCode;
  DWORD res;
  DWORD solveAllWaitResult;

  current=0;

  threadIndex=-1;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return -201;

  for (k=0; k<noOfCores; k++) {
    solveAllEvents[k]=CreateEvent(NULL, FALSE, FALSE, 0);
    if (solveAllEvents[k]==0) {
      errCode=GetLastError();
      return -202;
    }
  }

  param.bop=bop; param.solvedp=solvedp; param.noOfBoards=bop->noOfBoards;

  for (k=0; k<MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards=0;

  for (k=0; k<noOfCores; k++) {
    res=QueueUserWorkItem(SolveChunk, NULL, WT_EXECUTELONGFUNCTION);
    if (res!=1) {
      errCode=GetLastError();
      return res;
    }
  }

  solveAllWaitResult = WaitForMultipleObjects(noOfCores,
	  solveAllEvents, TRUE, INFINITE);
  if (solveAllWaitResult!=WAIT_OBJECT_0) {
    errCode=GetLastError();
    return -203;
  }

  for (k=0; k<noOfCores; k++) {
    CloseHandle(solveAllEvents[k]);
  }

  /* Calculate number of solved boards. */

  solvedp->noOfBoards=0;
  for (k=0; k<MAXNOOFBOARDS; k++) {
    if (solvedp->solvedBoard[k].cards!=0)
      solvedp->noOfBoards++;
  }

  if (param.error==0)
    return 1;
  else
    return param.error;
}
#else
int SolveAllBoards4(struct boards *bop, struct solvedBoards *solvedp) {
  int k, i, res, chunk, fail;
  struct futureTricks fut[MAXNOOFBOARDS];

  chunk=4; fail=1;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return -101;

  for (i=0; i<MAXNOOFBOARDS; i++)
      solvedp->solvedBoard[i].cards=0;

  omp_set_num_threads(noOfCores);	/* Added after suggestion by Dirk Willecke. */

  #pragma omp parallel shared(bop, solvedp, chunk, fail) private(k)
  {

    #pragma omp for schedule(dynamic, chunk)

    for (k=0; k<bop->noOfBoards; k++) {
      res=SolveBoard(bop->deals[k], bop->target[k], bop->solutions[k],
        bop->mode[k], &fut[k], omp_get_thread_num());
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

int SolveAllBoards1(struct boards *bop, struct solvedBoards *solvedp) {
  int k, i, res, chunk, fail;
  struct futureTricks fut[MAXNOOFBOARDS];

  chunk=1; fail=1;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return -101;

  for (i=0; i<MAXNOOFBOARDS; i++)
    solvedp->solvedBoard[i].cards=0;

  omp_set_num_threads(noOfCores);	/* Added after suggestion by Dirk Willecke. */

  #pragma omp parallel shared(bop, solvedp, chunk, fail) private(k)
  {

    #pragma omp for schedule(dynamic, chunk)

    for (k=0; k<bop->noOfBoards; k++) {
      res=SolveBoard(bop->deals[k], bop->target[k], bop->solutions[k],
        bop->mode[k], &fut[k], omp_get_thread_num());
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

  res=SolveAllBoards4(&bo, &solved);
  if (res==1) {
    for (ind=0; ind<20; ind++) {
      tablep->resTable[bo.deals[ind].trump][rho[bo.deals[ind].first]]=
	    13-solved.solvedBoard[ind].score[0];
    }
    return 1;
  }

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

#ifdef PBN 
int STDCALL SolveBoardPBN(struct dealPBN dlpbn, int target,
    int solutions, int mode, struct futureTricks *futp, int thrId) {

  int res, k;
  struct deal dl;
  int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);

  if (ConvertFromPBN(dlpbn.remainCards, dl.remainCards)!=1)
    return -99;

  for (k=0; k<=2; k++) {
    dl.currentTrickRank[k]=dlpbn.currentTrickRank[k];
    dl.currentTrickSuit[k]=dlpbn.currentTrickSuit[k];
  }
  dl.first=dlpbn.first;
  dl.trump=dlpbn.trump;

  res=SolveBoard(dl, target, solutions, mode, futp, thrId);

  return res;
}

int STDCALL CalcDDtablePBN(struct ddTableDealPBN tableDealPBN, struct ddTableResults * tablep) {
  struct ddTableDeal tableDeal;
  int res;

  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards)!=1)
    return -99;

  res=CalcDDtable(tableDeal, tablep);

  return res;
}
#endif

#ifdef PBN_PLUS
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
	  return -99;
  }

  res=SolveAllBoards1(&bo, solvedp);

  return res;
}
#endif

#ifdef PBN_PLUS
int STDCALL CalcParPBN(struct ddTableDealPBN tableDealPBN, 
  struct ddTableResults * tablep, int vulnerable, struct parResults *presp) {
  int res;
  struct ddTableDeal tableDeal;
  int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);
  int STDCALL CalcPar(struct ddTableDeal tableDeal, int vulnerable, 
    struct ddTableResults * tablep, struct parResults *presp);

  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards)!=1)
    return -99;

  res=CalcPar(tableDeal, vulnerable, tablep, presp);

  return res;
}

int STDCALL CalcPar(struct ddTableDeal tableDeal, int vulnerable, 
    struct ddTableResults * tablep, struct parResults *presp) {

  int res;

  int Par(struct ddTableResults * tablep, struct parResults *presp, int vulnerable);

  res=CalcDDtable(tableDeal, tablep);

  if (res!=1)
    return res;

  res=Par(tablep, presp, vulnerable);

  return res;

}
#endif

int Par(struct ddTableResults * tablep, struct parResults *presp, int vulnerable) {
	/* vulnerable 0: None  1: Both  2: NS  3: EW */

/* The code for calculation of par score / contracts is based upon the
   perl code written by Matthew Kidd ACBLmerge. He has kindly given me permission
   to include a C++ adaptation in DDS. */ 

/* The Par function computes the par result and contracts. */

  
  int denom_conv[5]={4, 0, 1, 2, 3};
  /* Preallocate for efficiency. These hold result from last direction
     (N-S or E-W) examined. */
  int i, j, k, m, isvul;
  int current_side, both_sides_once_flag, denom_max, max_lower;
  int new_score_flag, sc1, sc2; 
  int prev_par_denom=0, prev_par_tricks=0;
   
  int ut, t1, t2, tt, score, dr, ke, tu, tu_max, t3, t4, n;
  struct par_suits_type par_suits[5];
  char contr_sep[2]={',','\0'};
  char temp[8], buff[4];

  int par_denom[2] = {-1, -1};	 /* 0-4 = NT,S,H,D,C */
  int par_tricks[2] = {6, 6};	 /* Initial "contract" beats 0 NT */
  int par_score[2] = {0, 0};
  int par_sacut[2] = {0, 0};     /* Undertricks for sacrifice (0 if not sac) */

  int rawscore(int denom, int tricks, int isvul);
  void IniSidesString(int dr, int i, int t1, int t2, char stri[]);
  int CalcMultiContracts(int max_lower, int tricks);

  /* Find best par result for N-S (i==0) or E-W (i==1). These will
     nearly always be the same, but when we have a "hot" situation
     they will not be. */

  for (i=0; i<=1; i++) {
    /* Start with the with the offensive side (current_side = 0) and alternate
       between sides seeking the to improve the result for the current side.*/

    current_side=0;  both_sides_once_flag=0;
    while (1) {

      /* Find best contract for current side that beats current contract.
         Choose highest contract if results are equal. */

      k=(i+current_side) % 2;

      isvul=((vulnerable==1)||(k ? (vulnerable==3) : (vulnerable==2)));

      new_score_flag=0;
      prev_par_denom=par_denom[i];
      prev_par_tricks=par_tricks[i];

    /* Calculate tricks and score values and 
       store them for each denomination in structure par_suits[5]. */

      for (j=0; j<=4; j++) {
        t1 = k ? tablep->resTable[denom_conv[j]][1] : tablep->resTable[denom_conv[j]][0];
        t2 = k ? tablep->resTable[denom_conv[j]][3] : tablep->resTable[denom_conv[j]][2];
        tt = Max(t1, t2);
	/* tt is the maximum number of tricks current side can take in 
           denomination.*/
        par_suits[j].suit=j;
        par_suits[j].tricks=tt;
        if ((tt > par_tricks[i]) || ((tt == par_tricks[i]) &&
	  (j < par_denom[i]))) 
	  par_suits[j].score=rawscore(j, tt, isvul);
        else
	  par_suits[j].score=rawscore(-1, prev_par_tricks - tt, isvul);
      }
		
       /* Sort the items in the par_suits structure with decreasing order of the 
       values on the scores. */
	  
      for (int s = 1; s < 5; s++) { 
        struct par_suits_type tmp = par_suits[s]; 
        int r = s; 
        for (; r && tmp.score > par_suits[r - 1].score ; --r) 
          par_suits[r] = par_suits[r - 1]; 
        par_suits[r] = tmp; 
      }
	  
      /* Do the iteration as before but now in the order of the sorted denominations. */
		
      for (m=0; m<=4; m++) {
	j=par_suits[m].suit;
	tt=par_suits[m].tricks;
 
	if ((tt > par_tricks[i]) || ((tt == par_tricks[i]) &&
	  (j < par_denom[i]))) {
	  /* Can bid higher and make contract.*/
	  score=rawscore(j, tt, isvul);
	}
	else {
	  /* Bidding higher in this denomination will not beat previous denomination
             and may be a sacrifice. */
	  ut=prev_par_tricks - tt;
	  if (j >= prev_par_denom) {
	    /* Sacrifices higher than 7N are not permitted (but long ago
               the official rules did not prohibit bidding higher than 7N!) */
	    if (prev_par_tricks == 13)
	      continue;
            /* It will be necessary to bid one level higher, resulting in
               one more undertrick. */
	    ut++;
	  }
	  /* Not a sacrifice (due to par_tricks > prev_par_tricks) */
	  if (ut <= 0)
	    continue;
	  /* Compute sacrifice.*/
	  score=rawscore(-1, ut, isvul);
	}
	if (current_side == 1)
	  score=-score;

	if (((current_side == 0)&&(score > par_score[i])) || 
	  ((current_side == 1)&&(score < par_score[i]))) {
	  new_score_flag = 1;
	  par_score[i] = score;
	  par_denom[i] = j;		

	  if (((current_side == 0)&&(score > 0)) || 
	    ((current_side == 1)&&(score < 0))) {
	    /* New par score from a making contract. 
	       Can immediately update since score at same level in higher
	       ranking suit is always >= score in lower ranking suit and 
               better than any sacrifice. */
	    par_tricks[i] = tt;
	    par_sacut[i] = 0;
	  }
	  else {
	    par_tricks[i] = tt + ut;
	    par_sacut[i] = ut;
	  }
	}
      }

      if (!new_score_flag && both_sides_once_flag)
	break; 
      both_sides_once_flag = 1;
      current_side = 1 - current_side;
    }
  }

  presp->parScore[0][0]='N';
  presp->parScore[0][1]='S';
  presp->parScore[0][2]=' ';
  presp->parScore[0][3]='\0';
  presp->parScore[1][0]='E';
  presp->parScore[1][1]='W';
  presp->parScore[1][2]=' ';
  presp->parScore[1][3]='\0';

  itoa(par_score[0], temp, 10);
  strcat(presp->parScore[0], temp);
  itoa(par_score[1], temp, 10);
  strcat(presp->parScore[1], temp);

  for (i=0; i<=1; i++) {
    presp->parContractsString[0][0]='N';
    presp->parContractsString[0][1]='S';
    presp->parContractsString[0][2]=':';
    presp->parContractsString[0][3]='\0';
    presp->parContractsString[1][0]='E';
    presp->parContractsString[1][1]='W';
    presp->parContractsString[1][2]=':';
    presp->parContractsString[1][3]='\0';
  }

  if (par_score[0] == 0) {
    /* Neither side can make anything.*/
    return 1;
  }


  for (i=0; i<=1; i++) {

    if ( par_sacut[i] > 0 ) {
	  
      dr = (par_score[i] > 0) ? 0 : 1;
    
      for (j=par_denom[i]; j<=4; j++) {

        t1 = ((dr+i) % 2 ) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
        t2 = ((dr+i) % 2 ) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
        tt = (t1 > t2) ? t1 : t2;

	tu_max=0;
	for (m=4; m>=0; m--) {
	  t3 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][0] : tablep->resTable[denom_conv[m]][1];
          t4 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][2] : tablep->resTable[denom_conv[m]][3];
	  tu = (t3 > t4) ? t3 : t4;
	  if (tu > tu_max) {
	    tu_max=tu;
	    denom_max=m;
	  }
	}


        if (((par_tricks[i] - par_sacut[i]) != tt)||((par_denom[i] < denom_max)&&(j > denom_max)))  
          continue; 

	/* Continue if the par denomination is lower than the denomination of the opponent's highest 
	trick number and the current denomination is larger than the denomination of the opponent's 
	highest trick number. */

	IniSidesString(dr, i, t1, t2, buff);

	if (presp->parContractsString[i][3]!='\0')
	  strcat(presp->parContractsString[i], contr_sep);

	strcat(presp->parContractsString[i], buff);

	itoa(par_tricks[i]-6, temp, 10);
	buff[0]=cardSuit[denom_conv[j]];
	buff[1]='x';
	buff[2]='\0';
	strcat(temp, buff);
	strcat(presp->parContractsString[i], temp);

	stat_contr[0]++;
      }
    }
    else {
      /* Par contract is a makeable contract.*/
      dr = (par_score[i] < 0) ? 0 : 1;

      /* If spades or diamonds, lower major / minor may also be a par contract.*/
      ke = (par_denom[i] == 1 || par_denom[i] == 3) ? 1 : 0;
	  
      for (j=par_denom[i]; j<=par_denom[i]+ke; j++) {
        t1 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
	t2 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
	tt = (t1 > t2) ? t1 : t2;

	if (tt < par_tricks[i]) { continue; }

	IniSidesString(dr, i, t1, t2, buff);

	tu_max=0;
	for (m=0; m<=4; m++) {
	  t3 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][0] : tablep->resTable[denom_conv[m]][1];
          t4 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][2] : tablep->resTable[denom_conv[m]][3];
	  tu = (t3 > t4) ? t3 : t4;
	  if (tu > tu_max) {
	    tu_max=tu;
	    denom_max=m;  /* Lowest denomination if several denominations have max tricks. */
	  }
	}

	if (presp->parContractsString[i][3]!='\0')
	  strcat(presp->parContractsString[i], contr_sep);

	strcat(presp->parContractsString[i], buff);

	if (denom_max < par_denom[i]) 
	  max_lower = par_tricks[i] - tu_max - 2;
	else
	  max_lower = par_tricks[i] - tu_max - 1;

	/* max_lower is the maximal contract lowering, otherwise opponent contract is
	higher. It is already known that par_score is high enough to make
	opponent sacrifices futile. 
	To find the actual contract lowering allowed, it must be checked that the
	lowered contract still gets the score bonus points that is present in par score.*/

	while (max_lower > 0) {
	  if (denom_max < par_denom[i]) 
	    sc1 = -rawscore(-1, par_tricks[i] - max_lower - tu_max, isvul);
	  else
	    sc1 = -rawscore(-1, par_tricks[i] - max_lower - tu_max + 1, isvul);
	  /* Score for undertricks needed to beat the tentative lower par contract.*/
	  sc2 = rawscore(par_denom[i], par_tricks[i] - max_lower, isvul);
	  /* Score for making the tentative lower par contract. */
	  if (sc2 < sc1)
	    break;
	  else
	    max_lower--;
	  /* Tentative lower par contract must be 1 trick higher, since the cost
	  for the sacrifice is too small. */
	}

	switch (par_denom[i]) {
	  case 0:  k = 0; break;
	  case 1:  case 2: k = 1; break;
	  case 3:  case 4: k = 2;
	}

	max_lower = Min(max_low[k][par_tricks[i]-6], max_lower);

	n = CalcMultiContracts(max_lower, par_tricks[i]);

	itoa(n, temp, 10);
	buff[0]=cardSuit[denom_conv[j]];
	buff[1]='\0';
	strcat(temp, buff);
	strcat(presp->parContractsString[i], temp);

	stat_contr[1]++;
      }


      /* Deal with special case of 3N/5m (+400/600) */
      if ((par_denom[i] == 0) && (par_tricks[i] == 9)) {
	    
	for (j=3; j<=4; j++) {
	  t1 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
	  t2 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
	  tt = (t1 > t2) ? t1 : t2;

	  if (tt != 11) { continue; }

	  IniSidesString(dr, i, t1, t2, buff);

	  if (presp->parContractsString[i][3]!='\0')
	    strcat(presp->parContractsString[i], contr_sep);

	  strcat(presp->parContractsString[i], buff);

	  itoa(5, temp, 10);
	  buff[0]=cardSuit[denom_conv[j]];
	  buff[1]='\0';
	  strcat(temp, buff);
	  strcat(presp->parContractsString[i], temp);

	  stat_contr[2]++;
	}
	    
      }
      /* Deal with special case of 2S/2H (+110) which may have 3C and 3D
         as additional par contract(s).*/
      if ((par_denom[i] <=2) && (par_denom[i] != 0) && (par_tricks[i] == 8)) {
	/* Check if 3C and 3D make.*/
	for (j=3; j<=4; j++) {
	  t1 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
	  t2 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
	  tt = (t1 > t2) ? t1 : t2;

	  if (tt != 9) { continue; }

	  IniSidesString(dr, i, t1, t2, buff);

	  tu_max=0;

	  for (m=4; m>=0; m--) {
	    t3 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][0] : tablep->resTable[denom_conv[m]][1];
            t4 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][2] : tablep->resTable[denom_conv[m]][3];
	    tu = (t3 > t4) ? t3 : t4;
	    if (tu > tu_max) {
	      tu_max=tu;
	      denom_max=m;
	    }
	  }

	  if (presp->parContractsString[i][3]!='\0')
	    strcat(presp->parContractsString[i], contr_sep);

	  strcat(presp->parContractsString[i], buff);

	  if (denom_max < par_denom[i]) 
	    max_lower = 9 - tu_max - 2;
	  else
	    max_lower = 9 - tu_max - 1;

	  /* max_lower is the maximal contract lowering, otherwise opponent contract is
	  higher. It is already known that par_score is high enough to make
	  opponent sacrifices futile. 
	  To find the actual contract lowering allowed, it must be checked that the
	  lowered contract still gets the score bonus points that is present in par score.*/

	  while (max_lower > 0) {
	    if (denom_max < par_denom[i]) 
	      sc1 = -rawscore(-1, par_tricks[i] - max_lower - tu_max, isvul);
	    else
	      sc1 = -rawscore(-1, par_tricks[i] - max_lower - tu_max + 1, isvul);
	    /* Score for undertricks needed to beat the tentative lower par contract.*/
	    sc2 = rawscore(par_denom[i], par_tricks[i] - max_lower, isvul);
	    /* Score for making the tentative lower par contract. */
	    if (sc2 < sc1)
	      break;
	    else
	      max_lower--;
	    /* Tentative lower par contract must be 1 trick higher, since the cost
	    for the sacrifice is too small. */
	  }

	  switch (par_denom[i]) {
	    case 0:  k = 0; break;
	    case 1:  case 2: k = 1; break;
	    case 3:  case 4: k = 2;
	  }

	  max_lower = Min(max_low[k][3], max_lower);

	  n = CalcMultiContracts(max_lower, 9);

	  itoa(n, temp, 10);
	  buff[0]=cardSuit[denom_conv[j]];
	  buff[1]='\0';
	  strcat(temp, buff);
	  strcat(presp->parContractsString[i], temp);

	  stat_contr[3]++;
	}
      }
      /* Deal with special case 1NT (+90) which may have 2C or 2D as additonal par
         contracts(s). */
      if ((par_denom[i] == 0) && (par_tricks[i] == 7)) {
	for (j=3; j<=4; j++) {
	  t1 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][0] : tablep->resTable[denom_conv[j]][1];
	  t2 = ((dr+i) % 2) ? tablep->resTable[denom_conv[j]][2] : tablep->resTable[denom_conv[j]][3];
	  tt = (t1 > t2) ? t1 : t2;

	  if (tt != 8) { continue; }

	  IniSidesString(dr, i, t1, t2, buff);

	  tu_max=0;
	  for (m=4; m>=0; m--) {
	    t3 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][0] : tablep->resTable[denom_conv[m]][1];
            t4 = ((dr+i) % 2 == 0) ? tablep->resTable[denom_conv[m]][2] : tablep->resTable[denom_conv[m]][3];
	    tu = (t3 > t4) ? t3 : t4;
	    if (tu > tu_max) {
	      tu_max=tu;
	      denom_max=m;
	    }
	  }

	  if (presp->parContractsString[i][3]!='\0')
	    strcat(presp->parContractsString[i], contr_sep);

	  strcat(presp->parContractsString[i], buff);

	  if (denom_max < par_denom[i]) 
	    max_lower = 8 - tu_max - 2;
	  else
	    max_lower = 8 - tu_max - 1;

	  /* max_lower is the maximal contract lowering, otherwise opponent contract is
	  higher. It is already known that par_score is high enough to make
	  opponent sacrifices futile. 
	  To find the actual contract lowering allowed, it must be checked that the
	  lowered contract still gets the score bonus points that is present in par score.*/

	  while (max_lower > 0) {
	    if (denom_max < par_denom[i]) 
	      sc1 = -rawscore(-1, par_tricks[i] - max_lower - tu_max, isvul);
	    else
	      sc1 = -rawscore(-1, par_tricks[i] - max_lower - tu_max + 1, isvul);
	    /* Score for undertricks needed to beat the tentative lower par contract.*/
	    sc2 = rawscore(par_denom[i], par_tricks[i] - max_lower, isvul);
	    /* Score for making the tentative lower par contract. */
	    if (sc2 < sc1)
	      break;
	    else
	      max_lower--;
	    /* Tentative lower par contract must be 1 trick higher, since the cost
	    for the sacrifice is too small. */
	  }

	  switch (par_denom[i]) {
	    case 0:  k = 0; break;
	    case 1:  case 2: k = 1; break;
	    case 3:  case 4: k = 2;
	  }

	  max_lower = Min(max_low[k][3], max_lower);

	  n = CalcMultiContracts(max_lower, 8);

	  itoa(n, temp, 10);
	  buff[0]=cardSuit[denom_conv[j]];
	  buff[1]='\0';
	  strcat(temp, buff);
	  strcat(presp->parContractsString[i], temp);

	  stat_contr[4]++;
	}
      }
    }
  }

  return 1;
}


int rawscore(int denom, int tricks, int isvul) {
  int game_bonus, level, score;

  /* Computes score for undoubled contract or a doubled contract with
     for a given number of undertricks. These are the only possibilities
     for a par contract (aside from a passed out hand). 
  
     denom  - 0 = NT, 1 = Spades, 2 = Hearts, 3 = Diamonds, 4 = Clubs
             (same order as results from double dummy solver); -1 undertricks
     tricks - For making contracts (7-13); otherwise, number of undertricks.
     isvul  - True if vulnerable */

  if (denom==-1) {
    if (isvul)
      return -300 * tricks + 100;
    if (tricks<=3)
      return -200 * tricks + 100;
    return -300 * tricks + 400;
  }
  else {
    level=tricks-6;
    game_bonus=0;
    if (denom==0) {
      score=10 + 30 * level;
      if (level>=3)
	game_bonus=1;
    }
    else if ((denom==1)||(denom==2)) {
      score=30 * level;
      if (level>=4)
        game_bonus=1;
    }
    else {
      score=20 * level;
      if (level>=5)
	game_bonus=1;
    }
    if (game_bonus) {
      score+= (isvul ? 500 : 300);
    }
    else
      score+=50;

    if (level==6) {
      score+= (isvul ? 750 : 500);
    }
    else if (level==7) {
      score+= (isvul ? 1500 : 1000);
    }
  }

  return score;
}


void IniSidesString(int dr, int i, int t1, int t2, char stri[]) {

   if ((dr+i) % 2 ) {
     if (t1==t2) {
       stri[0]='N';
       stri[1]='S';
       stri[2]=' ';
       stri[3]='\0';
     }
     else if (t1 > t2) {
       stri[0]='N';
       stri[1]=' ';
       stri[2]='\0';
     }
     else {
       stri[0]='S';
       stri[1]=' ';
       stri[2]='\0';
     }
   }
   else {
     if (t1==t2) {
       stri[0]='E';
       stri[1]='W';
       stri[2]=' ';
       stri[3]='\0';
     }
     else if (t1 > t2) {
       stri[0]='E';
       stri[1]=' ';
       stri[2]='\0';
     }
     else {
       stri[0]='W';
       stri[1]=' ';
       stri[2]='\0';
     }
   }
   return;
}


int CalcMultiContracts(int max_lower, int tricks) {
  int n;

  switch (tricks-6) {
    case 5: if (max_lower==3) {n = 2345;}
	    else if (max_lower==2) {n = 345;}
	    else if (max_lower==1) {n = 45;}
	    else {n = 5;}
	    break;
    case 4: if (max_lower==3) {n = 1234;}
	    else if (max_lower==2) {n = 234;}
	    else if (max_lower==1) {n = 34;}
	    else {n = 4;}
	    break;
    case 3: if (max_lower==2) {n = 123;}
	    else if (max_lower==1) {n = 23;}
	    else {n = 3;}
	    break;
    case 2: if (max_lower==1) {n = 12;}
	    else {n = 2;}
	    break;
    default: n = tricks-6;
  }
  return n;
}




