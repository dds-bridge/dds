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
#include "SolveBoard.h"
#include "PBN.h"


int STDCALL CalcDDtable(struct ddTableDeal tableDeal, struct ddTableResults * tablep) {

  int h, s, k, ind, tr, first, res;
  struct deal dl;
  struct boards bo;
  struct solvedBoards solved;

  for (h=0; h<DDS_HANDS; h++)
    for (s=0; s<DDS_SUITS; s++)
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
    return RETURN_NO_FAULT;
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
	  lastBoardIndex[MAXNOOFBOARDS>>2], count=0;
  bool okey = false;
  struct boards bo;
  struct solvedBoards solved;

  /*int Par(struct ddTableResults * tablep, struct parResults *presp, int vulnerable);*/

  for (k=0; k<5; k++) { 
    if (!trumpFilter[k]) {
      okey=true; 
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
	  for (h=0; h<DDS_HANDS; h++)
            for (s=0; s<DDS_SUITS; s++)
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
    return RETURN_NO_FAULT;
  }
  return res;
}
 

int STDCALL CalcAllTablesPBN(struct ddTableDealsPBN *dealsp, int mode, int trumpFilter[5], 
    struct ddTablesRes *resp, struct allParResults *presp) {
  int res, k;
  struct ddTableDeals dls;

  for (k=0; k<dealsp->noOfTables; k++)
    if (ConvertFromPBN(dealsp->deals[k].cards, dls.deals[k].cards)!=1)
      return RETURN_PBN_FAULT;

  dls.noOfTables=dealsp->noOfTables;

  res=CalcAllTables(&dls, mode, trumpFilter, resp, presp);

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


