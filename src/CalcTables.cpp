/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "dds.h"
#include "SolveBoard.h"
#include "PBN.h"


int STDCALL CalcDDtable(
  ddTableDeal tableDeal,
  ddTableResults * tablep)
{
  deal dl;
  boards bo;
  solvedBoards solved;

  for (int h = 0; h < DDS_HANDS; h++)
    for (int s = 0; s < DDS_SUITS; s++)
      dl.remainCards[h][s] = tableDeal.cards[h][s];

  for (int k = 0; k <= 2; k++)
  {
    dl.currentTrickRank[k] = 0;
    dl.currentTrickSuit[k] = 0;
  }

  int ind = 0;
  bo.noOfBoards = 5;

  for (int tr = 4; tr >= 0; tr--)
  {
    dl.trump = tr;
    bo.deals[ind] = dl;
    bo.target[ind] = -1;
    bo.solutions[ind] = 1;
    bo.mode[ind] = 1;
    ind++;
  }

  int res = SolveAllBoardsN(&bo, &solved, 4, 1);
  if (res == 1)
  {
    for (int index = 0; index < 5; index++)
    {
      int strain = bo.deals[index].trump;

      // SH: I'm making a terrible use of the fut structure here.

      for (int first = 0; first < DDS_HANDS; first++)
      {
        tablep->resTable[strain][ rho[first] ] =
          13 - solved.solvedBoard[index].score[first];
      }
    }
    return RETURN_NO_FAULT;
  }

  return res;
}


int STDCALL CalcAllTables(
  ddTableDeals * dealsp,
  int mode,
  int trumpFilter[5],
  ddTablesRes * resp,
  allParResults * presp)
{
  /* mode = 0: par calculation, vulnerability None
     mode = 1: par calculation, vulnerability All
     mode = 2: par calculation, vulnerability NS
     mode = 3: par calculation, vulnerability EW
         mode = -1: no par calculation */

  boards bo;
  solvedBoards solved;
  int count = 0;
  bool okey = false;

  for (int k = 0; k < 5; k++)
  {
    if (!trumpFilter[k])
    {
      okey = true;
      count++;
    }
  }

  if (!okey)
    return RETURN_NO_SUIT;

  if (count * dealsp->noOfTables > MAXNOOFTABLES * DDS_STRAINS)
    return RETURN_TOO_MANY_TABLES;

  int ind = 0;
  int lastIndex = 0;
  resp->noOfBoards = 0;

  for (int m = 0; m < dealsp->noOfTables; m++)
  {
    for (int tr = 4; tr >= 0; tr--)
    {
      if (trumpFilter[tr])
        continue;

      for (int h = 0; h < DDS_HANDS; h++)
        for (int s = 0; s < DDS_SUITS; s++)
          bo.deals[ind].remainCards[h][s] =
            dealsp->deals[m].cards[h][s];

      bo.deals[ind].trump = tr;

      for (int k = 0; k <= 2; k++)
      {
        bo.deals[ind].currentTrickRank[k] = 0;
        bo.deals[ind].currentTrickSuit[k] = 0;
      }

      bo.target[ind] = -1;
      bo.solutions[ind] = 1;
      bo.mode[ind] = 1;
      lastIndex = ind;
      ind++;
    }
  }

  bo.noOfBoards = lastIndex + 1;

  int res = SolveAllBoardsN(&bo, &solved, 4, 1);
  if (res != 1)
    return res;

  resp->noOfBoards += 4 * solved.noOfBoards;

  for (int m = 0; m < dealsp->noOfTables; m++)
  {
    for (int strainIndex = 0; strainIndex < count; strainIndex++)
    {
      int index = m * count + strainIndex;
      int strain = bo.deals[index].trump;

      // SH: I'm making a terrible use of the fut structure here.

      for (int first = 0; first < DDS_HANDS; first++)
      {
        resp->results[m].resTable[strain][ rho[first] ] =
          13 - solved.solvedBoard[index].score[first];
      }
    }
  }

  if ((mode > -1) && (mode < 4) && (count == 5))
  {
    /* Calculate par */
    for (int k = 0; k < dealsp->noOfTables; k++)
    {
      res = Par(&(resp->results[k]), &(presp->presults[k]), mode);
      /* vulnerable 0: None 1: Both 2: NS 3: EW */
      if (res != 1)
        return res;
    }
  }
  return RETURN_NO_FAULT;
}


int STDCALL CalcAllTablesPBN(
  ddTableDealsPBN * dealsp,
  int mode,
  int trumpFilter[5],
  ddTablesRes * resp,
  allParResults * presp)
{
  int res;
  ddTableDeals dls;

  for (int k = 0; k < dealsp->noOfTables; k++)
    if (ConvertFromPBN(dealsp->deals[k].cards, dls.deals[k].cards) != 1)
      return RETURN_PBN_FAULT;

  dls.noOfTables = dealsp->noOfTables;

  res = CalcAllTables(&dls, mode, trumpFilter, resp, presp);

  return res;
}


int STDCALL CalcDDtablePBN(
  ddTableDealPBN tableDealPBN,
  ddTableResults * tablep)
{
  ddTableDeal tableDeal;
  int res;

  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards) != 1)
    return RETURN_PBN_FAULT;

  res = CalcDDtable(tableDeal, tablep);

  return res;
}


