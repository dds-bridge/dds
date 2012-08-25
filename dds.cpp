
/* DDS 1.1.15   A bridge double dummy solver.				      */
/* Copyright (C) 2006-2012 by Bo Haglund                                      */
/* Cleanups and porting to Linux and MacOSX (C) 2006 by Alex Martelli         */
/*								              */
/* This program is free software; you can redistribute it and/or              */
/* modify it under the terms of the GNU General Public License                */
/* as published by the Free software Foundation; either version 2             */
/* of the License, or (at your option) any later version.                     */ 
/*								              */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of             */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              */
/* GNU General Public License for more details.                               */
/*								              */
/* You should have received a copy of the GNU General Public License          */
/* along with this program; if not, write to the Free Software                */
/* Foundation, Inc, 51 Franklin Street, 5th Floor, Boston MA 02110-1301, USA. */

/*#include "stdafx.h"*/			/* Needed by Visual C++ */

#include "dll.h"

int nodeTypeStore[4];
int lho[4], rho[4], partner[4];
int trump;
unsigned short int bitMapRank[16];
unsigned short int lowestWin[50][4];
int nodes;
int trickNodes;
int no[50];
int iniDepth;
struct pos iniPosition, position;
struct pos lookAheadPos; /* Is initialized for starting
   alpha-beta search */
struct moveType forbiddenMoves[14];
struct moveType initialMoves[4];
struct movePlyType movePly[50];
int tricksTarget;
struct gameInfo game;
int newDeal;
int estTricks[4];
FILE *fp2, *fp7, *fp11;
struct moveType bestMove[50];
struct moveType bestMoveTT[50];
struct winCardType temp_win[5];
int nodeSetSizeLimit=0;
int winSetSizeLimit=0;
int lenSetSizeLimit=0;
unsigned long long maxmem, allocmem, summem;
int wmem, nmem, lmem;
int maxIndex;
int wcount, ncount, lcount;
int clearTTflag=FALSE, windex=-1;
int ttCollect=FALSE;
int suppressTTlog=FALSE;
int * highestRank;
int * counttable;
struct adaptWinRanksType * adaptWins;
unsigned char cardRank[15], cardSuit[5], cardHand[4];
struct posSearchType *rootnp[14][4];
struct winCardType **pw;
struct nodeCardsType **pn;
struct posSearchType **pl;

#ifdef _MANAGED
#pragma managed(push, off)
#endif

#if defined(_WIN32)
extern "C" BOOL APIENTRY DllMain(HMODULE hModule,
				DWORD ul_reason_for_call,
				LPVOID lpReserved) {

  if (ul_reason_for_call==DLL_PROCESS_ATTACH)
    InitStart(0,0);
  else if (ul_reason_for_call==DLL_PROCESS_DETACH) {
    Wipe();
    if (pw[0])
      free(pw[0]);
    pw[0]=NULL;
    if (pn[0])
      free(pn[0]);
    pn[0]=NULL;
    if (pl[0])
      free(pl[0]);
    pl[0]=NULL;
    if (pw)
      free(pw);
    pw=NULL;
    if (pn)
      free(pn);
    pn=NULL;
    if (pl)
      free(pl);
    pl=NULL;
    if (ttStore)
      free(ttStore);
    ttStore=NULL;
    if (rel)
      free(rel);
    rel=NULL;
    if (highestRank)
      free(highestRank);
    highestRank=NULL;
    if (counttable)
      free(counttable);
    counttable=NULL;
    if (adaptWins)
      free(adaptWins);
    adaptWins=NULL;
	/*_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_WNDW );
	_CrtDumpMemoryLeaks();*/	/* MEMORY LEAK? */
  }
  return 1;
}
#endif


