/*
   DDS, a bridge double dummy solver.

   Copyright (C) 2006-2014 by Bo Haglund /
   2014-2018 by Bo Haglund & Soren Hein.

   See LICENSE and README.
*/


#include <vector>
#include <string.h>

#include "compare.h"

using namespace std;


bool compare_PBN(
  const dealPBN& dl1, 
  const dealPBN& dl2)
{
  if (dl1.trump != dl2.trump) 
    return false;
  if (dl1.first != dl2.first) 
    return false;
  if (strcmp(dl1.remainCards, dl2.remainCards)) 
    return false;

  return true;
}


bool compare_FUT(
  const futureTricks& fut1, 
  const futureTricks& fut2)
{
  if (fut1.cards != fut2.cards)
    return false;

  for (int i = 0; i < fut1.cards; i++)
  {
    if (fut1.suit[i] != fut2.suit[i]) 
      return false;
    if (fut1.rank[i] != fut2.rank[i]) 
      return false;
    if (fut1.equals[i] != fut2.equals[i]) 
      return false;
    if (fut1.score[i] != fut2.score[i]) 
      return false;
  }

  return true;
}


bool compare_TABLE(
  const ddTableResults& table1, 
  const ddTableResults& table2)
{
  for (int suit = 0; suit < DDS_SUITS; suit++)
  {
    for (int pl = 0; pl < DDS_HANDS; pl++)
      if (table1.resTable[suit][pl] != table2.resTable[suit][pl])
        return false;
  }

  return true;
}


bool compare_PAR(
  const parResults& par1,
  const parResults& par2)
{
  if (strcmp(par1.parScore[0], par2.parScore[0])) 
    return false;
  if (strcmp(par1.parScore[1], par2.parScore[1])) 
    return false;
  if (strcmp(par1.parContractsString[0], par2.parContractsString[0]))
    return false;
  if (strcmp(par1.parContractsString[1], par2.parContractsString[1]))
    return false;

  return true;
}


bool compare_DEALERPAR(
  const parResultsDealer& par1,
  const parResultsDealer& par2)
{
  if (par1.score != par2.score) 
    return false;

  for (int i = 0; i < par1.number; i++)
    if (strcmp(par1.contracts[i], par2.contracts[i]))
      return false;

  return true;
}


bool compare_TRACE(
  const solvedPlay& trace1,
  const solvedPlay& trace2)
{
  // In a buglet, Trace returned trace1 == -3 if there is
  // no input at all (trace2 is then 0).
  if (trace1.number != trace2.number && trace2.number > 0)
    return false;

  // Once that was fixed, the input file had length 0, not 1.
  if (trace1.number == 1 && trace2.number == 0)
    return true;

  for (int i = 0; i < trace1.number; i++)
  {
    if (trace1.tricks[i] != trace2.tricks[i])
      return false;
  }

  return true;
}

