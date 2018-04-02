/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include "CalcTables.h"
#include "SolverIF.h"
#include "SolveBoard.h"
#include "System.h"
#include "Memory.h"
#include "Scheduler.h"
#include "PBN.h"


paramType cparam;

extern System sysdep;
extern Memory memory;
extern Scheduler scheduler;

int CalcAllBoardsN(
  boards * bop,
  solvedBoards * solvedp);


void CalcSingleCommon(
  const int thrId,
  const int bno)
{
  // Solves a single deal and strain for all four declarers.

  futureTricks fut;
  cparam.bop->deals[bno].first = 0;

  START_THREAD_TIMER(thrId);
  int res = SolveBoard(
                cparam.bop->deals[bno],
                cparam.bop->target[bno],
                cparam.bop->solutions[bno],
                cparam.bop->mode[bno],
                &fut,
                thrId);

  // SH: I'm making a terrible use of the fut structure here.

  if (res == 1)
    cparam.solvedp->solvedBoard[bno].score[0] = fut.score[0];
  else
    cparam.error = res;

  ThreadData * thrp = memory.GetPtr(static_cast<unsigned>(thrId));
  for (int k = 1; k < DDS_HANDS; k++)
  {
    int hint = (k == 2 ? fut.score[0] : 13 - fut.score[0]);

    cparam.bop->deals[bno].first = k; // Next declarer

    res = SolveSameBoard(thrp, cparam.bop->deals[bno], &fut, hint);

    if (res == 1)
      cparam.solvedp->solvedBoard[bno].score[k] = fut.score[0];
    else
      cparam.error = res;
  }
  END_THREAD_TIMER(thrId);
}


void CopyCalcSingle(const vector<int>& crossrefs)
{
  for (unsigned i = 0; i < crossrefs.size(); i++)
  {
    if (crossrefs[i] == -1)
      continue;

    START_THREAD_TIMER(thrId);
    for (int k = 0; k < DDS_HANDS; k++)
      cparam.solvedp->solvedBoard[i].score[k] = 
        cparam.solvedp->solvedBoard[ crossrefs[i] ].score[k];
    END_THREAD_TIMER(thrId);
  }
}


void CalcChunkCommon(
  const int thrId)
{
  // Solves each deal and strain for all four declarers.
  vector<futureTricks> fut;
  fut.resize(static_cast<unsigned>(cparam.noOfBoards));

  int index;
  schedType st;

  while (1)
  {
    st = scheduler.GetNumber(thrId);
    index = st.number;
    if (index == -1)
      break;

    if (st.repeatOf != -1)
    {
      START_THREAD_TIMER(thrId);
      for (int k = 0; k < DDS_HANDS; k++)
      {
        cparam.bop->deals[index].first = k;

        cparam.solvedp->solvedBoard[index].score[k] =
          cparam.solvedp->solvedBoard[ st.repeatOf ].score[k];
      }
      END_THREAD_TIMER(thrId);
      continue;
    }

    CalcSingleCommon(thrId, index);
  }
}


int CalcAllBoardsN(
  boards * bop,
  solvedBoards * solvedp)
{
  cparam.error = 0;

  if (bop->noOfBoards > MAXNOOFBOARDS)
    return RETURN_TOO_MANY_BOARDS;

  cparam.bop = bop;
  cparam.solvedp = solvedp;
  cparam.noOfBoards = bop->noOfBoards;

  scheduler.RegisterRun(DDS_RUN_CALC, * bop);
  sysdep.RegisterRun(DDS_RUN_CALC, * bop);

  for (int k = 0; k < MAXNOOFBOARDS; k++)
    solvedp->solvedBoard[k].cards = 0;

  START_BLOCK_TIMER;
  int retRun = sysdep.RunThreads();
  END_BLOCK_TIMER;

  if (retRun != RETURN_NO_FAULT)
    return retRun;

  solvedp->noOfBoards = cparam.noOfBoards;

#ifdef DDS_SCHEDULER 
  scheduler.PrintTiming();
#endif

  if (cparam.error == 0)
    return RETURN_NO_FAULT;
  else
    return cparam.error;
}



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
  bo.noOfBoards = DDS_STRAINS;

  for (int tr = DDS_STRAINS-1; tr >= 0; tr--)
  {
    dl.trump = tr;
    bo.deals[ind] = dl;
    bo.target[ind] = -1;
    bo.solutions[ind] = 1;
    bo.mode[ind] = 1;
    ind++;
  }

  int res = CalcAllBoardsN(&bo, &solved);
  if (res != 1)
    return res;

  for (int index = 0; index < DDS_STRAINS; index++)
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

  for (int k = 0; k < DDS_STRAINS; k++)
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
    for (int tr = DDS_STRAINS-1; tr >= 0; tr--)
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

  int res = CalcAllBoardsN(&bo, &solved);
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
  ddTableDeals dls;
  for (int k = 0; k < dealsp->noOfTables; k++)
    if (ConvertFromPBN(dealsp->deals[k].cards, dls.deals[k].cards) != 1)
      return RETURN_PBN_FAULT;

  dls.noOfTables = dealsp->noOfTables;

  int res = CalcAllTables(&dls, mode, trumpFilter, resp, presp);
  return res;
}


int STDCALL CalcDDtablePBN(
  ddTableDealPBN tableDealPBN,
  ddTableResults * tablep)
{
  ddTableDeal tableDeal;
  if (ConvertFromPBN(tableDealPBN.cards, tableDeal.cards) != 1)
    return RETURN_PBN_FAULT;

  int res = CalcDDtable(tableDeal, tablep);
  return res;
}


void DetectCalcDuplicates(
  const boards& bds,
  vector<int>& uniques,
  vector<int>& crossrefs)
{
  // Could save a little bit of time with a dedicated checker that
  // only looks at the cards.
  return DetectSolveDuplicates(bds, uniques, crossrefs);
}