#ifdef PLUSVER
  int STDCALL SolveBoard(struct deal dl, int target,
    int solutions, int mode, struct futureTricks *futp, int threadIndex) {
#else
  int STDCALL SolveBoard(struct deal dl, int target,
    int solutions, int mode, struct futureTricks *futp) {
#endif

  int k, n, cardCount, found, totalTricks, tricks, last, checkRes;
  int g, upperbound, lowerbound, first, i, j, h, forb, ind, flag, noMoves;
  int similarDeal;
  int newTrump;
  unsigned short int diffDeal;
  unsigned short int aggDeal;
  int mcurr, val, payOff, handToPlay;
  int noStartMoves;
  int handRelFirst;
  int noOfCardsPerHand[4];
  int latestTrickSuit[4];
  int latestTrickRank[4];
  int maxHand=0, maxSuit=0, maxRank;
  unsigned short int aggrRemain;
  struct movePlyType temp;
  struct moveType mv, cd;
  int hiwinSetSize=0, hinodeSetSize=0;
  int hilenSetSize=0;
  int MaxnodeSetSize=0;
  int MaxwinSetSize=0;
  int MaxlenSetSize=0;
  /*FILE *fp;*/
  
  /*InitStart(0,0);*/   /* Include InitStart() if inside SolveBoard,
			   but preferable InitStart should be called outside
			   SolveBoard like in DllMain for Windows. */

  for (k=0; k<=13; k++) {
    forbiddenMoves[k].rank=0;
    forbiddenMoves[k].suit=0;
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
    tricksTarget=99;
  else
    tricksTarget=target;

  newDeal=FALSE; newTrump=FALSE;
  diffDeal=0; aggDeal=0;
  cardCount=0;
  for (i=0; i<=3; i++) {
    for (j=0; j<=3; j++) {
      cardCount+=counttable[dl.remainCards[i][j]>>2];
      diffDeal+=((dl.remainCards[i][j]>>2)^
	    (game.suit[i][j]));
      aggDeal+=(dl.remainCards[i][j]>>2);
      if (game.suit[i][j]!=dl.remainCards[i][j]>>2) {
        game.suit[i][j]=dl.remainCards[i][j]>>2;
	newDeal=TRUE;
      }
    }
  }

  if (newDeal) {
    if (diffDeal==0)
      similarDeal=TRUE;
    else if ((aggDeal/diffDeal)>SIMILARDEALLIMIT)
      similarDeal=TRUE;
    else
      similarDeal=FALSE;
  }
  else
    similarDeal=FALSE;

  if (dl.trump!=trump)
    newTrump=TRUE;

  for (i=0; i<=3; i++)
    for (j=0; j<=3; j++)
      noOfCardsPerHand[i]+=counttable[game.suit[i][j]];

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
    handToPlay=handId(dl.first, 3);
    handRelFirst=3;
    noStartMoves=3;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (game.suit[handToPlay][k]!=0) {
          latestTrickSuit[handToPlay]=k;
          latestTrickRank[handToPlay]=
            InvBitMapRank(game.suit[handToPlay][k]);
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
    handToPlay=handId(dl.first, 2);
    handRelFirst=2;
    noStartMoves=2;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (game.suit[handToPlay][k]!=0) {
          latestTrickSuit[handToPlay]=k;
          latestTrickRank[handToPlay]=
            InvBitMapRank(game.suit[handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
	if (game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(game.suit[handId(dl.first, 3)][k]);
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
    handToPlay=handId(dl.first,1);
    handRelFirst=1;
    noStartMoves=1;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (game.suit[handToPlay][k]!=0) {
          latestTrickSuit[handToPlay]=k;
          latestTrickRank[handToPlay]=
            InvBitMapRank(game.suit[handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
	if (game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (game.suit[handId(dl.first, 2)][k]!=0) {
          latestTrickSuit[handId(dl.first, 2)]=k;
          latestTrickRank[handId(dl.first, 2)]=
            InvBitMapRank(game.suit[handId(dl.first, 2)][k]);
          break;
        }
      }
      latestTrickSuit[dl.first]=dl.currentTrickSuit[0];
      latestTrickRank[dl.first]=dl.currentTrickRank[0];
    }
  }
  else {
    handToPlay=dl.first;
    handRelFirst=0;
    noStartMoves=0;
    if (cardCount<=4) {
      for (k=0; k<=3; k++) {
        if (game.suit[handToPlay][k]!=0) {
          latestTrickSuit[handToPlay]=k;
          latestTrickRank[handToPlay]=
            InvBitMapRank(game.suit[handToPlay][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (game.suit[handId(dl.first, 3)][k]!=0) {
          latestTrickSuit[handId(dl.first, 3)]=k;
          latestTrickRank[handId(dl.first, 3)]=
            InvBitMapRank(game.suit[handId(dl.first, 3)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (game.suit[handId(dl.first, 2)][k]!=0) {
          latestTrickSuit[handId(dl.first, 2)]=k;
          latestTrickRank[handId(dl.first, 2)]=
            InvBitMapRank(game.suit[handId(dl.first, 2)][k]);
          break;
        }
      }
      for (k=0; k<=3; k++) {
        if (game.suit[handId(dl.first, 1)][k]!=0) {
          latestTrickSuit[handId(dl.first, 1)]=k;
          latestTrickRank[handId(dl.first, 1)]=
            InvBitMapRank(game.suit[handId(dl.first, 1)][k]);
          break;
        }
      }
    }
  }

  trump=dl.trump;
  game.first=dl.first;
  first=dl.first;
  game.noOfCards=cardCount;
  if (dl.currentTrickRank[0]!=0) {
    game.leadHand=dl.first;
    game.leadSuit=dl.currentTrickSuit[0];
    game.leadRank=dl.currentTrickRank[0];
  }
  else {
    game.leadHand=0;
    game.leadSuit=0;
    game.leadRank=0;
  }

  for (k=0; k<=2; k++) {
    initialMoves[k].suit=255;
    initialMoves[k].rank=255;
  }

  for (k=0; k<noStartMoves; k++) {
    initialMoves[noStartMoves-1-k].suit=dl.currentTrickSuit[k];
    initialMoves[noStartMoves-1-k].rank=dl.currentTrickRank[k];
  }

  if (cardCount % 4)
    totalTricks=((cardCount-4)>>2)+2;
  else
    totalTricks=((cardCount-4)>>2)+1;
  checkRes=CheckDeal(&cd);
  if (game.noOfCards<=0) {
    DumpInput(-2, dl, target, solutions, mode);
    return -2;
  }
  if (game.noOfCards>52) {
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
    futp->suit[0]=latestTrickSuit[handToPlay];
    futp->rank[0]=latestTrickRank[handToPlay];
    futp->equals[0]=0;
    if ((target==0)&&(solutions<3))
      futp->score[0]=0;
    else if ((handToPlay==maxHand)||
	(partner[handToPlay]==maxHand))
      futp->score[0]=1;
    else
      futp->score[0]=0;
	/*_CrtDumpMemoryLeaks();*/ /* MEMORY LEAK? */
    return 1;
  }

  if ((mode!=2)&&
    (((newDeal)&&(!similarDeal)) 
      || newTrump || (winSetSize > SIMILARMAXWINNODES))) {
    Wipe();
	winSetSizeLimit=WINIT;
    nodeSetSizeLimit=NINIT;
    lenSetSizeLimit=LINIT;
    allocmem=(WINIT+1)*sizeof(struct winCardType);
	allocmem+=(NINIT+1)*sizeof(struct nodeCardsType);
	allocmem+=(LINIT+1)*sizeof(struct posSearchType);
    winCards=pw[0];
	nodeCards=pn[0];
	posSearch=pl[0];
	wcount=0; ncount=0; lcount=0;
    InitGame(0, FALSE, first, handRelFirst);
  }
  else {
    InitGame(0, TRUE, first, handRelFirst);
	/*fp2=fopen("dyn.txt", "a");
	fprintf(fp2, "wcount=%d, ncount=%d, lcount=%d\n", 
	  wcount, ncount, lcount);
    fprintf(fp2, "winSetSize=%d, nodeSetSize=%d, lenSetSize=%d\n", 
	  winSetSize, nodeSetSize, lenSetSize);
    fclose(fp2);*/
  }

  nodes=0; trickNodes=0;
  iniDepth=cardCount-4;
  hiwinSetSize=0;
  hinodeSetSize=0;

  if (mode==0) {
    MoveGen(&lookAheadPos, iniDepth);
    if (movePly[iniDepth].last==0) {
      futp->nodes=0;
    #ifdef BENCH
      futp->totalNodes=0;
    #endif
      futp->cards=1;
      futp->suit[0]=movePly[iniDepth].move[0].suit;
      futp->rank[0]=movePly[iniDepth].move[0].rank;
      futp->equals[0]=
	  movePly[iniDepth].move[0].sequence<<2;
      futp->score[0]=-2;
      /*_CrtDumpMemoryLeaks();*/ /* MEMORY LEAK? */
      return 1;
    }
  }
  if ((target==0)&&(solutions<3)) {
    MoveGen(&lookAheadPos, iniDepth);
    futp->nodes=0;
    #ifdef BENCH
    futp->totalNodes=0;
    #endif
    for (k=0; k<=movePly[iniDepth].last; k++) {
      futp->suit[k]=movePly[iniDepth].move[k].suit;
      futp->rank[k]=movePly[iniDepth].move[k].rank;
      futp->equals[k]=
	  movePly[iniDepth].move[k].sequence<<2;
      futp->score[k]=0;
    }
    if (solutions==1)
      futp->cards=1;
    else
      futp->cards=movePly[iniDepth].last+1;
    /*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
    return 1;
  }

  if ((target!=-1)&&(solutions!=3)) {
    val=ABsearch(&lookAheadPos, tricksTarget, iniDepth);
    temp=movePly[iniDepth];
    last=movePly[iniDepth].last;
    noMoves=last+1;
    hiwinSetSize=winSetSize;
    hinodeSetSize=nodeSetSize;
    hilenSetSize=lenSetSize;
    if (nodeSetSize>MaxnodeSetSize)
      MaxnodeSetSize=nodeSetSize;
    if (winSetSize>MaxwinSetSize)
      MaxwinSetSize=winSetSize;
    if (lenSetSize>MaxlenSetSize)
      MaxlenSetSize=lenSetSize;
    if (val==1)
      payOff=tricksTarget;
    else
      payOff=0;
    futp->cards=1;
    ind=2;

    if (payOff<=0) {
      futp->suit[0]=movePly[game.noOfCards-4].move[0].suit;
      futp->rank[0]=movePly[game.noOfCards-4].move[0].rank;
	futp->equals[0]=(movePly[game.noOfCards-4].move[0].sequence)<<2;
	if (tricksTarget>1)
        futp->score[0]=-1;
	else
	  futp->score[0]=0;
    }
    else {
      futp->suit[0]=bestMove[game.noOfCards-4].suit;
      futp->rank[0]=bestMove[game.noOfCards-4].rank;
	futp->equals[0]=(bestMove[game.noOfCards-4].sequence)<<2;
      futp->score[0]=payOff;
    }
  }
  else {
    g=estTricks[handToPlay];
    upperbound=13;
    lowerbound=0;
    do {
      if (g==lowerbound)
        tricks=g+1;
      else
        tricks=g;
      val=ABsearch(&lookAheadPos, tricks, iniDepth);
      if (val==TRUE)
        mv=bestMove[game.noOfCards-4];
      hiwinSetSize=Max(hiwinSetSize, winSetSize);
      hinodeSetSize=Max(hinodeSetSize, nodeSetSize);
	hilenSetSize=Max(hilenSetSize, lenSetSize);
      if (nodeSetSize>MaxnodeSetSize)
        MaxnodeSetSize=nodeSetSize;
      if (winSetSize>MaxwinSetSize)
        MaxwinSetSize=winSetSize;
	if (lenSetSize>MaxlenSetSize)
        MaxlenSetSize=lenSetSize;
      if (val==FALSE) {
	upperbound=tricks-1;
	g=upperbound;
      }	
      else {
	lowerbound=tricks;
	g=lowerbound;
      }
      InitSearch(&iniPosition, game.noOfCards-4,
        initialMoves, first, TRUE);
    }
    while (lowerbound<upperbound);
    payOff=g;
    temp=movePly[iniDepth];
    last=movePly[iniDepth].last;
    noMoves=last+1;
    ind=2;
    bestMove[game.noOfCards-4]=mv;
    futp->cards=1;
    if (payOff<=0) {
      futp->score[0]=0;
      futp->suit[0]=movePly[game.noOfCards-4].move[0].suit;
      futp->rank[0]=movePly[game.noOfCards-4].move[0].rank;
	futp->equals[0]=(movePly[game.noOfCards-4].move[0].sequence)<<2;
    }
    else {
      futp->score[0]=payOff;
      futp->suit[0]=bestMove[game.noOfCards-4].suit;
      futp->rank[0]=bestMove[game.noOfCards-4].rank;
	futp->equals[0]=(bestMove[game.noOfCards-4].sequence)<<2;
    }
    tricksTarget=payOff;
  }

  if ((solutions==2)&&(payOff>0)) {
    forb=1;
    ind=forb;
    while ((payOff==tricksTarget)&&(ind<(temp.last+1))) {
      forbiddenMoves[forb].suit=bestMove[game.noOfCards-4].suit;
      forbiddenMoves[forb].rank=bestMove[game.noOfCards-4].rank;
      forb++; ind++;
      /* All moves before bestMove in the move list shall be
      moved to the forbidden moves list, since none of them reached
      the target */
      mcurr=movePly[iniDepth].current;
      for (k=0; k<=movePly[iniDepth].last; k++)
        if ((bestMove[iniDepth].suit==movePly[iniDepth].move[k].suit)
          &&(bestMove[iniDepth].rank==movePly[iniDepth].move[k].rank))
          break;
      for (i=0; i<k; i++) {  /* All moves until best move */
        flag=FALSE;
        for (j=0; j<forb; j++) {
          if ((movePly[iniDepth].move[i].suit==forbiddenMoves[j].suit)
            &&(movePly[iniDepth].move[i].rank==forbiddenMoves[j].rank)) {
            /* If the move is already in the forbidden list */
            flag=TRUE;
            break;
          }
        }
        if (!flag) {
          forbiddenMoves[forb]=movePly[iniDepth].move[i];
          forb++;
        }
      }
      InitSearch(&iniPosition, game.noOfCards-4,
          initialMoves, first, TRUE);
      val=ABsearch(&lookAheadPos, tricksTarget, iniDepth);
      hiwinSetSize=winSetSize;
      hinodeSetSize=nodeSetSize;
	  hilenSetSize=lenSetSize;
      if (nodeSetSize>MaxnodeSetSize)
        MaxnodeSetSize=nodeSetSize;
      if (winSetSize>MaxwinSetSize)
        MaxwinSetSize=winSetSize;
	if (lenSetSize>MaxlenSetSize)
        MaxlenSetSize=lenSetSize;
      if (val==TRUE) {
        payOff=tricksTarget;
        futp->cards=ind;
        futp->suit[ind-1]=bestMove[game.noOfCards-4].suit;
        futp->rank[ind-1]=bestMove[game.noOfCards-4].rank;
	  futp->equals[ind-1]=(bestMove[game.noOfCards-4].sequence)<<2;
        futp->score[ind-1]=payOff;
      }
      else
        payOff=0;
    }
  }
  else if ((solutions==2)&&(payOff==0)&&
	((target==-1)||(tricksTarget==1))) {
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

  if ((solutions==3)&&(payOff>0)) {
    forb=1;
    ind=forb;
    for (i=0; i<last; i++) {
      forbiddenMoves[forb].suit=bestMove[game.noOfCards-4].suit;
      forbiddenMoves[forb].rank=bestMove[game.noOfCards-4].rank;
      forb++; ind++;

      g=payOff;
      upperbound=payOff;
      lowerbound=0;
      InitSearch(&iniPosition, game.noOfCards-4,
      initialMoves, first, TRUE);
      do {
        if (g==lowerbound)
          tricks=g+1;
        else
          tricks=g;
        val=ABsearch(&lookAheadPos, tricks, iniDepth);
        if (val==TRUE)
          mv=bestMove[game.noOfCards-4];
        hiwinSetSize=Max(hiwinSetSize, winSetSize);
        hinodeSetSize=Max(hinodeSetSize, nodeSetSize);
	hilenSetSize=Max(hilenSetSize, lenSetSize);
        if (nodeSetSize>MaxnodeSetSize)
          MaxnodeSetSize=nodeSetSize;
        if (winSetSize>MaxwinSetSize)
          MaxwinSetSize=winSetSize;
	if (lenSetSize>MaxlenSetSize)
          MaxlenSetSize=lenSetSize;
        if (val==FALSE) {
	  upperbound=tricks-1;
	  g=upperbound;
	}	
	else {
	  lowerbound=tricks;
	  g=lowerbound;
	}
        InitSearch(&iniPosition, game.noOfCards-4,
            initialMoves, first, TRUE);
      }
      while (lowerbound<upperbound);
      payOff=g;
      if (payOff==0) {
        last=movePly[iniDepth].last;
        futp->cards=temp.last+1;
        for (j=0; j<=last; j++) {
          futp->suit[ind-1+j]=movePly[game.noOfCards-4].move[j].suit;
          futp->rank[ind-1+j]=movePly[game.noOfCards-4].move[j].rank;
	    futp->equals[ind-1+j]=(movePly[game.noOfCards-4].move[j].sequence)<<2;
          futp->score[ind-1+j]=payOff;
        }
        break;
      }
      else {
        bestMove[game.noOfCards-4]=mv;

        futp->cards=ind;
        futp->suit[ind-1]=bestMove[game.noOfCards-4].suit;
        futp->rank[ind-1]=bestMove[game.noOfCards-4].rank;
	futp->equals[ind-1]=(bestMove[game.noOfCards-4].sequence)<<2;
        futp->score[ind-1]=payOff;
      }   
    }
  }
  else if ((solutions==3)&&(payOff==0)) {
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
    forbiddenMoves[k].suit=0;
    forbiddenMoves[k].rank=0;
  }

  futp->nodes=trickNodes;
  #ifdef BENCH
  futp->totalNodes=nodes;
  #endif
  /*if ((wcount>0)||(ncount>0)||(lcount>0)) {
    fp2=fopen("dyn.txt", "a");
	fprintf(fp2, "wcount=%d, ncount=%d, lcount=%d\n", 
	  wcount, ncount, lcount);
    fprintf(fp2, "winSetSize=%d, nodeSetSize=%d, lenSetSize=%d\n", 
	  winSetSize, nodeSetSize, lenSetSize);
	fprintf(fp2, "\n");
    fclose(fp2);
  }*/
  /*_CrtDumpMemoryLeaks();*/  /* MEMORY LEAK? */
  return 1;
}

struct relRanksType * rel;
struct ttStoreType * ttStore;
struct nodeCardsType * nodeCards;
struct winCardType * winCards;
struct posSearchType * posSearch;
unsigned short int iniRemovedRanks[4];
int nodeSetSize=0; /* Index with range 0 to nodeSetSizeLimit */
int winSetSize=0;  /* Index with range 0 to winSetSizeLimit */
int lenSetSize=0;  /* Index with range 0 to lenSetSizeLimit */
int lastTTstore=0;

int _initialized=0;

void InitStart(int gb_ram, int ncores) {
  int k, r, i, j;
  unsigned short int res;
  unsigned long long pcmem;	/* kbytes */

  if (_initialized)
      return;
  _initialized = 1;
 
  ttStore = (struct ttStoreType *)calloc(SEARCHSIZE, sizeof(struct ttStoreType));
  if (ttStore==NULL)
    exit(1);

  if ((gb_ram==0)||(ncores==0)) {		/* Autoconfig */

#ifdef _WIN32
  
    SYSTEM_INFO temp;

    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);

    pcmem=(unsigned long long)(statex.ullTotalPhys/1024);

    GetSystemInfo(&temp);

#endif

  }
  else {
    pcmem=(unsigned long long)(1000000 * gb_ram);
  }

  nodeSetSizeLimit=NINIT;
  winSetSizeLimit=WINIT;
  lenSetSizeLimit=LINIT;

  if ((gb_ram!=0)&&(ncores!=0)) 
    maxmem=gb_ram * ((8000001*sizeof(struct nodeCardsType)+
		   25000001*sizeof(struct winCardType)+
		   400001*sizeof(struct posSearchType)));
  else {
    maxmem = (unsigned long long)(pcmem-32678) * 700;  
	  /* Linear calculation of maximum memory, formula by Michiel de Bondt */

    if (maxmem < 10485760) exit (1);
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


  summem=(WINIT+1)*sizeof(struct winCardType)+
	     (NINIT+1)*sizeof(struct nodeCardsType)+
		 (LINIT+1)*sizeof(struct posSearchType);
  wmem=(WSIZE+1)*sizeof(struct winCardType);
  nmem=(NSIZE+1)*sizeof(struct nodeCardsType);
  lmem=(LSIZE+1)*sizeof(struct posSearchType);
  maxIndex=(int)(maxmem-summem)/((WSIZE+1) * sizeof(struct winCardType)); 

  pw = (struct winCardType **)calloc(maxIndex+1, sizeof(struct winCardType *));
  if (pw==NULL)
    exit(1);
  pn = (struct nodeCardsType **)calloc(maxIndex+1, sizeof(struct nodeCardsType *));
  if (pn==NULL)
    exit(1);
  pl = (struct posSearchType **)calloc(maxIndex+1, sizeof(struct posSearchType *));
  if (pl==NULL)
    exit(1);
  for (k=0; k<=maxIndex; k++) {
    if (pw[k])
      free(pw[k]);
    pw[k]=NULL;
  }
  for (k=0; k<=maxIndex; k++) {
    if (pn[k])
      free(pn[k]);
    pn[k]=NULL;
  }
  for (k=0; k<=maxIndex; k++) {
    if (pl[k])
      free(pl[k]);
    pl[k]=NULL;
  }

  pw[0] = (struct winCardType *)calloc(winSetSizeLimit+1, sizeof(struct winCardType));
  if (pw[0]==NULL) 
    exit(1);
  allocmem=(winSetSizeLimit+1)*sizeof(struct winCardType);
  winCards=pw[0];
  pn[0] = (struct nodeCardsType *)calloc(nodeSetSizeLimit+1, sizeof(struct nodeCardsType));
  if (pn[0]==NULL)
    exit(1);
  allocmem+=(nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
  nodeCards=pn[0];
  pl[0] = (struct posSearchType *)calloc(lenSetSizeLimit+1, sizeof(struct posSearchType));
  if (pl[0]==NULL)
    exit(1);
  allocmem+=(lenSetSizeLimit+1)*sizeof(struct posSearchType);
  posSearch=pl[0];
  wcount=0; ncount=0; lcount=0;

  ttStore = (struct ttStoreType *)calloc(SEARCHSIZE, sizeof(struct ttStoreType));
  /*ttStore = new ttStoreType[SEARCHSIZE];*/
  if (ttStore==NULL)
    exit(1);

  rel = (struct relRanksType *)calloc(8192, sizeof(struct relRanksType));
  /*rel = new relRanksType[8192];*/
  if (rel==NULL)
    exit(1);

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

  adaptWins = (struct adaptWinRanksType *)calloc(8192, 
	sizeof(struct adaptWinRanksType));

  for (i=0; i<8192; i++)
    for (j=0; j<14; j++) {
      res=0;
      if (j==0)
	adaptWins[i].winRanks[j]=0;
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
	adaptWins[i].winRanks[j]=res;
      }
    }
 
  /*fp2=fopen("dyn.txt", "w");
  fclose(fp2);*/
  /*fp2=fopen("dyn.txt", "a");
  fprintf(fp2, "maxIndex=%ld\n", maxIndex);
  fclose(fp2);*/

  return;
}


void InitGame(int gameNo, int moveTreeFlag, int first, int handRelFirst) {

  int k, s, h, m, r, ord;
  unsigned int topBitRank=1;
  unsigned short int ind;

  #ifdef STAT
    fp2=fopen("stat.txt","w");
  #endif

  #ifdef TTDEBUG
  if (!suppressTTlog) {
    fp7=fopen("storett.txt","w");
    fp11=fopen("rectt.txt", "w");
    fclose(fp11);
    ttCollect=TRUE;
  }
  #endif	
  

  if (newDeal) {

    /* Initialization of the rel structure is implemented
       according to a solution given by Thomas Andrews */ 

    for (k=0; k<=3; k++)
      for (m=0; m<=3; m++)
        iniPosition.rankInSuit[k][m]=game.suit[k][m];

    for (s=0; s<4; s++) {
      rel[0].aggrRanks[s]=0;
      rel[0].winMask[s]=0;
      for (ord=1; ord<=13; ord++) {
	rel[0].absRank[ord][s].hand=-1;
	rel[0].absRank[ord][s].rank=0;
      }
      for (r=2; r<=14; r++)
        rel[0].relRank[r][s]=0;
    }
  
    for (ind=1; ind<8192; ind++) {
      if (ind>=(topBitRank+topBitRank)) {
       /* Next top bit */
        topBitRank <<=1;
      }

      rel[ind]=rel[ind ^ topBitRank];

      for (s=0; s<4; s++) {
	ord=0;
	for (r=14; r>=2; r--) {
	  if ((ind & bitMapRank[r])!=0) {
	    ord++;
	    rel[ind].relRank[r][s]=ord;
	    for (h=0; h<4; h++) {
	      if ((game.suit[h][s] & bitMapRank[r])!=0) {
		rel[ind].absRank[ord][s].hand=h;
		rel[ind].absRank[ord][s].rank=r;
		break;
	      }
	    }
	  }
	}
	for (k=ord+1; k<=13; k++) {
	  rel[ind].absRank[k][s].hand=-1;
	  rel[ind].absRank[k][s].rank=0;
	}
	for (h=0; h<4; h++) {
	  if (game.suit[h][s] & topBitRank) {
	    rel[ind].aggrRanks[s]=
	      (rel[ind].aggrRanks[s]>>2)|(h<<24);
	    rel[ind].winMask[s]=
	      (rel[ind].winMask[s]>>2)|(3<<24);
	    break;
	  }
	}
      }
    }
  }

  iniPosition.first[game.noOfCards-4]=first;
  iniPosition.handRelFirst=handRelFirst;
  lookAheadPos=iniPosition;
  
  estTricks[1]=6;
  estTricks[3]=6;
  estTricks[0]=7;
  estTricks[2]=7;

  #ifdef STAT
  fprintf(fp2, "Estimated tricks for hand to play:\n");	
  fprintf(fp2, "hand=%d  est tricks=%d\n", 
	  handToPlay, estTricks[handToPlay]);
  #endif

  InitSearch(&lookAheadPos, game.noOfCards-4, initialMoves, first,
    moveTreeFlag);
  return;
}


void InitSearch(struct pos * posPoint, int depth, struct moveType startMoves[], int first, int mtd)  {

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
    bestMove[d].rank=0;
    bestMoveTT[d].rank=0;
    /*bestMove[d].weight=0;
    bestMove[d].sequence=0; 0315 */
  }

  if (((handId(first, handRelFirst))==0)||
    ((handId(first, handRelFirst))==2)) {
    nodeTypeStore[0]=MAXNODE;
    nodeTypeStore[1]=MINNODE;
    nodeTypeStore[2]=MAXNODE;
    nodeTypeStore[3]=MINNODE;
  }
  else {
    nodeTypeStore[0]=MINNODE;
    nodeTypeStore[1]=MAXNODE;
    nodeTypeStore[2]=MINNODE;
    nodeTypeStore[3]=MAXNODE;
  }

  k=noOfStartMoves;
  posPoint->first[depth]=first;
  posPoint->handRelFirst=k;
  posPoint->tricksMAX=0;

  if (k>0) {
    posPoint->move[depth+k]=startMoves[k-1];
    move=startMoves[k-1];
  }

  posPoint->high[depth+k]=first;

  while (k>0) {
    movePly[depth+k].current=0;
    movePly[depth+k].last=0;
    movePly[depth+k].move[0].suit=startMoves[k-1].suit;
    movePly[depth+k].move[0].rank=startMoves[k-1].rank;
    if (k<noOfStartMoves) {     /* If there is more than one start move */
      if (WinningMove(&startMoves[k-1], &move)) {
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
    iniRemovedRanks[s]=posPoint->removedRanks[s];

  /* Initialize winning and second best ranks */
  for (s=0; s<=3; s++) {
    maxAgg=0;
    for (h=0; h<=3; h++) {
      aggHand[h][s]=startMovesBitMap[h][s] | game.suit[h][s];
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
    no[d]=0;
  }
  #endif

  if (!mtd) {
    lenSetSize=0;  
    for (k=0; k<=13; k++) { 
      for (h=0; h<=3; h++) {
	rootnp[k][h]=&posSearch[lenSetSize];
	posSearch[lenSetSize].suitLengths=0;
	posSearch[lenSetSize].posSearchPoint=NULL;
	posSearch[lenSetSize].left=NULL;
	posSearch[lenSetSize].right=NULL;
	lenSetSize++;
      }
    }
    nodeSetSize=0;
    winSetSize=0;
  }
  
  #ifdef TTDEBUG
  if (!suppressTTlog) 
    lastTTstore=0;
  #endif

  return;
}



int score1Counts[50], score0Counts[50];
int sumScore1Counts, sumScore0Counts;
int c1[50], c2[50], c3[50], c4[50], c5[50], c6[50], c7[50], c8[50], c9[50];
int sumc1, sumc2, sumc3, sumc4, sumc5, sumc6, sumc7, sumc8, sumc9;

int ABsearch(struct pos * posPoint, int target, int depth) {
    /* posPoint points to the current look-ahead position,
       target is number of tricks to take for the player,
       depth is the remaining search length, must be positive,
       the value of the subtree is returned.  */

  int moveExists, mexists, value, hand, scoreFlag, found;
  int ready, hfirst, hh, ss, rr, mcurrent, qtricks, tricks, res, k;
  unsigned short int makeWinRank[4];
  struct nodeCardsType * cardsP;
  struct evalType evalData;
  struct winCardType * np;
  struct posSearchType * pp;
  long long suitLengths;
  struct nodeCardsType  * tempP;
  unsigned short int aggr[4];
  unsigned short int ranks;

  struct evalType Evaluate(struct pos * posPoint);
  void Make(struct pos * posPoint, unsigned short int trickCards[4], 
    int depth);
  void Undo(struct pos * posPoint, int depth);

  /*cardsP=NULL;*/
  hand=handId(posPoint->first[depth], posPoint->handRelFirst);
  nodes++;
  if (posPoint->handRelFirst==0) {
    trickNodes++;
    if (posPoint->tricksMAX>=target) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

        #ifdef STAT
        c1[depth]++;
        
        score1Counts[depth]++;
        if (depth==iniDepth) {
          fprintf(fp2, "score statistics:\n");
          for (dd=iniDepth; dd>=0; dd--) {
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
    if (((posPoint->tricksMAX+(depth>>2)+1)<target)/*&&(depth>0)*/) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

        #ifdef STAT
        c2[depth]++;
        score0Counts[depth]++;
        if (depth==iniDepth) {
          fprintf(fp2, "score statistics:\n");
          for (dd=iniDepth; dd>=0; dd--) {
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
    	
    if (nodeTypeStore[hand]==MAXNODE) {
      qtricks=QuickTricks(posPoint, hand, depth, target, trump, &res);
      if (res) {
	if (qtricks==0)
	  return FALSE;
	else
          return TRUE;
	  #ifdef STAT
          c3[depth]++;
          score1Counts[depth]++;
          if (depth==iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                c3[dd], c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
          #endif
      }
      if (!LaterTricksMIN(posPoint,hand,depth,target, trump))
	return FALSE;
    }
    else {
      qtricks=QuickTricks(posPoint, hand, depth, target, trump, &res);
      if (res) {
        if (qtricks==0)
	  return TRUE;
	else
          return FALSE;
	  #ifdef STAT
          c4[depth]++;
          score0Counts[depth]++;
          if (depth==iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
              score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                c3[dd], c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
            }
          }
          #endif
      }
      if (LaterTricksMAX(posPoint,hand,depth,target, trump))
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
	
    if ((found)&&(depth!=iniDepth)) {
      for (k=0; k<=3; k++)
	posPoint->winRanks[depth][k]=0;
      if (rr!=0)
	posPoint->winRanks[depth][ss]=bitMapRank[rr];

      if (nodeTypeStore[hand]==MAXNODE) {
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
	    if ((k!=ss)&&(posPoint->length[hh][k]!=0))  {  /* Not lead suit, not void in suit */
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
    (depth!=iniDepth)) {  
    for (ss=0; ss<=3; ss++) {
      aggr[ss]=0;
      for (hh=0; hh<=3; hh++)
	aggr[ss]=aggr[ss] | posPoint->rankInSuit[hh][ss];
	  /* New algo */
      posPoint->orderSet[ss]=rel[aggr[ss]].aggrRanks[ss];
    }
    tricks=depth>>2;
    suitLengths=0; 
    for (ss=0; ss<=2; ss++)
      for (hh=0; hh<=3; hh++) {
	suitLengths=suitLengths<<4;
	suitLengths|=posPoint->length[hh][ss];
      }
    	
    pp=SearchLenAndInsert(rootnp[tricks][hand], suitLengths, FALSE, &res);
	/* Find node that fits the suit lengths */
    if (pp!=NULL) {
      np=pp->posSearchPoint;
      if (np==NULL)
        cardsP=NULL;
      else 
        cardsP=FindSOP(posPoint, np, hand, target, tricks, &scoreFlag);
      
      if ((cardsP!=NULL)/*&&(depth!=iniDepth)*/) {
        if (scoreFlag==1) {
	  for (ss=0; ss<=3; ss++)
	    posPoint->winRanks[depth][ss]=
	      adaptWins[aggr[ss]].winRanks[(int)cardsP->leastWin[ss]];
		    
          if (cardsP->bestMoveRank!=0) {
            bestMoveTT[depth].suit=cardsP->bestMoveSuit;
            bestMoveTT[depth].rank=cardsP->bestMoveRank;
          }
            #ifdef STAT
            c5[depth]++;
            if (scoreFlag==1)
              score1Counts[depth]++;
            else
              score0Counts[depth]++;
            if (depth==iniDepth) {
              fprintf(fp2, "score statistics:\n");
              for (dd=iniDepth; dd>=0; dd--) {
                fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
                score1Counts[dd], score0Counts[dd], c1[dd], c2[dd],
                  c3[dd], c4[dd]);
                fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                c6[dd], c7[dd], c8[dd]);
              }
            }
            #endif
          #ifdef TTDEBUG
          if (!suppressTTlog) { 
            if (lastTTstore<SEARCHSIZE) 
              ReceiveTTstore(posPoint, cardsP, target, depth);
            else 
              ttCollect=FALSE;
	      }
          #endif 
          return TRUE;
	}
        else {
	  for (ss=0; ss<=3; ss++)
	    posPoint->winRanks[depth][ss]=
	      adaptWins[aggr[ss]].winRanks[(int)cardsP->leastWin[ss]];

          if (cardsP->bestMoveRank!=0) {
            bestMoveTT[depth].suit=cardsP->bestMoveSuit;
            bestMoveTT[depth].rank=cardsP->bestMoveRank;
          }
	  #ifdef STAT
	  c6[depth]++;
          if (scoreFlag==1)
            score1Counts[depth]++;
          else
            score0Counts[depth]++;
          if (depth==iniDepth) {
            fprintf(fp2, "score statistics:\n");
            for (dd=iniDepth; dd>=0; dd--) {
              fprintf(fp2, "d=%d s1=%d s0=%d c1=%d c2=%d c3=%d c4=%d", dd,
                score1Counts[dd], score0Counts[dd], c1[dd], c2[dd], c3[dd],
                  c4[dd]);
              fprintf(fp2, " c5=%d c6=%d c7=%d c8=%d\n", c5[dd],
                  c6[dd], c7[dd], c8[dd]);
            }
          }
          #endif

          #ifdef TTDEBUG
          if (!suppressTTlog) {
            if (lastTTstore<SEARCHSIZE) 
              ReceiveTTstore(posPoint, cardsP, target, depth);
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
    evalData=Evaluate(posPoint);        /* Leaf node */
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
        if (depth==iniDepth) {
          fprintf(fp2, "score statistics:\n");
          for (dd=iniDepth; dd>=0; dd--) {
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
    moveExists=MoveGen(posPoint, depth);

    /*#if 0*/
    if ((posPoint->handRelFirst==3)&&(depth>=/*29*/33/*37*/)&&(depth!=iniDepth)) {
      movePly[depth].current=0;
      mexists=TRUE;
      ready=FALSE;
      while (mexists) {
      Make(posPoint, makeWinRank, depth);
      depth--;

      for (ss=0; ss<=3; ss++) {
	aggr[ss]=0;
	for (hh=0; hh<=3; hh++)
	  aggr[ss]|=posPoint->rankInSuit[hh][ss];
	  posPoint->orderSet[ss]=rel[aggr[ss]].aggrRanks[ss];
	}
	tricks=depth>>2;
	hfirst=posPoint->first[depth];
	suitLengths=0;
	for (ss=0; ss<=2; ss++)
	  for (hh=0; hh<=3; hh++) {
            suitLengths=suitLengths<<4;
	    suitLengths|=posPoint->length[hh][ss];
	  }

	  pp=SearchLenAndInsert(rootnp[tricks][hfirst], suitLengths, FALSE, &res);
	    /* Find node that fits the suit lengths */
	  if (pp!=NULL) {
	  np=pp->posSearchPoint;
	  if (np==NULL)
	    tempP=NULL;
	  else
	    tempP=FindSOP(posPoint, np, hfirst, target, tricks, &scoreFlag);

	  if (tempP!=NULL) {
	    if ((nodeTypeStore[hand]==MAXNODE)&&(scoreFlag==1)) {
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]=
			  adaptWins[aggr[ss]].winRanks[(int)tempP->leastWin[ss]];
	      if (tempP->bestMoveRank!=0) {
		bestMoveTT[depth+1].suit=tempP->bestMoveSuit;
		bestMoveTT[depth+1].rank=tempP->bestMoveRank;
	      }
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]|=makeWinRank[ss];
	      Undo(posPoint, depth+1);
	      return TRUE;
	    }
	    else if ((nodeTypeStore[hand]==MINNODE)&&(scoreFlag==0)) {
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]=
		  adaptWins[aggr[ss]].winRanks[(int)tempP->leastWin[ss]];
	      if (tempP->bestMoveRank!=0) {
		bestMoveTT[depth+1].suit=tempP->bestMoveSuit;
		bestMoveTT[depth+1].rank=tempP->bestMoveRank;
	      }
	      for (ss=0; ss<=3; ss++)
		posPoint->winRanks[depth+1][ss]|=makeWinRank[ss];
	      Undo(posPoint, depth+1);
	      return FALSE;
	    }
	    else {
	      movePly[depth+1].move[movePly[depth+1].current].weight+=100;
	      ready=TRUE;
	    }
	  }
	}
	depth++;
        Undo(posPoint, depth);
	if (ready)
	  break;
	if (movePly[depth].current<=movePly[depth].last-1) {
	  movePly[depth].current++;
	  mexists=TRUE;
	}
	else
	  mexists=FALSE;
      }
      if (ready)
	MergeSort(movePly[depth].last+1, movePly[depth].move);
    }
    /*#endif*/

    movePly[depth].current=0;
    if (nodeTypeStore[hand]==MAXNODE) {
      value=FALSE;
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;

      while (moveExists)  {
        Make(posPoint, makeWinRank, depth);        /* Make current move */

        value=ABsearch(posPoint, target, depth-1);
          
        Undo(posPoint, depth);      /* Retract current move */
        if (value==TRUE) {
        /* A cut-off? */
	    for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth-1][ss] |
              makeWinRank[ss];
	    mcurrent=movePly[depth].current;
          bestMove[depth]=movePly[depth].move[mcurrent];
          goto ABexit;
        }  
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth][ss] |
           posPoint->winRanks[depth-1][ss] | makeWinRank[ss];

        moveExists=NextMove(posPoint, depth);
      }
    }
    else {                          /* A minnode */
      value=TRUE;
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
        
      while (moveExists)  {
        Make(posPoint, makeWinRank, depth);        /* Make current move */
        
        value=ABsearch(posPoint, target, depth-1);

        Undo(posPoint, depth);       /* Retract current move */
        if (value==FALSE) {
        /* A cut-off? */
	    for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth-1][ss] |
              makeWinRank[ss];
	    mcurrent=movePly[depth].current;
          bestMove[depth]=movePly[depth].move[mcurrent];
          goto ABexit;
        }
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=posPoint->winRanks[depth][ss] |
           posPoint->winRanks[depth-1][ss] | makeWinRank[ss];

        moveExists=NextMove(posPoint, depth);
      }
    }
  }
  ABexit:
  if (depth>=4) {
    if(posPoint->handRelFirst==0) { 
      tricks=depth>>2;
      /*hand=posPoint->first[depth-1];*/
      if (value)
	k=target;
      else
	k=target-1;
      if (depth!=iniDepth)
        BuildSOP(posPoint, suitLengths, tricks, hand, target, depth,
        value, k);
      if (clearTTflag) {
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

        Wipe();
	winSetSizeLimit=WINIT;
	nodeSetSizeLimit=NINIT;
	lenSetSizeLimit=LINIT;
	lcount=0;  
	allocmem=(lenSetSizeLimit+1)*sizeof(struct posSearchType);
	lenSetSize=0;
	posSearch=pl[lcount];  
	for (k=0; k<=13; k++) { 
	  for (hh=0; hh<=3; hh++) {
	    rootnp[k][hh]=&posSearch[lenSetSize];
	    posSearch[lenSetSize].suitLengths=0;
	    posSearch[lenSetSize].posSearchPoint=NULL;
	    posSearch[lenSetSize].left=NULL;
	    posSearch[lenSetSize].right=NULL;
	    lenSetSize++;
	  }
	}
        nodeSetSize=0;
        winSetSize=0;
	wcount=0; ncount=0; 
	allocmem+=(winSetSizeLimit+1)*sizeof(struct winCardType);
        winCards=pw[wcount];
	allocmem+=(nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
	nodeCards=pn[ncount];
	clearTTflag=FALSE;
	windex=-1;
      }
    } 
  }
    #ifdef STAT
    c8[depth]++;
    if (value==1)
      score1Counts[depth]++;
    else
      score0Counts[depth]++;
    if (depth==iniDepth) {
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
      for (dd=iniDepth; dd>=0; dd--) {
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
      fprintf(fp2, "nodeSetSize=%d  winSetSize=%d\n", nodeSetSize,
        winSetSize);
      fprintf(fp2, "sumc1=%d sumc2=%d sumc3=%d sumc4=%d\n",
        sumc1, sumc2, sumc3, sumc4);
      fprintf(fp2, "sumc5=%d sumc6=%d sumc7=%d sumc8=%d sumc9=%d\n",
        sumc5, sumc6, sumc7, sumc8, sumc9);
	  fprintf(fp2, "\n");	
      fprintf(fp2, "\n");
      fprintf(fp2, "No of searched nodes per depth:\n");
      for (dd=iniDepth; dd>=0; dd--)
        fprintf(fp2, "depth=%d  nodes=%d\n", dd, no[dd]);
	  fprintf(fp2, "\n");
      fprintf(fp2, "Total nodes=%d\n", nodes);
    }
    #endif
    
  return value;
}

void Make(struct pos * posPoint, unsigned short int trickCards[4], 
  int depth)  {
  int r, s, t, u, w, firstHand;
  int suit, count, mcurr, h, q, done;
  struct moveType mo1, mo2;
  unsigned short aggr;

  for (suit=0; suit<=3; suit++)
    trickCards[suit]=0;

  firstHand=posPoint->first[depth];
  r=movePly[depth].current;

  if (posPoint->handRelFirst==3)  {         /* This hand is last hand */
    mo1=movePly[depth].move[r];
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
    for (h=0; h<=3; h++) {
      mcurr=movePly[depth+h].current;
      if (movePly[depth+h].move[mcurr].suit==suit)
        count++;
    }

    if (nodeTypeStore[posPoint->high[depth]]==MAXNODE)
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
      r=movePly[depth+s].current;
      w=movePly[depth+s].move[r].rank;
      u=movePly[depth+s].move[r].suit;
      posPoint->removedRanks[u]|=bitMapRank[w];

      if (s==0)
        posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);

      if ((w==posPoint->winner[u].rank)||(w==posPoint->secondBest[u].rank)) {
	aggr=0;
        for (h=0; h<=3; h++)
	  aggr|=posPoint->rankInSuit[h][u];
        posPoint->winner[u].rank=rel[aggr].absRank[1][u].rank;
        posPoint->winner[u].hand=rel[aggr].absRank[1][u].hand;
        posPoint->secondBest[u].rank=rel[aggr].absRank[2][u].rank;
        posPoint->secondBest[u].hand=rel[aggr].absRank[2][u].hand;
      }  

    /* Determine win-ranked cards */
      if ((q==posPoint->high[depth])&&(!done)) {
        done=TRUE;
        if (count>=2) {
          trickCards[u]=bitMapRank[w];
          /* Mark ranks as winning if they are part of a sequence */
          trickCards[u]|=movePly[depth+s].move[r].sequence; 
        }
      }
    }
  }
  else if (posPoint->handRelFirst==0) {   /* Is it the 1st hand? */
    posPoint->first[depth-1]=firstHand;   /* First hand is not changed in
                                            next move */
    posPoint->high[depth]=firstHand;
    posPoint->move[depth]=movePly[depth].move[r];
    t=firstHand;
    posPoint->handRelFirst=1;
    r=movePly[depth].current;
    u=movePly[depth].move[r].suit;
    w=movePly[depth].move[r].rank;
    posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);
  }
  else {
    mo1=movePly[depth].move[r];
    mo2=posPoint->move[depth+1];
    r=movePly[depth].current;
    u=movePly[depth].move[r].suit;
    w=movePly[depth].move[r].rank;
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
    posPoint->first[depth-1]=firstHand;     /* First hand is not changed in
                                            next move */
    
    posPoint->rankInSuit[t][u]&=(~bitMapRank[w]);
  }

  posPoint->length[t][u]--;

#ifdef STAT
  no[depth]++;
#endif
    
  return;
}


void Undo(struct pos * posPoint, int depth)  {
  int r, s, t, u, w, firstHand;

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
    r=movePly[depth].current;
    u=movePly[depth].move[r].suit;
    w=movePly[depth].move[r].rank;

  }
  else if (posPoint->handRelFirst==3)  {    /* Last hand */
    for (s=3; s>=0; s--) {
    /* Delete the moves from removed ranks */
      r=movePly[depth+s].current;
      w=movePly[depth+s].move[r].rank;
      u=movePly[depth+s].move[r].suit;
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

        
    if (nodeTypeStore[posPoint->first[depth-1]]==MAXNODE)   /* First hand
                                            of next trick is winner of the
                                            current trick */
      posPoint->tricksMAX--;
  }
  else {
    t=handId(firstHand, posPoint->handRelFirst);
    r=movePly[depth].current;
    u=movePly[depth].move[r].suit;
    w=movePly[depth].move[r].rank;
	/*posPoint->removedRanks[u]&= (~bitMapRank[w]);*/
  }    

  posPoint->rankInSuit[t][u]|=bitMapRank[w];

  posPoint->length[t][u]++;

  return;
}



struct evalType Evaluate(struct pos * posPoint)  {
  int s, smax=0, max, k, firstHand, count;
  struct evalType eval;

  firstHand=posPoint->first[0];

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

      if (nodeTypeStore[smax]==MAXNODE)
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

  if (nodeTypeStore[smax]==MAXNODE)
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
	int depth, int target, int trump, int *result) {
  int suit, sum, qtricks, commPartner, commRank=0, commSuit=-1, s, found=FALSE;
  int opps, res;
  int countLho, countRho, countPart, countOwn, lhoTrumpRanks, rhoTrumpRanks;
  int cutoff, hh, ss, rr, lowestQtricks=0, count=0/*, ruff=FALSE*/;

  int QtricksLeadHandNT(int hand, struct pos *posPoint, int cutoff, int depth, 
	int countLho, int countRho, int *lhoTrumpRanks, int *rhoTrumpRanks, int commPartner,
	int commSuit, int countOwn, int countPart, int suit, int qtricks, int *res);
  
  int QtricksLeadHandTrump(int hand, struct pos *posPoint, int cutoff, int depth, 
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int *res);

  int QuickTricksPartnerHandTrump(int hand, struct pos *posPoint, int cutoff, int depth, 
	int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int *res);

  int QuickTricksPartnerHandNT(int hand, struct pos *posPoint, int cutoff, int depth, 
	int countLho, int countRho, int countOwn,
	int countPart, int suit, int qtricks, int commSuit, int commRank, int *res);

  
  *result=TRUE;
  qtricks=0;
  for (s=0; s<=3; s++)   
    posPoint->winRanks[depth][s]=0;

  if ((depth<=0)||(depth==iniDepth)) {
    *result=FALSE;
    return qtricks;
  }

  if (nodeTypeStore[hand]==MAXNODE) 
    cutoff=target-posPoint->tricksMAX;
  else
    cutoff=posPoint->tricksMAX-target+(depth>>2)+2;
      
  commPartner=FALSE;
  for (s=0; s<=3; s++) {
    if ((trump!=4)&&(trump!=s)) {
     /*if ((posPoint->rankInSuit[hand][s]!=0)&&((posPoint->rankInSuit[lho[hand]][s]!=0)||
		 (posPoint->rankInSuit[lho[hand]][trump]==0))&&
		 ((posPoint->rankInSuit[rho[hand]][s]!=0)||(posPoint->rankInSuit[rho[hand]][trump]==0))&&
		 (posPoint->rankInSuit[partner[hand]][s]==0)&&(posPoint->rankInSuit[partner[hand]][trump]>0)
		 &&((posPoint->winner[s].hand==lho[hand])||(posPoint->winner[s].hand==rho[hand]))&&
		 ((posPoint->winner[trump].hand==lho[hand])||(posPoint->winner[trump].hand==rho[hand]))) {
		commPartner=TRUE;
        	commSuit=s;
        	commRank=0;
		ruff=TRUE;
        	break;  
	  }
      else*/ if (posPoint->winner[s].hand==partner[hand]) {
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

    /*if ((ruff)&&(suit==commSuit)) {
      qtricks++;
      if (qtricks>=cutoff)
	return qtricks;
      suit++;
      if ((trump!=4) && (suit==trump))
        suit++;
      continue;
    }
    else*/ if (!opps && (countPart==0)) {
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
	       countPart, suit, qtricks, &res);
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
          countPart, suit, qtricks, &res);
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
      if ((posPoint->winner[suit].hand==partner[hand])&&(countPart>0)) {
        /* Winner found at partner*/
        if (commPartner) {
        /* There is communication with the partner */
          if ((trump!=4)&&(trump!=suit)) {
	    qtricks=QuickTricksPartnerHandTrump(hand, posPoint, cutoff, depth, 
	      countLho, countRho, lhoTrumpRanks, rhoTrumpRanks, countOwn,
	      countPart, suit, qtricks, commSuit, commRank, &res);
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
	      countPart, suit, qtricks, commSuit, commRank, &res);
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
		posPoint->winRanks[depth][trump]=
		  posPoint->winRanks[depth][trump] | bitMapRank[rr];
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
		posPoint->winRanks[depth][trump]=
		  posPoint->winRanks[depth][trump] | bitMapRank[rr];
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
    if ((trump==4)||(posPoint->winner[trump].rank==0)) {
      found=FALSE;
      for (ss=0; ss<=3; ss++) {
        if (posPoint->winner[ss].rank==0)
	  continue;
        hh=posPoint->winner[ss].hand;
	if (nodeTypeStore[hh]==nodeTypeStore[hand]) {
          if (posPoint->length[hand][ss]>0) {
            found=TRUE;   
            break;
          }
        }
	else if ((posPoint->length[partner[hand]][ss]>0)&&
	      (posPoint->length[hand][ss]>0)&&
	      (posPoint->length[partner[hh]][ss]>0)) { 
          count++;
	}
      }
      if (!found) {
	if (nodeTypeStore[hand]==MAXNODE) {
          if ((posPoint->tricksMAX+(depth>>2)-Max(0,count-1))<target) {
            for (ss=0; ss<=3; ss++) {
	      if (posPoint->winner[ss].hand==-1)
	        posPoint->winRanks[depth][ss]=0;
              else if ((posPoint->length[hand][ss]>0)
                 &&(nodeTypeStore[posPoint->winner[ss].hand]==MINNODE))
                posPoint->winRanks[depth][ss]=
		        bitMapRank[posPoint->winner[ss].rank];
              else
                posPoint->winRanks[depth][ss]=0;
	    }
	    return 0;
	  }
        }
	else {
	  if ((posPoint->tricksMAX+1+Max(0,count-1))>=target) {
          for (ss=0; ss<=3; ss++) {
	    if (posPoint->winner[ss].hand==-1)
	      posPoint->winRanks[depth][ss]=0;
            else if ((posPoint->length[hand][ss]>0) 
               &&(nodeTypeStore[posPoint->winner[ss].hand]==MAXNODE))
              posPoint->winRanks[depth][ss]=
		           bitMapRank[posPoint->winner[ss].rank];
              else
                posPoint->winRanks[depth][ss]=0;
	    }
	    return 0;
	  }
        }
      }
    }
  }

  *result=FALSE;
  return qtricks;
}


int QtricksLeadHandTrump(int hand, struct pos *posPoint, int cutoff, int depth, 
      int countLho, int countRho, int lhoTrumpRanks, int rhoTrumpRanks, int countOwn,
      int countPart, int suit, int qtricks, int *res) {
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
	int countPart, int suit, int qtricks, int *res) {
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
	int countPart, int suit, int qtricks, int commSuit, int commRank, int *res) {
	/* res=0		Continue with same suit. 
	   res=1		Cutoff.
	   res=2		Continue with next suit. */

  int qt, k/*, found, rr*/;
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
    if (rel[ranks].absRank[3][suit].hand==partner[hand]) {
      /*posPoint->winRanks[depth][suit]|=bitMapRank[rr];*/
	  posPoint->winRanks[depth][suit]|=bitMapRank[rel[ranks].absRank[3][suit].rank];
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
	int countPart, int suit, int qtricks, int commSuit, int commRank, int *res) {

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

  if ((posPoint->secondBest[suit].hand==partner[hand])&&(countPart>0)) {
       /* Second best found in partners hand */
    posPoint->winRanks[depth][suit]|=bitMapRank[posPoint->secondBest[suit].rank];
    /*posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];*/
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
    /*posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];*/
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
      ranks|=posPoint->rankInSuit[k][suit];
    if (rel[ranks].absRank[3][suit].hand==partner[hand]) {
      posPoint->winRanks[depth][suit]|=bitMapRank[rel[ranks].absRank[3][suit].rank];
      /*posPoint->winRanks[depth][suit]=posPoint->winRanks[depth][suit] | bitMapRank[rr];*/
      /*posPoint->winRanks[depth][commSuit]|=bitMapRank[commRank];*/
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
	int trump) {
  int hh, ss, k, h, sum=0;
  unsigned short aggr;

  if ((trump==4)||(posPoint->winner[trump].rank==0)) {
    for (ss=0; ss<=3; ss++) {
      hh=posPoint->winner[ss].hand;
      if (hh!=-1) {
        if (nodeTypeStore[hh]==MAXNODE)
          sum+=Max(posPoint->length[hh][ss], posPoint->length[partner[hh]][ss]);
      }
    }
    if ((posPoint->tricksMAX+sum<target)&&
      (sum>0)&&(depth>0)&&(depth!=iniDepth)) {
      if ((posPoint->tricksMAX+(depth>>2)<target)) {
	for (ss=0; ss<=3; ss++) {
	  if (posPoint->winner[ss].hand==-1)
	    posPoint->winRanks[depth][ss]=0;
          else if (nodeTypeStore[posPoint->winner[ss].hand]==MINNODE) {  
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
    (nodeTypeStore[posPoint->winner[trump].hand]==MINNODE)) {
    if ((posPoint->length[hand][trump]==0)&&
      (posPoint->length[partner[hand]][trump]==0)) {
      if (((posPoint->tricksMAX+(depth>>2)+1-
	  Max(posPoint->length[lho[hand]][trump],
	  posPoint->length[rho[hand]][trump]))<target)
          &&(depth>0)&&(depth!=iniDepth)) {
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=0;
	return FALSE;
      }
    }    
    else if (((posPoint->tricksMAX+(depth>>2))<target)&&
      (depth>0)&&(depth!=iniDepth)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
      posPoint->winRanks[depth][trump]=
	  bitMapRank[posPoint->winner[trump].rank];
      return FALSE;
    }
    else {
      hh=posPoint->secondBest[trump].hand;
      if (hh!=-1) {
        if ((nodeTypeStore[hh]==MINNODE)&&(posPoint->secondBest[trump].rank!=0))  {
          if (((posPoint->length[hh][trump]>1) ||
            (posPoint->length[partner[hh]][trump]>1))&&
            ((posPoint->tricksMAX+(depth>>2)-1)<target)&&(depth>0)
	     &&(depth!=iniDepth)) {
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
      if ((nodeTypeStore[hh]==MINNODE)&&
        (posPoint->length[hh][trump]>1)) {
	if (posPoint->winner[trump].hand==rho[hh]) {
          if (((posPoint->tricksMAX+(depth>>2))<target)&&
            (depth>0)&&(depth!=iniDepth)) {
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
	  h=rel[aggr].absRank[3][trump].hand;
	  if (h!=-1) {
	    if ((nodeTypeStore[h]==MINNODE)&&
	      ((posPoint->tricksMAX+(depth>>2))<target)&&
              (depth>0)&&(depth!=iniDepth)) {
              for (ss=0; ss<=3; ss++)
                posPoint->winRanks[depth][ss]=0;
	      posPoint->winRanks[depth][trump]=
		bitMapRank[rel[aggr].absRank[3][trump].rank]; 
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
	int trump) {
  int hh, ss, k, h, sum=0;
  unsigned short aggr;
	
  if ((trump==4)||(posPoint->winner[trump].rank==0)) {
    for (ss=0; ss<=3; ss++) {
      hh=posPoint->winner[ss].hand;
      if (hh!=-1) {
        if (nodeTypeStore[hh]==MINNODE)
          sum+=Max(posPoint->length[hh][ss], posPoint->length[partner[hh]][ss]);
      }
    }
    if ((posPoint->tricksMAX+(depth>>2)+1-sum>=target)&&
	(sum>0)&&(depth>0)&&(depth!=iniDepth)) {
      if ((posPoint->tricksMAX+1>=target)) {
	for (ss=0; ss<=3; ss++) {
	  if (posPoint->winner[ss].hand==-1)
	    posPoint->winRanks[depth][ss]=0;
          else if (nodeTypeStore[posPoint->winner[ss].hand]==MAXNODE) {  
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
    (nodeTypeStore[posPoint->winner[trump].hand]==MAXNODE)) {
    if ((posPoint->length[hand][trump]==0)&&
      (posPoint->length[partner[hand]][trump]==0)) {
      if (((posPoint->tricksMAX+Max(posPoint->length[lho[hand]][trump],
        posPoint->length[rho[hand]][trump]))>=target)
        &&(depth>0)&&(depth!=iniDepth)) {
        for (ss=0; ss<=3; ss++)
          posPoint->winRanks[depth][ss]=0;
	return TRUE;
      }
    }    
    else if (((posPoint->tricksMAX+1)>=target)
      &&(depth>0)&&(depth!=iniDepth)) {
      for (ss=0; ss<=3; ss++)
        posPoint->winRanks[depth][ss]=0;
      posPoint->winRanks[depth][trump]=
	  bitMapRank[posPoint->winner[trump].rank];
      return TRUE;
    }
    else {
      hh=posPoint->secondBest[trump].hand;
      if (hh!=-1) {
        if ((nodeTypeStore[hh]==MAXNODE)&&(posPoint->secondBest[trump].rank!=0))  {
          if (((posPoint->length[hh][trump]>1) ||
            (posPoint->length[partner[hh]][trump]>1))&&
            ((posPoint->tricksMAX+2)>=target)&&(depth>0)
	    &&(depth!=iniDepth)) {
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
      if ((nodeTypeStore[hh]==MAXNODE)&&
        (posPoint->length[hh][trump]>1)) {
	if (posPoint->winner[trump].hand==rho[hh]) {
          if (((posPoint->tricksMAX+1)>=target)&&(depth>0)
	     &&(depth!=iniDepth)) {
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
	  h=rel[aggr].absRank[3][trump].hand;
	  if (h!=-1) {
	    if ((nodeTypeStore[h]==MAXNODE)&&
		((posPoint->tricksMAX+1)>=target)&&(depth>0)
		&&(depth!=iniDepth)) {
              for (ss=0; ss<=3; ss++)
                posPoint->winRanks[depth][ss]=0;
	      posPoint->winRanks[depth][trump]=
		bitMapRank[rel[aggr].absRank[3][trump].rank]; 
              return TRUE;
	    }
	  }
	}
      }
    }
  }
  return FALSE;
}


int MoveGen(struct pos * posPoint, int depth) {
  int suit, k, m, r, s, t, state;
  unsigned short ris;
  int q, first;
  int WeightAllocTrump(struct pos *, struct moveType * mp, int depth,
    unsigned short notVoidInSuit, int trump);
  int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
  unsigned short notVoidInSuit);

  for (k=0; k<4; k++) 
    lowestWin[depth][k]=0;
  
  m=0;
  r=posPoint->handRelFirst;
  assert((r>=0)&&(r<=3));
  first=posPoint->first[depth];
  q=handId(first, r);
  
  s=movePly[depth+r].current;             /* Current move of first hand */
  t=movePly[depth+r].move[s].suit;        /* Suit played by first hand */
  ris=posPoint->rankInSuit[q][t];

  if ((r!=0)&&(ris!=0)) {
  /* Not first hand and not void in suit */
    k=14;   state=MOVESVALID;
    while (k>=2) {
      if ((ris & bitMapRank[k])&&(state==MOVESVALID)) {
           /* Only first move in sequence is generated */
        movePly[depth].move[m].suit=t;
        movePly[depth].move[m].rank=k;
        movePly[depth].move[m].sequence=0;
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
            movePly[depth].move[m-1].sequence|=bitMapRank[k];
        }
      k--;
    }
    if (m!=1) {
      if (trump!=4) {
        for (k=0; k<=m-1; k++) 
          movePly[depth].move[k].weight=WeightAllocTrump(posPoint,
            &movePly[depth].move[k], depth, ris, trump);
      }
      else {
	for (k=0; k<=m-1; k++) 
          movePly[depth].move[k].weight=WeightAllocNT(posPoint,
            &movePly[depth].move[k], depth, ris);
      }
    }

    movePly[depth].last=m-1;
    if (m!=1)
      MergeSort(m, movePly[depth].move);
    if (depth!=iniDepth)
      return m;
    else {
      m=AdjustMoveList();
      return m;
    }
  }
  else {                  /* First hand or void in suit */
    for (suit=0; suit<=3; suit++)  {
      k=14;  state=MOVESVALID; 
      while (k>=2) {
        if ((posPoint->rankInSuit[q][suit] & bitMapRank[k])&&
            (state==MOVESVALID)) {
           /* Only first move in sequence is generated */
          movePly[depth].move[m].suit=suit;
          movePly[depth].move[m].rank=k;
          movePly[depth].move[m].sequence=0;
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
              movePly[depth].move[m-1].sequence|=bitMapRank[k];
          }
        k--;
      }
    }

    if (trump!=4) {
      for (k=0; k<=m-1; k++) { 
        movePly[depth].move[k].weight=WeightAllocTrump(posPoint,
          &movePly[depth].move[k], depth, ris, trump);
      }
    }
    else {
      for (k=0; k<=m-1; k++) { 
        movePly[depth].move[k].weight=WeightAllocNT(posPoint,
          &movePly[depth].move[k], depth, ris);
      }
    }
  
    movePly[depth].last=m-1;
    if (m!=1)
      MergeSort(m, movePly[depth].move);
    if (depth!=iniDepth)
     return m;
    else {
      m=AdjustMoveList();
      return m;
    }  
  }
}



int WeightAllocNT(struct pos * posPoint, struct moveType * mp, int depth,
  unsigned short notVoidInSuit) {
  int weight=0, k, l, kk, ll, suit, suitAdd=0, leadSuit;
  int suitWeightDelta, first, q;
  int rRank;
  int suitBonus=0;
  int winMove=FALSE;
  unsigned short suitCount, suitCountLH, suitCountRH, aggr;
  int countLH, countRH;

  first=posPoint->first[depth];
  q=handId(first, posPoint->handRelFirst);
  suit=mp->suit;
  aggr=0;
  for (k=0; k<=3; k++)
    aggr|=posPoint->rankInSuit[k][suit];
  rRank=rel[aggr].relRank[mp->rank][suit];

  switch (posPoint->handRelFirst) {
    case 0:
      suitCount=posPoint->length[q][suit];
      suitCountLH=posPoint->length[lho[q]][suit];
      suitCountRH=posPoint->length[rho[q]][suit];

      if ((posPoint->winner[suit].hand==rho[q])||
          (posPoint->secondBest[suit].hand==rho[q])) {
	if (suitCountRH!=1)
	  suitBonus-=7/*6*//*12*//*17*/;
      }
      else if ((posPoint->winner[suit].hand==lho[q])&&
	(posPoint->secondBest[suit].hand==partner[q])) {
	/* This case was suggested by Joël Bradmetz. */
	if (posPoint->length[partner[q]][suit]!=1)
	  suitBonus+=34/*37*//*32*//*20*/;
      }
      else if ((posPoint->secondBest[suit].hand==lho[q])&&
	((posPoint->winner[suit].hand==partner[q])||
	(posPoint->winner[suit].hand==q)))
	suitBonus+=14;

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

      suitWeightDelta=suitBonus-((countLH+countRH)<<5)/(19/*20*//*15*/);	
	  
      if (posPoint->winner[suit].rank==mp->rank) 
        winMove=TRUE;			   
      else if (posPoint->rankInSuit[partner[first]][suit] >
	    (posPoint->rankInSuit[lho[first]][suit] |
	     posPoint->rankInSuit[rho[first]][suit])) {
	    winMove=TRUE;
      }			
              
      if (winMove) {
	if (((posPoint->secondBest[suit].hand!=lho[first])
	  ||(suitCountLH==1))&&
	  ((posPoint->secondBest[suit].hand!=rho[first])
	  ||(suitCountRH==1)))
	  weight=suitWeightDelta+48+rRank;
	/*else 
	  weight=suitWeightDelta+35+rRank;*/

	else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank))			
          weight=suitWeightDelta+39;
      /*else if (mp->sequence)
	  weight=suitWeightDelta+25+rRank;*/
        else
          weight=suitWeightDelta+20+rRank;

	if ((bestMove[depth].suit==suit)&&
          (bestMove[depth].rank==mp->rank)) 
          weight+=123/*122*//*112*//*73*/;
	else if ((bestMoveTT[depth].suit==suit)&&
          (bestMoveTT[depth].rank==mp->rank)) 
          weight+=20/*17*//*14*/;
      }
      else {
	if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+25/*23*//*22*/+rRank;
        else if (posPoint->winner[suit].hand==first) {
          weight=suitWeightDelta-24/*27*//*12*//*10*/+rRank;
        }
        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank)) 
	  weight=suitWeightDelta+42;
	else if (mp->sequence)
          weight=suitWeightDelta+32-rRank;
        else 
          weight=suitWeightDelta+12+rRank; 
	
	if ((bestMove[depth].suit==suit)&&
            (bestMove[depth].rank==mp->rank)) 
          weight+=47/*45*//*39*//*38*/;
	else if ((bestMoveTT[depth].suit==suit)&&
            (bestMoveTT[depth].rank==mp->rank)) 
          weight+=15/*16*//*19*//*14*/;
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
	 /* Side with
	    highest rank in leadSuit wins */
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
	    if (suitCount==2)
	      suitAdd-=3;
	  }
	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd-=3;
 
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
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=7;				
	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd-=10;

          weight=-(mp->rank)+suitAdd; 
        }
        else {
	  kk=posPoint->rankInSuit[partner[first]][leadSuit];
          ll=posPoint->rankInSuit[rho[first]][leadSuit];
          k=kk & (-kk); l=ll & (-ll);  /* Only least significant 1 bit */
	  if ((k > bitMapRank[mp->rank])||
            (l > bitMapRank[mp->rank])) 
	      weight=-3+rRank;
          /* If lowest rank for either partner to leading hand 
	     or rho is higher than played card for lho,
	     lho should play as low card as possible */			
          else if (mp->rank > posPoint->move[depth+1].rank) {
	    /*if ((mp->sequence)&&
              (mp->rank==posPoint->secondBest[suit].rank))
		weight=15+rRank;
            else */if (mp->sequence) { 
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
      if (WinningMove(mp, &(posPoint->move[depth+1]))) {
	if (suit==leadSuit) {
	  if (bitMapRank[mp->rank] >
	    posPoint->rankInSuit[rho[first]][suit])
	    winMove=TRUE;
	}
      }	
      else if (posPoint->high[depth+1]==first) {
        if (posPoint->length[rho[first]][leadSuit]!=0) {
	  if (posPoint->rankInSuit[rho[first]][leadSuit]
	      < bitMapRank[posPoint->move[depth+2].rank])	
	    winMove=TRUE;
	}
	else
	  winMove=TRUE;
      }
      
      if (winMove) {
        if (!notVoidInSuit) {
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(17/*27*//*30*//*35*/);

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
	  else if ((suitCount==1)&&(posPoint->winner[suit].hand==q)) 
	    suitAdd-=(4/*5*/);	

          weight=-(mp->rank)+suitAdd;   /* Insensitive */

        }
        else {
          /*weight=20-(mp->rank);*/
		  
	  k=posPoint->rankInSuit[rho[first]][suit];
	  if ((k & (-k)) > bitMapRank[mp->rank])
	    weight=-(mp->rank);
          else if (WinningMove(mp, &(posPoint->move[depth+1]))) {
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
      else if (WinningMove(mp, &(posPoint->move[depth+1])))
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
  unsigned short notVoidInSuit, int trump) {
  int weight=0, k, l, kk, ll, suit, suitAdd=0, leadSuit;
  int suitWeightDelta, first, q, rRank;
  int suitBonus=0;
  int winMove=FALSE;
  unsigned short suitCount, suitCountLH, suitCountRH, aggr;
  int countLH, countRH;

  first=posPoint->first[depth];
  q=handId(first, posPoint->handRelFirst);
  suit=mp->suit;
  aggr=0;
  for (k=0; k<=3; k++)
    aggr|=posPoint->rankInSuit[k][suit];
  rRank=rel[aggr].relRank[mp->rank][suit];

  switch (posPoint->handRelFirst) {
    case 0:
      suitCount=posPoint->length[q][suit];
      suitCountLH=posPoint->length[lho[q]][suit];
      suitCountRH=posPoint->length[rho[q]][suit];

      if ((suit!=trump) &&
          (((posPoint->rankInSuit[lho[q]][suit]==0) &&
          (posPoint->rankInSuit[lho[q]][trump]!=0)) ||
          ((posPoint->rankInSuit[rho[q]][suit]==0) &&
          (posPoint->rankInSuit[rho[q]][trump]!=0))))
        suitBonus=-17/*20*//*-10*/;

	if ((suit!=trump)&&(posPoint->length[partner[q]][suit]==0)&&
	   (posPoint->length[partner[q]][trump]>0)&&(suitCountRH>0))
	  suitBonus+=26/*28*/;

      if ((posPoint->winner[suit].hand==rho[q])||
          (posPoint->secondBest[suit].hand==rho[q])) {
	if (suitCountRH!=1)
	  suitBonus-=11/*12*//*13*//*18*/;
      }
      else if ((posPoint->winner[suit].hand==lho[q])&&
	(posPoint->secondBest[suit].hand==partner[q])) {
	/* This case was suggested by Joël Bradmetz. */
	if (posPoint->length[partner[q]][suit]!=1) 
	  suitBonus+=30/*28*//*22*/;
      }
      /*else if ((posPoint->secondBest[suit].hand==lho[q])&&
		(posPoint->winner[suit].hand==partner[q]))
	   suitBonus+=10;*/

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
	if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+39+rRank;
        else if (posPoint->winner[suit].hand==first) {
          if (posPoint->secondBest[suit].hand==partner[first])
            weight=suitWeightDelta+46+rRank;
	  else if (posPoint->winner[suit].rank==mp->rank) 
            weight=suitWeightDelta+31;
          else
            weight=suitWeightDelta-2+rRank;
        }
        else if (posPoint->winner[suit].hand==partner[first]) {
          /* If partner has winning card */
          if (posPoint->secondBest[suit].hand==first)
            weight=suitWeightDelta+35/*35*//*46*//*50*/+rRank;
          else 
            weight=suitWeightDelta+24/*35*/+rRank;  
        } 
        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank))			
          weight=suitWeightDelta+41;
	else if (mp->sequence)
	  weight=suitWeightDelta+17+rRank;
        else
          weight=suitWeightDelta+11+rRank;

	if ((bestMove[depth].suit==suit)&&
            (bestMove[depth].rank==mp->rank)) 
          weight+=53/*50*//*52*/;
	else if ((bestMoveTT[depth].suit==suit)&&
            (bestMoveTT[depth].rank==mp->rank)) 
          weight+=14/*15*//*12*//*11*/;
      }
      else {
        if (((suitCountLH==1)&&(posPoint->winner[suit].hand==lho[first]))
            ||((suitCountRH==1)&&(posPoint->winner[suit].hand==rho[first])))
          weight=suitWeightDelta+rRank-2;
        else if (posPoint->winner[suit].hand==first) {
          if ((posPoint->secondBest[suit].rank!=0)&&
	     (posPoint->secondBest[suit].hand==partner[first]))
            weight=suitWeightDelta+33+rRank;
          else if (posPoint->winner[suit].rank==mp->rank) 
            weight=suitWeightDelta+36;
          else
            weight=suitWeightDelta-17+rRank;
        }
        else if (posPoint->winner[suit].hand==partner[first]) {
          /* If partner has winning card */
          /*if (posPoint->secondBest[suit].hand==first)*/
          weight=suitWeightDelta+33+rRank;
          /*else 
	    weight=suitWeightDelta+28+rRank;*/
        } 
        else if ((mp->sequence)&&
          (mp->rank==posPoint->secondBest[suit].rank)) 
          weight=suitWeightDelta+31;
	/*else if (mp->sequence) 
          weight=suitWeightDelta+25-rRank;*/
        else 
	  weight=suitWeightDelta+13-(mp->rank);
	
	if ((bestMove[depth].suit==suit)&&
            (bestMove[depth].rank==mp->rank)) 
          weight+=17;
	else if ((bestMoveTT[depth].suit==suit)&&
            (bestMoveTT[depth].rank==mp->rank)) 
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
      k=kk & (-kk); l=ll & (-ll);  /* Only least significant 1 bit */
      if (winMove) {
        if (!notVoidInSuit) { 
          suitCount=posPoint->length[q][suit];
          suitAdd=(suitCount<<6)/(43/*36*/);
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
      if (WinningMove(mp, &(posPoint->move[depth+1]))) {
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
          if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(3/*2*/);
        
          if (posPoint->high[depth+1]==first) {
            if (suit==trump) 
              weight=30-(mp->rank)+suitAdd;  /* Ruffs partner's winner */  
	    else 
              weight=60-(mp->rank)+suitAdd;  
          } 
          else if (WinningMove(mp, &(posPoint->move[depth+1])))
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
	  if ((suitCount==2)&&(posPoint->secondBest[suit].hand==q))
            suitAdd-=(4/*2*/);	
          
          if (WinningMove(mp, &(posPoint->move[depth+1])))
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

	    weight=-(mp->rank);

          else if (WinningMove(mp, &(posPoint->move[depth+1]))) {
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
        else if (WinningMove(mp, &(posPoint->move[depth+1]))) 
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
      else if (WinningMove(mp, &(posPoint->move[depth+1])))
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
/*void InsertSort(int n, int depth) {
  int i, j;
  struct moveType a, temp;

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

  a=movePly[depth].move[0];
  for (i=1; i<=n-1; i++) 
    if (movePly[depth].move[i].weight>a.weight) {
      temp=a;
      a=movePly[depth].move[i];
      movePly[depth].move[i]=temp;
    }
  movePly[depth].move[0]=a; 
  for (i=2; i<=n-1; i++) {  
    j=i;
    a=movePly[depth].move[i];
    while (a.weight>movePly[depth].move[j-1].weight) {
      movePly[depth].move[j]=movePly[depth].move[j-1];
      j--;
    }
    movePly[depth].move[j]=a;
  }
}*/  


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


int AdjustMoveList(void) {
  int k, r, n, rank, suit;

  for (k=1; k<=13; k++) {
    suit=forbiddenMoves[k].suit;
    rank=forbiddenMoves[k].rank;
    for (r=0; r<=movePly[iniDepth].last; r++) {
      if ((suit==movePly[iniDepth].move[r].suit)&&
        (rank!=0)&&(rank==movePly[iniDepth].move[r].rank)) {
        /* For the forbidden move r: */
        for (n=r; n<=movePly[iniDepth].last; n++)
          movePly[iniDepth].move[n]=movePly[iniDepth].move[n+1];
        movePly[iniDepth].last--;
      }  
    }
  }
  return movePly[iniDepth].last+1;
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


inline int WinningMove(struct moveType * mvp1, struct moveType * mvp2) {
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


struct nodeCardsType * CheckSOP(struct pos * posPoint, struct nodeCardsType
  * nodep, int target, int tricks, int * result, int *value) {
    /* Check SOP if it matches the
    current position. If match, pointer to the SOP node is returned and
    result is set to TRUE, otherwise pointer to SOP node is returned
    and result set to FALSE. */

  /* 07-04-22 */ 
  if (nodeTypeStore[0]==MAXNODE) {
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
  /* Update SOP node with new values for upper and lower bounds. */
	
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
	int target, int tricks, int * valp) {
  struct nodeCardsType * sopP;
  struct winCardType * np;
  int s, res;

  np=nodeP; s=0;
  while ((np!=NULL)&&(s<4)) {
    if ((np->winMask & posPoint->orderSet[s])==
       np->orderSet)  {
      /* Winning rank set fits position */
      if (s==3) {
	sopP=CheckSOP(posPoint, np->first, target, tricks, &res, valp/*&val*/);
	/**valp=val;*/
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
  struct posSearchType *nodep, int * result) {
  /* If result is TRUE, a new SOP has been created and BuildPath returns a
  pointer to it. If result is FALSE, an existing SOP is used and BuildPath
  returns a pointer to the SOP */

  int found, suit;
  struct winCardType * np, * p2, * nprev, * fnp, *pnp;
  struct winCardType temp;
  struct nodeCardsType * sopP=0, * p;

  np=nodep->posSearchPoint;
  nprev=NULL;
  suit=0;

  /* If winning node has a card that equals the next winning card deduced
  from the position, then there already exists a (partial) path */

  if (np==NULL) {   /* There is no winning list created yet */
   /* Create winning nodes */
    p2=&winCards[winSetSize];
    AddWinSet();
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
      p2=&winCards[winSetSize];
      AddWinSet();
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
    p=&nodeCards[nodeSetSize];
    AddNodeSet();
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
    p2=&winCards[winSetSize];
    AddWinSet();
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
      p2=&winCards[winSetSize];
      AddWinSet();
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
    p=&nodeCards[nodeSetSize];
    AddNodeSet();
    np->first=p;
    *result=TRUE;
    return p;
  }  
}


struct posSearchType * SearchLenAndInsert(struct posSearchType
	* rootp, long long key, int insertNode, int *result) {
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
    sp=&posSearch[lenSetSize];
		
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
	p=sp/*&posSearch[lenSetSize]*/;
	AddLenSet();
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
	p=sp/*&posSearch[lenSetSize]*/;
	AddLenSet();
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



void BuildSOP(struct pos * posPoint, long long suitLengths, 
  int tricks, int firstHand, int target, int depth, int scoreFlag, int score) {
  int ss, hh, res, wm;
  unsigned short int w;
  unsigned short int temp[4][4];
  unsigned short int aggr[4];
  struct nodeCardsType * cardsP;
  struct posSearchType * np;

#ifdef TTDEBUG
  int k, mcurrent, rr;
  mcurrent=movePly[depth].current;
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
      posPoint->winMask[ss]=rel[aggr[ss]].winMask[ss];
      posPoint->winOrderSet[ss]=rel[aggr[ss]].aggrRanks[ss];
      wm=posPoint->winMask[ss];
      wm=wm & (-wm);
      posPoint->leastWin[ss]=InvWinMask(wm);
    }
  }

  /* 07-04-22 */
  if (scoreFlag) {
    if (nodeTypeStore[0]==MAXNODE) {
      posPoint->ubound=tricks+1;
      posPoint->lbound=target-posPoint->tricksMAX;
    }
    else {
      posPoint->ubound=tricks+1-target+posPoint->tricksMAX;
      posPoint->lbound=0;
    }
  }
  else {
    if (nodeTypeStore[0]==MAXNODE) {
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
      suitLengths=suitLengths<<4;
      suitLengths|=posPoint->length[hh][ss];
    }
  
  np=SearchLenAndInsert(rootnp[tricks][firstHand], suitLengths, TRUE, &res);
  
  cardsP=BuildPath(posPoint, np, &res);
  if (res) {
    cardsP->ubound=posPoint->ubound;
    cardsP->lbound=posPoint->lbound;
    if (((nodeTypeStore[firstHand]==MAXNODE)&&(scoreFlag))||
	((nodeTypeStore[firstHand]==MINNODE)&&(!scoreFlag))) {
      cardsP->bestMoveSuit=bestMove[depth].suit;
	  cardsP->bestMoveRank=bestMove[depth].rank;
    }
    else {
      cardsP->bestMoveSuit=0;
      cardsP->bestMoveRank=0;
    }
    posPoint->bestMoveSuit=bestMove[depth].suit;
    posPoint->bestMoveRank=bestMove[depth].rank;
    for (ss=0; ss<=3; ss++) 
      cardsP->leastWin[ss]=posPoint->leastWin[ss];
  }
      		
  #ifdef STAT
    c9[depth]++;
  #endif  

  #ifdef TTDEBUG
  if ((res) && (ttCollect) && (!suppressTTlog)) {
    fprintf(fp7, "cardsP=%d\n", (int)cardsP);
    fprintf(fp7, "nodeSetSize=%d\n", nodeSetSize);
    fprintf(fp7, "ubound=%d\n", cardsP->ubound);
    fprintf(fp7, "lbound=%d\n", cardsP->lbound);
    fprintf(fp7, "target=%d\n", target);
    fprintf(fp7, "first=%c nextFirst=%c\n",
      cardHand[posPoint->first[depth]], cardHand[posPoint->first[depth-1]]);
    fprintf(fp7, "bestMove:  suit=%c rank=%c\n", cardSuit[bestMove[depth].suit],
      cardRank[bestMove[depth].rank]);
    fprintf(fp7, "\n");
    fprintf(fp7, "Last trick:\n");
    fprintf(fp7, "1st hand=%c\n", cardHand[posPoint->first[depth+3]]);
    for (k=3; k>=0; k--) {
      mcurrent=movePly[depth+k+1].current;
      fprintf(fp7, "suit=%c  rank=%c\n",
        cardSuit[movePly[depth+k+1].move[mcurrent].suit],
        cardRank[movePly[depth+k+1].move[mcurrent].rank]);
    }
    fprintf(fp7, "\n");
    for (hh=0; hh<=3; hh++) {
      fprintf(fp7, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
	fprintf(fp7, "suit=%c", cardSuit[ss]);
	for (rr=14; rr>=2; rr--)
	  if (posPoint->rankInSuit[hh][ss] & bitMapRank[rr])
	    fprintf(fp7, " %c", cardRank[rr]);
	fprintf(fp7, "\n");
      }
      fprintf(fp7, "\n");
    }
    fprintf(fp7, "\n");

    for (hh=0; hh<=3; hh++) {
      fprintf(fp7, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
	fprintf(fp7, "suit=%c", cardSuit[ss]);
	for (rr=1; rr<=13; rr++)
	  if (posPoint->relRankInSuit[hh][ss] & bitMapRank[15-rr])
	    fprintf(fp7, " %c", cardRank[rr]);
	fprintf(fp7, "\n");
      }
      fprintf(fp7, "\n");
    }
    fprintf(fp7, "\n");
  }
  #endif  
}


int CheckDeal(struct moveType * cardp) {
  int h, s, k, found;
  unsigned short int temp[4][4];

  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      temp[h][s]=game.suit[h][s];

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


int NextMove(struct pos *posPoint, int depth) {
  /* Returns TRUE if at least one move remains to be
  searched, otherwise FALSE is returned. */
  int mcurrent;
  unsigned short int lw;
  unsigned char suit;
  struct moveType currMove;
  
  mcurrent=movePly[depth].current;
  currMove=movePly[depth].move[mcurrent];

  if (lowestWin[depth][currMove.suit]==0) {
    /* A small card has not yet been identified for this suit. */
    lw=posPoint->winRanks[depth][currMove.suit];
    if (lw!=0)
      lw=lw & (-lw);  /* LSB */
    else
      lw=bitMapRank[15];
    if (bitMapRank[currMove.rank]<lw) {
       /* The current move has a small card. */
      lowestWin[depth][currMove.suit]=lw;
      while (movePly[depth].current<=(movePly[depth].last-1)) {
        movePly[depth].current++;
        mcurrent=movePly[depth].current;
        if (bitMapRank[movePly[depth].move[mcurrent].rank] >=
	    lowestWin[depth][movePly[depth].move[mcurrent].suit]) { 
	  return TRUE;
	}
      }
      return FALSE;
    }
    else {
      while (movePly[depth].current<=movePly[depth].last-1) {
        movePly[depth].current++;
        mcurrent=movePly[depth].current;
        suit=movePly[depth].move[mcurrent].suit;
        if ((currMove.suit==suit) ||
	  (bitMapRank[movePly[depth].move[mcurrent].rank] >=
	    lowestWin[depth][suit])) {
	  return TRUE;
	}
      }
      return FALSE;
    }
  }
  else {
    while (movePly[depth].current<=movePly[depth].last-1) { 
      movePly[depth].current++;
      mcurrent=movePly[depth].current;
      if (bitMapRank[movePly[depth].move[mcurrent].rank] >=
	    lowestWin[depth][movePly[depth].move[mcurrent].suit]) {
	return TRUE;
      }
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



void Wipe(void) {
  int k;

  for (k=1; k<=wcount; k++) {
    if (pw[k])
      free(pw[k]);
    pw[k]=NULL;
  }
  for (k=1; k<=ncount; k++) {
    if (pn[k])
      free(pn[k]);
    pn[k]=NULL;
  }
  for (k=1; k<=lcount; k++) {
    if (pl[k])
      free(pl[k]);
    pl[k]=NULL;
  }
	
  allocmem=summem/*(WINIT+1)*sizeof(struct winCardType)+
	  (NINIT+1)*sizeof(struct nodeCardsType)+
	  (LINIT+1)*sizeof(struct posSearchType)*/;

  return;
}


void AddWinSet(void) {
  if (clearTTflag) {
    windex++;
    winSetSize=windex;
    /*fp2=fopen("dyn.txt", "a");
    fprintf(fp2, "windex=%d\n", windex);
    fclose(fp2);*/
    winCards=&temp_win[windex];
  }
  else if (winSetSize>=winSetSizeLimit) {
    /* The memory chunk for the winCards structure will be exceeded. */
    if ((allocmem+wmem)>maxmem) {
      /* Already allocated memory plus needed allocation overshot maxmem */
      windex++;
      winSetSize=windex;
      /*fp2=fopen("dyn.txt", "a");
      fprintf(fp2, "windex=%d\n", windex);
      fclose(fp2);*/
      clearTTflag=TRUE;
      winCards=&temp_win[windex];
    }
    else {
      wcount++; winSetSizeLimit=WSIZE; 
      pw[wcount] = (struct winCardType *)calloc(winSetSizeLimit+1, sizeof(struct winCardType));
      if (pw[wcount]==NULL) {
        clearTTflag=TRUE;
        windex++;
	winSetSize=windex;
	winCards=&temp_win[windex];
      }
      else {
	allocmem+=(winSetSizeLimit+1)*sizeof(struct winCardType);
	winSetSize=0;
	winCards=pw[wcount];
      }
    }
  }
  else
    winSetSize++;
  return;
}

void AddNodeSet(void) {
  if (nodeSetSize>=nodeSetSizeLimit) {
    /* The memory chunk for the nodeCards structure will be exceeded. */
    if ((allocmem+nmem)>maxmem) {
      /* Already allocated memory plus needed allocation overshot maxmem */  
      clearTTflag=TRUE;
    }
    else {
      ncount++; nodeSetSizeLimit=NSIZE; 
      pn[ncount] = (struct nodeCardsType *)calloc(nodeSetSizeLimit+1, sizeof(struct nodeCardsType));
      if (pn[ncount]==NULL) {
        clearTTflag=TRUE;
      }
      else {
	allocmem+=(nodeSetSizeLimit+1)*sizeof(struct nodeCardsType);
	nodeSetSize=0;
	nodeCards=pn[ncount];
      }
    }
  }
  else
    nodeSetSize++;
  return;
}

void AddLenSet(void) {
  if (lenSetSize>=lenSetSizeLimit) {
      /* The memory chunk for the posSearchType structure will be exceeded. */
    if ((allocmem+lmem)>maxmem) { 
       /* Already allocated memory plus needed allocation overshot maxmem */
      clearTTflag=TRUE;
    }
    else {
      lcount++; lenSetSizeLimit=LSIZE; 
      pl[lcount] = (struct posSearchType *)calloc(lenSetSizeLimit+1, sizeof(struct posSearchType));
      if (pl[lcount]==NULL) {
        clearTTflag=TRUE;
      }
      else {
	allocmem+=(lenSetSizeLimit+1)*sizeof(struct posSearchType);
	lenSetSize=0;
	posSearch=pl[lcount];
      }
    }
  }
  else
    lenSetSize++;
  return;
} 

#ifdef PLUSVER
int STDCALL CalcDDtable(struct ddTableDeal tableDeal, struct ddTableResults * tablep) {

  int h, s, k, tr, first, res;
  struct deal dl;
  struct futureTricks fut;

  for (h=0; h<=3; h++)
    for (s=0; s<=3; s++)
      dl.remainCards[h][s]=tableDeal.cards[h][s];

  for (k=0; k<=2; k++) {
    dl.currentTrickRank[k]=0;
    dl.currentTrickSuit[k]=0;
  }

  for (tr=4; tr>=0; tr--) 
    for (first=0; first<=3; first++) {
      dl.first=first;
      dl.trump=tr;
      res=SolveBoard(dl, -1, 1, 1, &fut, 0);
      if (res==1) {
	tablep->resTable[tr][rho[dl.first]]=13-fut.score[0];
      }
      else
	/*return 0;*/
        return res;		/* 2011-07-27 */
    }

  return 1;
}

int STDCALL CalcDDtablePBN(struct ddTableDealPBN tableDealPBN, 
  struct ddTableResults * tablep) {
  int res;
  struct ddTableDeal tableDeal;
  int ConvertFromPBN(char * dealBuff, unsigned int remainCards[4][4]);

  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards)!=1)
	return -99;

  res=CalcDDtable(tableDeal, tablep);

  return res;
}
#endif


#ifdef TTDEBUG

void ReceiveTTstore(struct pos *posPoint, struct nodeCardsType * cardsP, 
  int target, int depth) {
/* Stores current position information and TT position value in table
  ttStore with current entry lastTTStore. Also stores corresponding
  information in log rectt.txt. */
  tricksLeft=0;
  for (hh=0; hh<=3; hh++)
    for (ss=0; ss<=3; ss++)
      tricksLeft=tricksLeft+posPoint->length[hh][ss];
  tricksLeft=tricksLeft/4;
  ttStore[lastTTstore].tricksLeft=tricksLeft;
  ttStore[lastTTstore].cardsP=cardsP;
  ttStore[lastTTstore].first=posPoint->first[depth];
  if ((handToPlay==posPoint->first[depth])||
    (handToPlay==partner[posPoint->first[depth]])) {
    ttStore[lastTTstore].target=target-posPoint->tricksMAX;
    ttStore[lastTTstore].ubound=cardsP->ubound[handToPlay];
    ttStore[lastTTstore].lbound=cardsP->lbound[handToPlay];
  }
  else {
    ttStore[lastTTstore].target=tricksLeft-
      target+posPoint->tricksMAX+1;
  }
  for (hh=0; hh<=3; hh++)
    for (ss=0; ss<=3; ss++)
      ttStore[lastTTstore].suit[hh][ss]=
        posPoint->rankInSuit[hh][ss];
  fp11=fopen("rectt.txt", "a");
  if (lastTTstore<SEARCHSIZE) {
    fprintf(fp11, "lastTTstore=%d\n", lastTTstore);
    fprintf(fp11, "tricksMAX=%d\n", posPoint->tricksMAX);
    fprintf(fp11, "leftTricks=%d\n",
      ttStore[lastTTstore].tricksLeft);
    fprintf(fp11, "cardsP=%d\n",
      ttStore[lastTTstore].cardsP);
    fprintf(fp11, "ubound=%d\n",
      ttStore[lastTTstore].ubound);
    fprintf(fp11, "lbound=%d\n",
      ttStore[lastTTstore].lbound);
    fprintf(fp11, "first=%c\n",
      cardHand[ttStore[lastTTstore].first]);
    fprintf(fp11, "target=%d\n",
      ttStore[lastTTstore].target);
    fprintf(fp11, "\n");
    for (hh=0; hh<=3; hh++) {
      fprintf(fp11, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
        fprintf(fp11, "suit=%c", cardSuit[ss]);
        for (rr=14; rr>=2; rr--)
          if (ttStore[lastTTstore].suit[hh][ss]
            & bitMapRank[rr])
            fprintf(fp11, " %c", cardRank[rr]);
         fprintf(fp11, "\n");
      }
      fprintf(fp11, "\n");
    }
    for (hh=0; hh<=3; hh++) {
      fprintf(fp11, "hand=%c\n", cardHand[hh]);
      for (ss=0; ss<=3; ss++) {
        fprintf(fp11, "suit=%c", cardSuit[ss]);
        for (rr=1; rr<=13; rr++)
          if (posPoint->relRankInSuit[hh][ss] & bitMapRank[15-rr])
            fprintf(fp11, " %c", cardRank[rr]);
        fprintf(fp11, "\n");
      }
      fprintf(fp11, "\n");
    }
  }
  fclose(fp11);
  lastTTstore++;
}
#endif

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

#ifdef PLUSVER
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

  res=SolveBoard(dl, target, solutions, mode, futp, 0);

  return res;
}

#else
#ifdef PBN
int STDCALL SolveBoardPBN(struct dealPBN dlpbn, int target, 
    int solutions, int mode, struct futureTricks *futp) {
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

  res=SolveBoard(dl, target, solutions, mode, futp);

  return res;
}
#endif
#endif



